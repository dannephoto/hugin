// -*- c-basic-offset: 4 -*-

/** @file filesystem.cpp
 *
 *  @brief wrapper around different filesystem headers
 *
 *
 *  @author T. Modes
 *
 */

/*  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This software is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public
 *  License along with this software. If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#ifndef _HUGIN_UTILS_FILESYSTEM_H
#define _HUGIN_UTILS_FILESYSTEM_H
#include "hugin_config.h"
#ifdef HAVE_STD_FILESYSTEM
    #if defined _MSC_VER && _MSC_VER >= 1920
      #define _SILENCE_EXPERIMENTAL_FILESYSTEM_DEPRECATION_WARNING
      #include <experimental/filesystem>
    #else
      #include <filesystem>
    #endif
    #if defined _MSC_VER && _MSC_VER <= 1900
        // MSVC 2015 has implemented in std::tr2::sys
        namespace fs = std::tr2::sys;
        #define OVERWRITE_EXISTING std::tr2::sys::copy_options::overwrite_existing
    #else
        // MSVC 2017 is using experimental namespace
        namespace fs = std::experimental::filesystem;
        #define OVERWRITE_EXISTING std::experimental::filesystem::copy_options::overwrite_existing
    #endif
#else
    // use Boost::Filesystem as fallback
    #define BOOST_FILESYSTEM_VERSION 3
    #ifdef __GNUC__
        #include <boost/version.hpp>
        #if BOOST_VERSION<105700
            #if BOOST_VERSION>=105100
                // workaround a bug in boost filesystem
                // boost filesystem is probably compiled with C++03
                // but Hugin is compiled with C++11, this results in
                // conflicts in boost::filesystems at a enum
                // see https://svn.boost.org/trac/boost/ticket/6779
                #define BOOST_NO_CXX11_SCOPED_ENUMS
            #else
                #define BOOST_NO_SCOPED_ENUMS
            #endif
        #endif
    #endif
    #include <boost/filesystem.hpp>
    namespace fs = boost::filesystem;
    #define OVERWRITE_EXISTING boost::filesystem::copy_option::overwrite_if_exists
#endif
#endif // _HUGIN_UTILS_FILESYSTEM_H
