/*
   Copyright (C) 2013 by Felix Bauer <fehlxb+wesnoth@gmail.com>
   Part of the Battle for Wesnoth Project http://www.wesnoth.org/

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/

#include "unit_advancements_aspect.hpp"
#include "../../log.hpp"
#include "lua/lualib.h"
#include "lua/lauxlib.h"
#include <string>
#include <vector>

#include "scripting/lua.hpp"
#include "scripting/lua_api.hpp"

#include "global.hpp"

#include "../../unit.hpp"
#include "../../board/map.hpp"


static lg::log_domain log_ai_engine_lua("ai/engine/lua");
#define LOG_LUA LOG_STREAM(info, log_ai_engine_lua)
#define ERR_LUA LOG_STREAM(err, log_ai_engine_lua)

namespace ai{

unit_advancements_aspect::unit_advancements_aspect():
		val_(), L_(),ref_()
{
}

unit_advancements_aspect::unit_advancements_aspect(lua_State* L, int n)
{
	val_ = "Lua Function";
	L_ = L;
	lua_settop(L, n);

	//on the top of the Lua-Stack is now the pointer to the function. Save it:
	ref_ = luaL_ref(L, LUA_REGISTRYINDEX);

}

unit_advancements_aspect::unit_advancements_aspect(const std::string& val):  val_(val), L_(), ref_()
{
}

const std::vector<std::string> unit_advancements_aspect::get_advancements(const unit_map::const_iterator& unit) const
{


	if(!unit.valid())
	{
		return std::vector<std::string>();
	}

	const std::string& unit_id = (*unit).id();
	const int unit_x = (*unit).get_location().x + 1;
	const int unit_y = (*unit).get_location().y + 1;

	LOG_LUA << "Entering unit_advancements_aspect::get_advancements() in instance " << this << " with unit " << unit_id <<  " on (x,y) = (" << unit_x << ", " << unit_y << ")\n";

	if(L_ == NULL || ref_ == LUA_REFNIL)
	{
		//If we end up here, most likely the aspect don't use the lua-engine.
		//Just to make sure:
		if (val_ == "Lua Function")
		{
			return std::vector<std::string>();
		}
		return utils::split(val_);
	}

	//put the Pointer back on the Stack
	lua_rawgeti(L_, LUA_REGISTRYINDEX, ref_);

	if(lua_isstring(L_, -1))
	{
		return utils::split(lua_tostring(L_, -1));
	}

	if(!lua_isfunction(L_, -1))
	{
		ERR_LUA << "Can't evaluate advancement aspect: Value is neither a string nor a function.\n";
		return std::vector<std::string>();
	}

	//push parameter to the stack
	lua_pushinteger(L_, unit_x);
	lua_pushinteger(L_, unit_y);

	//To make unit_id a Parameter of the Lua function:
	//lua_pushfstring(L_, unit_id.c_str());

	//call function
	if(lua_pcall(L_, 2, 1, 0) != 0)
	{
		ERR_LUA << "LUA Error while evaluating advancements_aspect: " << lua_tostring(L_, -1) << "\n";
		return std::vector<std::string>();
	}
	if (!lua_isstring(L_, -1))
	{
		ERR_LUA << "LUA Error while evaluating advancements_aspect: Function must return String \n";
		return std::vector<std::string>();
	}

	//get result from Lua-Stack
	const std::string retval = std::string(lua_tostring(L_, -1));
	lua_pop(L_, 1);

	LOG_LUA << "Called Lua advancement function. Result was: \"" << retval << "\".\n";

	return utils::split(retval);
}


const std::string unit_advancements_aspect::get_value() const
{
	return val_;
}
}
