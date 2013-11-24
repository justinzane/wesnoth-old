/**
 * @file src/game_events/conditional_wml.cpp
 * @project The Battle for Wesnoth NG - https://github.com/justinzane/wesnoth-ng
 * @brief 
 * @authors 
 * @copyright Copyright (C) 2003 - 2013 by David White <dave@whitevine.net>
   Part of the Battle for Wesnoth Project http://www.wesnoth.org/

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/

/**
 * @file
 * Implementations of conditional action WML tags.
 */

#include "../global.hpp"
#include "conditional_wml.hpp"

#include "../config.hpp"
#include "../gamestatus.hpp"
#include "../log/log.hpp"
#include "../log/log.hpp"
#include "../resources.hpp"
#include "../serdes/string_utils.hpp"
#include "../team.hpp"
#include "../terrain_filter.hpp"
#include "../unit/unit.hpp"
#include "../unit/unit.hpp"
#include "../unit/unit_map.hpp"
#include "../unit/unit_map.hpp"
#include "../unit/unit_types.hpp"
#include "../unit/unit_types.hpp"
#include "../util.hpp"
#include "../variable.hpp"

#include "global.hpp"
#include <boost/foreach.hpp>


static lg::log_domain log_engine("engine");
#define WRN_NG LOG_STREAM(warn, log_engine)


// This file is in the game_events namespace.
namespace game_events {

namespace { // Support functions

	bool internal_conditional_passed(const vconfig& cond, bool& backwards_compat)
	{
		static std::vector<std::pair<int,int> > default_counts = utils::parse_ranges("1-99999");

		// If the if statement requires we have a certain unit,
		// then check for that.
		const vconfig::child_list& have_unit = cond.get_children("have_unit");
		backwards_compat = backwards_compat && have_unit.empty();
		for(vconfig::child_list::const_iterator u = have_unit.begin(); u != have_unit.end(); ++u) {
			if(resources::units == nullptr)
				return false;
			std::vector<std::pair<int,int> > counts = (*u).has_attribute("count")
				? utils::parse_ranges((*u)["count"]) : default_counts;
			int match_count = 0;
			foreach_ng(const unit &i, *resources::units)
			{
				if ( i.hitpoints() > 0  &&  i.matches_filter(*u) ) {
					++match_count;
					if(counts == default_counts) {
						// by default a single match is enough, so avoid extra work
						break;
					}
				}
			}
			if ((*u)["search_recall_list"].to_bool())
			{
				for(std::vector<team>::iterator team = resources::teams->begin();
						team!=resources::teams->end(); ++team)
				{
					if(counts == default_counts && match_count) {
						break;
					}
					const std::vector<unit>& avail_units = team->recall_list();
					for(std::vector<unit>::const_iterator unit = avail_units.begin(); unit!=avail_units.end(); ++unit) {
						if(counts == default_counts && match_count) {
							break;
						}
						scoped_recall_unit auto_store("this_unit", team->save_id(), unit - avail_units.begin());
						if ( unit->matches_filter(*u) ) {
							++match_count;
						}
					}
				}
			}
			if(!in_ranges(match_count, counts)) {
				return false;
			}
		}

		// If the if statement requires we have a certain location,
		// then check for that.
		const vconfig::child_list& have_location = cond.get_children("have_location");
		backwards_compat = backwards_compat && have_location.empty();
		for(vconfig::child_list::const_iterator v = have_location.begin(); v != have_location.end(); ++v) {
			std::set<map_location> res;
			terrain_filter(*v, *resources::units).get_locations(res);

			std::vector<std::pair<int,int> > counts = (*v).has_attribute("count")
				? utils::parse_ranges((*v)["count"]) : default_counts;
			if(!in_ranges<int>(res.size(), counts)) {
				return false;
			}
		}

		// Check against each variable statement,
		// to see if the variable matches the conditions or not.
		const vconfig::child_list& variables = cond.get_children("variable");
		backwards_compat = backwards_compat && variables.empty();

		foreach_ng(const vconfig &values, variables)
		{
			const std::string name = values["name"];
			config::attribute_value value = resources::gamedata->get_variable_const(name);

#define TEST_STR_ATTR(name, test) do { \
			if (values.has_attribute(name)) { \
				std::string attr_str = values[name].str(); \
				std::string str_value = value.str(); \
				if (!(test)) return false; \
			} \
			} while (0)

#define TEST_NUM_ATTR(name, test) do { \
			if (values.has_attribute(name)) { \
				double attr_num = values[name].to_double(); \
				double num_value = value.to_double(); \
				if (!(test)) return false; \
			} \
			} while (0)

#define TEST_BOL_ATTR(name, test) do { \
			if (values.has_attribute(name)) { \
				bool attr_bool = values[name].to_bool(); \
				bool bool_value = value.to_bool(); \
				if (!(test)) return false; \
			} \
			} while (0)

