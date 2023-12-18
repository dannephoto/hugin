// -*- c-basic-offset: 4 -*-

/** @file pto_mask.cpp
 *
 *  @brief program to set mask from command line
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

struct MaskFiles
{
    size_t imageNr;
    std::string maskFile;
};

static void usage(const char* name)
{
    std::cout << name << ": add mask to pto project" << std::endl
         << name << " version " << hugin_utils::GetHuginVersion() << std::endl
         << std::endl
         << "Usage:  " << name << " [options] input.pto" << std::endl
         << std::endl
         << "  Options:" << std::endl
         << "     -o, --output=file.pto  Output Hugin PTO file. Default: <filename>_mask.pto" << std::endl
         << "     --mask=filename@imgNr  Read the mask from the file and" << std::endl
         << "                            assign the mask to given image" << std::endl
         << "     --rotate=CLOCKWISE|90|COUNTERCLOCKWISE|-90" << std::endl
         << "                            Rotates the mask clock- or counterclockwise" << std::endl
         << "     --process==CLIP|SCALE|PROP_SCALE   Specify how the mask should be modified" << std::endl
         << "                            if the mask is create for an image with" << std::endl
         << "                            different size." << std::endl
         << "                            * CLIP: clipping (Default)" << std::endl
         << "                            * SCALE: Scaling width and height individually" << std::endl
         << "                            * PROP_SCALE: Proportional scale" << std::endl
         << "     --delete-mask=maskNr@imgNr|ALL@imgNr|ALL" << std::endl
         << "                            Removes the specified mask(s)" << std::endl
         << "     -h, --help             Shows this help" << std::endl
         << std::endl;
}

int main(int argc, char* argv[])
{
    // parse arguments
    const char* optstring = "o:h";
    enum
    {
        MASK_SWITCH=1000,
        ROTATE_SWITCH,
        PROC_SWITCH,
        MASK_DELETE_SWITCH
    };
    static struct option longOptions[] =
    {
        {"output", required_argument, NULL, 'o' },
        {"mask", required_argument, NULL, MASK_SWITCH },
        {"rotate", required_argument, NULL, ROTATE_SWITCH},
        {"process", required_argument, NULL, PROC_SWITCH},
        {"delete-mask", required_argument, NULL, MASK_DELETE_SWITCH},
        {"help", no_argument, NULL, 'h' },
        0
    };

    int c;
    std::vector<MaskFiles> maskFiles;
    size_t rotate=0;
    size_t process=0;
    std::string output;
    // boolean if all masks should be delete
    bool deleteAllMasks = false;
    // map, which contains all masks which should be deleted
    // key is the image number, value is a set with the mask numbers
    // UINT_MAX indicates all masks for given image
    std::map<unsigned int, HuginBase::UIntSet> masksToDelete;
    while ((c = getopt_long (argc, argv, optstring, longOptions,nullptr)) != -1)
    {
        switch (c)
        {
            case 'o':
                output = optarg;
                break;
            case MASK_SWITCH:
                {
                    std::string s=optarg;
                    size_t found=s.rfind('@');
                    MaskFiles mf;
                    if(found!=std::string::npos)
                    {
                        std::string s2=s.substr(found+1, std::string::npos);
                        mf.imageNr=atoi(s2.c_str());
                        if(mf.imageNr==0 && s2!="0")
                        {
                            std::cerr << hugin_utils::stripPath(argv[0]) << ": Could not parse image number: \"" << s2 << "\"." << std::endl;
                            return 1;
                        };
                    }
                    else
                    {
                        std::cerr << hugin_utils::stripPath(argv[0]) << ": No image number found in \"" << s << "\"." << std::endl;
                        return 1;
                    };
                    mf.maskFile=s.substr(0, found);
                    if(!hugin_utils::FileExists(mf.maskFile))
                    {
                        std::cerr << hugin_utils::stripPath(argv[0]) << ": File \"" << mf.maskFile << "\" does not exists." << std::endl;
                        return 1;
                    };
                    maskFiles.push_back(mf);
                };
                break;
            case ROTATE_SWITCH:
                {
                    std::string s=optarg;
                    s=hugin_utils::toupper(s);
                    if(s=="CLOCKWISE" || s=="90")
                    {
                        rotate=1;
                    }
                    else
                    {
                        if(s=="COUNTERCLOCKWISE" || s=="-90")
                        {
                            rotate=2;
                        }
                        else
                        {
                            std::cerr << hugin_utils::stripPath(argv[0]) << ": Unknown rotate command (" << optarg << ") found." << std::endl;
                            return 1;
                        };
                    };
                };
                break;
            case PROC_SWITCH:
                {
                    std::string s=optarg;
                    s=hugin_utils::toupper(s);
                    if(s=="CLIP")
                    {
                        process=0;
                    }
                    else
                    {
                        if(s=="SCALE")
                        {
                            process=1;
                        }
                        else
                        {
                            if(s=="PROP_SCALE")
                            {
                                process=2;
                            }
                            else
                            {
                                std::cerr << hugin_utils::stripPath(argv[0]) << ": Unknown process command (" << optarg << ") found." << std::endl;
                                return 1;
                            };
                        };
                    };
                }
                break;
            case MASK_DELETE_SWITCH:
                {
                    std::string s = hugin_utils::tolower(optarg);
                    if (s == "all")
                    {
                        deleteAllMasks = true;
                    }
                    else
                    {
                        size_t found = s.rfind('@');
                        if (found != std::string::npos)
                        {
                            // parse mask number with special case for all
                            std::string s2 = s.substr(0, found);
                            unsigned int maskNr;
                            if (s2 == "all")
                            {
                                maskNr = UINT_MAX;
                            }
                            else
                            {
                                if (!hugin_utils::stringToUInt(s2, maskNr))
                                {
                                    std::cerr << hugin_utils::stripPath(argv[0]) << ": Could not parse mask number: \"" << s2 << "\"." << std::endl;
                                    return 1;
                                };
                            };
                            // now parse imgNr
                            s2 = s.substr(found + 1, std::string::npos);
                            unsigned int imgNr;
                            if (!hugin_utils::stringToUInt(s2, imgNr))
                            {
                                std::cerr << hugin_utils::stripPath(argv[0]) << ": Could not parse image number: \"" << s2 << "\"." << std::endl;
                                return 1;
                            };
                            // now remember parsed imgNr and maskNr
                            masksToDelete[imgNr].insert(maskNr);
                        }
                        else
                        {
                            std::cerr << hugin_utils::stripPath(argv[0]) << ": No image number found in \"" << s << "\"." << std::endl;
                            return 1;
                        };
                };
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

    if (maskFiles.empty() && !deleteAllMasks && masksToDelete.empty())
    {
        std::cerr << hugin_utils::stripPath(argv[0]) << ": No changes to mask given." << std::endl << std::endl;
        return 1;
    };

    std::string input=argv[optind];
    // read panorama
    HuginBase::Panorama pano;
    if (!pano.ReadPTOFile(input, hugin_utils::getPathPrefix(input)))
    {
        return 1;
    };

    //delete masks
    if (deleteAllMasks)
    {
        // delete all masks
        std::cout << "Deleting all masks in project file" << std::endl;
        for (size_t i = 0; i < pano.getNrOfImages(); ++i)
        {
            pano.updateMasksForImage(i, HuginBase::MaskPolygonVector());
        };
    }
    else
    {
        if (!masksToDelete.empty())
        {
            for (auto it = masksToDelete.begin(); it != masksToDelete.end(); ++it)
            {
                // get image number and check that it is valid
                const unsigned int imgNr = it->first;
                if (imgNr < pano.getNrOfImages())
                {
                    HuginBase::MaskPolygonVector masks = pano.getImage(imgNr).getMasks();
                    const HuginBase::UIntSet& masksSet = it->second;
                    if (*masksSet.rbegin() == UINT_MAX)
                    {
                        // delete all masks for given image
                        masks = HuginBase::MaskPolygonVector();
                        std::cout << "Deleting all masks for image " << imgNr << std::endl;
                    }
                    else
                    {
                        // iterate all masks
                        for (auto maskNr = masksSet.rbegin(); maskNr != masksSet.rend(); ++maskNr)
                        {
                            // range check
                            if (*maskNr < masks.size())
                            {
                                masks.erase(masks.begin() + (*maskNr));
                                std::cout << "Deleting mask " << *maskNr << " for image " << imgNr << std::endl;
                            }
                            else
                            {
                                std::cout << "Ignoring deleting non-existing mask number " << *maskNr << " for image " << imgNr << std::endl;
                            };
                        };
                    };
                    // finally update mask vector im panorama
                    pano.updateMasksForImage(imgNr, masks);
                }
                else
                {
                    std::cout << "Ignoring deleting masks for invalid image nr " << imgNr << std::endl;
                };
            };
        };
    };
    //read masks and apply
    for(size_t i=0; i<maskFiles.size(); i++)
    {
        if(maskFiles[i].imageNr<pano.getNrOfImages())
        {
            std::ifstream in(maskFiles[i].maskFile.c_str());
            vigra::Size2D maskImageSize;
            HuginBase::MaskPolygonVector loadedMasks;
            LoadMaskFromStream(in, maskImageSize, loadedMasks, maskFiles[i].imageNr);
            in.close();
            if(maskImageSize.area()==0 || loadedMasks.empty())
            {
                std::cerr << "Error: Could not parse mask from file \"" << maskFiles[i].maskFile << "\"." << std::endl;
                return 1;
            };
            double maskWidth;
            double maskHeight;
            if(rotate==0)
            {
                maskWidth=maskImageSize.width();
                maskHeight=maskImageSize.height();
            }
            else
            {
                maskWidth=maskImageSize.height();
                maskHeight=maskImageSize.width();
                bool clockwise=(rotate==1);
                for(unsigned int i=0; i<loadedMasks.size(); i++)
                {
                    loadedMasks[i].rotate90(clockwise, maskImageSize.width(), maskImageSize.height());
                }
            };
            // compare image size from file with that of current image alert user
            // if different.
            vigra::Size2D imageSize=pano.getImage(maskFiles[i].imageNr).getSize();
            if (maskImageSize != imageSize)
            {
                switch(process)
                {
                    case 0:
                        // clip mask
                        std::cout << "Clipping mask" << std::endl;
                        for(unsigned int i=0; i<loadedMasks.size(); i++)
                            loadedMasks[i].clipPolygon(vigra::Rect2D(-0.5*HuginBase::maskOffset, -0.5*HuginBase::maskOffset,
                                                       imageSize.width()+0.5*HuginBase::maskOffset, imageSize.height()+0.5*HuginBase::maskOffset));
                        break;
                    case 1:
                        // scale mask
                        std::cout << "Scaling mask" << std::endl;
                        for(unsigned int i=0; i<loadedMasks.size(); i++)
                        {
                            loadedMasks[i].scale((double)imageSize.width()/maskWidth,(double)imageSize.height()/maskHeight);
                        }
                        break;
                    case 2:
                        // proportional scale mask
                        std::cout << "Propotional scale mask" << std::endl;
                        {
                            double factor=std::min((double)imageSize.width()/maskWidth, (double)imageSize.height()/maskHeight);
                            for(unsigned int i=0; i<loadedMasks.size(); i++)
                            {
                                loadedMasks[i].scale(factor);
                            }
                        };
                        break;
                };
            };
            HuginBase::MaskPolygonVector masks = pano.getImage(maskFiles[i].imageNr).getMasks();
            for(size_t j=0; j<loadedMasks.size(); j++)
            {
                masks.push_back(loadedMasks[j]);
            };
            pano.updateMasksForImage(maskFiles[i].imageNr, masks);
        }
        else
        {
            std::cout << "Warning: Invalid image number \"" << maskFiles[i].imageNr << "\"." << std::endl
                 << "         Project contains only " << pano.getNrOfImages()+1 << " images." << std::endl
                 << "         Ignoring this mask." << std::endl;
        };
    };

    //write output
    // Set output .pto filename if not given
    output = hugin_utils::GetOutputFilename(output, input, "mask");
    if (pano.WritePTOFile(output, hugin_utils::getPathPrefix(output)))
    {
        std::cout << std::endl << "Written project file " << output << std::endl;
    };
    return 0;
}
