// -*- c-basic-offset: 4 -*-

/** @file IniParser.cpp
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
  
#include "IniParser.h"
#include <iostream>
#include <fstream>
#include "hugin_utils/utils.h"
#include "hugin_utils/stl_utils.h"

int IniParser::Read(const std::string& file)
{
	//open file
	std::ifstream inifile;
	inifile.open(file);

	if (!inifile.is_open())
	{
		return 1;
	};
	std::string line;
	std::string currentSection;
	// go through all lines
	while (std::getline(inifile, line))
	{
		// remove trailing and leading white space and tabs
		line = hugin_utils::StrTrim(line);
		if (line.empty())
		{
			// empty line, skip
			continue;
		}
		// skip comment lines, beginning with ; or #
		if (line[0] == ';' || line[0] == '#')
		{
			continue;
		};
		// new section ?
		if (line.front() == '[' && line.back() == ']')
		{
			currentSection = line.substr(1, line.length() - 2);
			continue;
		};
		// key=value pair ?
		size_t pos = line.find('=');
		// found = and = is not the first character of the line
		if (pos != std::string::npos && pos > 1)
		{
			// extract key and value string
			const std::string key = line.substr(0, pos);
			const std::string value = line.substr(pos + 1);
			if (!currentSection.empty())
			{
				// store for later, updates earlier read values
				m_iniValues[currentSection][key] = value;
			};
		};
	};
	// close file
	inifile.close();
	return 0;
}

bool IniParser::HasKey(const std::string& section, const std::string& key) const
{
	const auto& iniSection = m_iniValues.find(section);
	if (iniSection != m_iniValues.end())
	{
		if (iniSection->second.find(key) != iniSection->second.end())
		{
			return true;
		};
	}
	return false;
}

std::string IniParser::GetKey(const std::string& section, const std::string& key, const std::string& defaultValue) const
{
	const auto& iniSection = m_iniValues.find(section);
	if (iniSection != m_iniValues.end())
	{
		if (iniSection->second.find(key) != iniSection->second.end())
		{
			return iniSection->second.at(key);
		};
	}
	return defaultValue;
}

int IniParser::GetKey(const std::string& section, const std::string& key, const int defaultValue) const
{
	const auto& iniSection = m_iniValues.find(section);
	if (iniSection != m_iniValues.end())
	{
		if (iniSection->second.find(key) != iniSection->second.end())
		{
			int value;
			if (hugin_utils::stringToInt(iniSection->second.at(key), value))
			{
				return value;
			};
		};
	}
	// section/key not found or could not parse number as integer
	return defaultValue;
}

bool IniParser::GetKey(const std::string& section, const std::string& key, const bool defaultValue) const
{
	const auto& iniSection = m_iniValues.find(section);
	if (iniSection != m_iniValues.end())
	{
		if (iniSection->second.find(key) != iniSection->second.end())
		{
			const std::string text = hugin_utils::toupper(iniSection->second.at(key));
			if (text == "TRUE" || text == "1")
			{
				return true;
			}
			else
			{
				if (text == "FALSE" || text == "0")
				{
					return false;
				};
			};
		};
	};
	// section/key not found or could not parse as bool
	return defaultValue;
}

// return vector of all known sections
std::vector<std::string> IniParser::GetSections() const
{
	std::vector<std::string> sections;
	for (const auto& section : m_iniValues)
	{
		sections.push_back(section.first);
	}
	return sections;
}

// return vector of all keys in given section
std::vector<std::string> IniParser::GetKeys(const std::string& section) const
{
	std::vector<std::string> keys;
	const auto iniSection = m_iniValues.find(section);
	if (iniSection != m_iniValues.end())
	{
		for (const auto& key : iniSection->second)
		{
			keys.push_back(key.first);
		};
	};
	return keys;
}

void IniParser::PrintValues() const
{
	for (const auto& section : m_iniValues)
	{
		for (const auto& key : section.second)
		{
			std::cout << section.first << "/" << key.first << "=" << key.second << std::endl;
		};
	};
}
