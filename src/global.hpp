/**
 * @file src/global.hpp
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

#ifndef GLOBAL_HPP_INCLUDED
#define GLOBAL_HPP_INCLUDED

/**
 * @def foreach_ng
 * @brief This is simply a wrapper for Boost's BOOST_FOREACH since
 * Eclipse CDT's parser has ungodly fits with the real thing.
 * @link https://bugs.eclipse.org/bugs/show_bug.cgi?id=332278
 * @note It would probably be ideal to replace these with c++11 for calls
 * where possible.
 * @date 2013-11-12 @author justinzane
 */
#ifdef __CDT_PARSER__
    #define foreach_ng(a, b) for(a : b)
#else
    #define foreach_ng(a, b) BOOST_FOREACH(a, b)
#endif



#ifdef _MSC_VER

#undef snprintf
#define snprintf _snprintf

// Disable warning about source encoding not in current code page.
#pragma warning(disable: 4819)

// Disable warning about deprecated functions.
#pragma warning(disable: 4996)

//disable some MSVC warnings which are useless according to mordante
#pragma warning(disable: 4244)
#pragma warning(disable: 4345)
#pragma warning(disable: 4250)
#pragma warning(disable: 4355)
#pragma warning(disable: 4800)
#pragma warning(disable: 4351)

#endif //_MSC_VER

/**
 * Enable C++11 support in some parts of the code.
 *
 * These parts \em must  also work without C++11, since Wesnoth still uses C++98
 * as the official C++ version.
 *
 * @note Older versions of GCC don't define the proper version for
 * @c __cplusplus,  but have their own test macro. That test is omitted since
 * the amount of support for these compilers depends a lot on the exact
 * compiler version. If you want to enable it for these compilers simply define
 * the macro manually.
 */
#if (__cplusplus >= 201103L)
#define HAVE_CXX11
#endif

#ifdef HAVE_CXX11
#define FINAL final
#define OVERRIDE override
#else
#define FINAL
#define OVERRIDE
#endif

#endif //GLOBAL_HPP_INCLUDED
