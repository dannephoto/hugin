// -*- c-basic-offset: 4 -*-

/** @file pto_template.cpp
 *
 *  @brief program to apply template
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

#include <fstream>
#include <sstream>
#include <getopt.h>
#include <panodata/Panorama.h>

static void usage(const char* name)
{
    std::cout << name << ": apply template" << std::endl
         << name << " version " << hugin_utils::GetHuginVersion() << std::endl
         << std::endl
         << "Usage:  " << name << " [options] input.pto" << std::endl
         << std::endl
         << "  Options:" << std::endl
         << "     -o, --output=file.pto  Output Hugin PTO file." << std::endl
         << "                            Default: <filename>_template.pto" << std::endl
         << "     --template=template.pto   Apply the given template file" << std::endl
         << "     -h, --help             Shows this help" << std::endl
         << std::endl;
}

int main(int argc, char* argv[])
{
    // parse arguments
    const char* optstring = "o:t:h";
    enum
    {
        MINOVERLAP=1000
    };

    static struct option longOptions[] =
    {
        {"output", required_argument, NULL, 'o' },
        {"template", required_argument, NULL, 't'},
        {"help", no_argument, NULL, 'h' },
        0
    };

    int c;
    std::string output;
    std::string templateFile;
    while ((c = getopt_long (argc, argv, optstring, longOptions,nullptr)) != -1)
    {
        switch (c)
        {
            case 'o':
                output = optarg;
                break;
            case 't':
                templateFile = optarg;
                if(!hugin_utils::FileExists(templateFile))
                {
                    std::cerr << hugin_utils::stripPath(argv[0]) << ": Template \"" << templateFile << "\" not found." << std::endl;
                    return 1;
                };
                break;
            case 'h':
                usage(hugin_utils::stripPath(argv[0]).c_str());
                return 0;
            case ':':
            case '?':
                // missing argument or invalid switch
                return 1;
                break;
            default:
                // this should not happen
                abort();
        }
    }

    if (argc - optind != 1)
    {
        if (argc - optind < 1)
        {
            std::cerr << hugin_utils::stripPath(argv[0]) << ": No project file given." << std::endl;
        }
        else
        {
            std::cerr << hugin_utils::stripPath(argv[0]) << ": Only one project file expected." << std::endl;
        };
        return 1;
    };
    if (templateFile.empty())
    {
        std::cerr << hugin_utils::stripPath(argv[0]) << ": No template given." << std::endl;
        return 1;
    };

    std::string input=argv[optind];
    // read panorama
    HuginBase::Panorama pano;
    if (!pano.ReadPTOFile(input, hugin_utils::getPathPrefix(input)))
    {
        return 1;
    };

    HuginBase::Panorama newPano;
    if (!newPano.ReadPTOFile(templateFile, hugin_utils::getPathPrefix(templateFile)))
    {
        return 1;
    };

    if (pano.getNrOfImages() != newPano.getNrOfImages())
    {
        std::cerr << "Error: template expects " << newPano.getNrOfImages() << " images," << std::endl
             << "       current project contains " << pano.getNrOfImages() << " images" << std::endl
             << "       Could not apply template" << std::endl;
        return false;
    }

    // check image sizes, and correct parameters if required.
    HuginBase::VariableMapVector vars(newPano.getNrOfImages());
    for (unsigned int i = 0; i < newPano.getNrOfImages(); i++)
    {
        // check if image size is correct
        const HuginBase::SrcPanoImage& oldSrcImg = pano.getImage(i);
        HuginBase::SrcPanoImage newSrcImg = newPano.getSrcImage(i);

        // just keep the file name
        newSrcImg.setFilename(oldSrcImg.getFilename());
        if (oldSrcImg.getSize() != newSrcImg.getSize())
        {
            // adjust size properly.
            newSrcImg.resize(oldSrcImg.getSize(), &vars[i]);
        }
        newPano.setSrcImage(i, newSrcImg);
    }
    // now update all possible linked variables
    for (unsigned int i = 0; i < newPano.getNrOfImages(); ++i)
    {
        if (!vars[i].empty())
        {
            newPano.updateVariables(i, vars[i]);
        };
    };

    // keep old control points.
    newPano.setCtrlPoints(pano.getCtrlPoints());

    //write output
    // Set output .pto filename if not given
    output = hugin_utils::GetOutputFilename(output, input, "template");
    if (newPano.WritePTOFile(output, hugin_utils::getPathPrefix(output)))
    {
        std::cout << std::endl << "Written project file " << output << std::endl;
    };
    return 0;
}
