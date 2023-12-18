// -*- c-basic-offset: 4 -*-

/** @file pto_merge.cpp
 *
 *  @brief program to merge several project files
 *
 *  @author Thomas Modes
 *
 *  $Id$
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
    std::cout << name << ": merges several project files" << std::endl
         << "pto_merge version " << hugin_utils::GetHuginVersion() << std::endl
         << std::endl
         << "Usage:  " << name << " [options] input.pto input2.pto ..." << std::endl
         << std::endl
         << "  Options:" << std::endl
         << "     -o, --output=file.pto  Output Hugin PTO file." << std::endl
         << "                            Default: <filename>_merge.pto" << std::endl
         << "     -h, --help             Shows this help" << std::endl
         << std::endl;
}

int main(int argc, char* argv[])
{
    // parse arguments
    const char* optstring = "o:h";

    static struct option longOptions[] =
    {
        {"output", required_argument, NULL, 'o' },
        {"help", no_argument, NULL, 'h' },
        0
    };

    int c;
    std::string output;
    while ((c = getopt_long (argc, argv, optstring, longOptions,nullptr)) != -1)
    {
        switch (c)
        {
            case 'o':
                output = optarg;
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

    if (argc - optind < 2)
    {
        std::cerr << hugin_utils::stripPath(argv[0]) << ": at least 2 project files needed" << std::endl << std::endl;
        return 1;
    };

    std::string input=argv[optind];
    // read panorama
    HuginBase::Panorama pano;
    if (!pano.ReadPTOFile(input, hugin_utils::getPathPrefix(input)))
    {
        return 1;
    };

    optind++;
    while(optind<argc)
    {
        HuginBase::Panorama pano2;
        const std::string input2=argv[optind];
        if (!pano2.ReadPTOFile(input2, hugin_utils::getPathPrefix(input2)))
        {
            return 1;
        };
        // read EXIF data, needed for lens detection in merged pano
        for (size_t i = 0; i < pano.getNrOfImages(); ++i)
        {
            HuginBase::SrcPanoImage srcImg(pano.getImage(i));
            srcImg.readEXIF();
            pano.setSrcImage(i, srcImg);
        };
        pano.mergePanorama(pano2);
        optind++;
    };

    //write output
    // Set output .pto filename if not given
    output = hugin_utils::GetOutputFilename(output, input, "merge");
    if (pano.WritePTOFile(output, hugin_utils::getPathPrefix(output)))
    {
        std::cout << std::endl << "Written project file " << output << std::endl;
    };

    return 0;
}
