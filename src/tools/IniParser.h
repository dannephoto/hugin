// -*- c-basic-offset: 4 -*-

/** @file IniParser.h
 *
 *  @brief reads and parse an ini file
 *
 *  @author T. Modes
 *
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

#ifndef INI_PARSER_H
#define INI_PARSER_H

#include <string>
#include <map>
#include <vector>

/** A simple parser for ini files, it implements only some basic features */

class IniParser
{
public:
	/** Reads the given ini file. Append the result to existings values
	 * return 0 if succesful
	 */
	int Read(const std::string& file);
	
	/** Checks if given section/key exists */
	bool HasKey(const std::string& section, const std::string& key) const;
	
	/** returns the value of the given section/key or default value if it does not exists */
	std::string GetKey(const std::string& section, const std::string& key, const std::string& defaultValue) const;
	int GetKey(const std::string& section, const std::string& key, const int defaultValue) const;
	bool GetKey(const std::string& section, const std::string& key, const bool defaultValue) const;

	/** returns a vector of all know sections */
	std::vector<std::string> GetSections() const;
	/** returns a vector of all know keys in given section */
	std::vector<std::string> GetKeys(const std::string& section) const;
	/** for debugging purpose, print all values */
	void PrintValues() const;
private:
	/** stores the key/values pairs for each section */
	typedef std::map<std::string, std::string> IniValues;
	/** map to store the information from different sections */
	std::map<std::string, IniValues> m_iniValues;
};

#endif
