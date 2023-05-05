
/**
 * Implementation of basic routines for deghosting algorithms
 * Copyright (C) 2009  Lukáš Jirkovský <l.jirkovsky@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 *Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include "deghosting.h"

namespace deghosting {
    
    void Deghosting::loadImages(std::vector<std::string>& newInputFiles) THROWNOIMAGES
    {
        if (newInputFiles.empty())
            throw NoImages();
        const vigra::ImageImportInfo firstInfo(newInputFiles[0].c_str());
        const vigra::Rect2D inputRect(vigra::Point2D(firstInfo.getPosition()), firstInfo.size());
        m_inputROI.push_back(inputRect);
        inputFiles.push_back(firstInfo);
        m_outputROI = inputRect;
        for (unsigned int i = 1; i< newInputFiles.size(); i++) {
            vigra::ImageImportInfo tmpInfo(newInputFiles[i].c_str());
            const vigra::Rect2D inputRect(vigra::Point2D(tmpInfo.getPosition()), tmpInfo.size());
            m_inputROI.push_back(inputRect);
            inputFiles.push_back(tmpInfo);
            m_outputROI |= inputRect;
        }
     }
    
    void Deghosting::setFlags(const uint16_t newFlags) {
        flags = newFlags;
    }

    void Deghosting::setDebugFlags(const uint16_t newFlags) {
        debugFlags = newFlags;
    }

    void Deghosting::setIterationNum(const int newIterations) {
        iterations = newIterations;
    }

    void Deghosting::setCameraResponse(EMoR newResponse) {
        response = newResponse;
    }

    void Deghosting::setVerbosity(int newVerbosity) {
        verbosity = newVerbosity;
    }

    vigra::Rect2D Deghosting::getOutputROI() const
    {
        return m_outputROI;
    }

    std::vector<vigra::Rect2D> Deghosting::getInputROIs() const
    {
        return m_inputROI;
    }
}