			TEST_STR_ATTR("equals",                str_value == attr_str);
			TEST_STR_ATTR("not_equals",            str_value != attr_str);
			TEST_NUM_ATTR("numerical_equals",      num_value == attr_num);
			TEST_NUM_ATTR("numerical_not_equals",  num_value != attr_num);
			TEST_NUM_ATTR("greater_than",          num_value >  attr_num);
			TEST_NUM_ATTR("less_than",             num_value <  attr_num);
			TEST_NUM_ATTR("greater_than_equal_to", num_value >= attr_num);
			TEST_NUM_ATTR("less_than_equal_to",    num_value <= attr_num);
			TEST_BOL_ATTR("boolean_equals",       bool_value == attr_bool);
			TEST_BOL_ATTR("boolean_not_equals",   bool_value != attr_bool);
			TEST_STR_ATTR("contains", str_value.find(attr_str) != std::string::npos);

#undef TEST_STR_ATTR
#undef TEST_NUM_ATTR
#undef TEST_BOL_ATTR
		}
		return true;
	}

} // end anonymous namespace (support functions)


bool conditional_passed(const vconfig& cond, bool backwards_compat)
{
	bool allow_backwards_compat = backwards_compat = backwards_compat &&
		cond["backwards_compat"].to_bool(true);
	bool matches = internal_conditional_passed(cond, allow_backwards_compat);

	// Handle [and], [or], and [not] with in-order precedence
	int or_count = 0;
	vconfig::all_children_iterator cond_i = cond.ordered_begin();
	vconfig::all_children_iterator cond_end = cond.ordered_end();
	while(cond_i != cond_end)
	{
		const std::string& cond_name = cond_i.get_key();
		const vconfig& cond_filter = cond_i.get_child();

		// Handle [and]
		if(cond_name == "and")
		{
			matches = matches && conditional_passed(cond_filter, backwards_compat);
			backwards_compat = false;
		}
		// Handle [or]
		else if(cond_name == "or")
		{
			matches = matches || conditional_passed(cond_filter, backwards_compat);
			++or_count;
		}
		// Handle [not]
		else if(cond_name == "not")
		{
			matches = matches && !conditional_passed(cond_filter, backwards_compat);
			backwards_compat = false;
		}
		++cond_i;
	}
	// Check for deprecated [or] syntax
	if(matches && or_count > 1 && allow_backwards_compat)
	{
		///@deprecated r18803 [or] syntax
		lg::wml_error << "possible deprecated [or] syntax: now forcing re-interpretation\n";
		/**
		 * @todo For now we will re-interpret it according to the old
		 * rules, but this should be later to prevent re-interpretation
		 * errors.
		 */
		const vconfig::child_list& orcfgs = cond.get_children("or");
		for(unsigned int i=0; i < orcfgs.size(); ++i) {
			if(conditional_passed(orcfgs[i])) {
				return true;
			}
		}
		return false;
	}
	return matches;
}

bool matches_special_filter(const config &cfg, const vconfig& filter)
{
	if (!cfg) {
		WRN_NG << "attempt to filter attack for an event with no attack data.\n";
		// better to not execute the event (so the problem is more obvious)
		return false;
	}
	const attack_type attack(cfg);
	return attack.matches_filter(filter.get_parsed_config());
}

} // end namespace game_events

