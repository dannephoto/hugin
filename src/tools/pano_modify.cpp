// -*- c-basic-offset: 4 -*-

/** @file pano_modify.cpp
 *
 *  @brief program to modify some aspects of a project file on the command line
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
#include <cmath>
#include <getopt.h>
#include <panodata/Panorama.h>
#include <algorithms/nona/CenterHorizontally.h>
#include <algorithms/basic/StraightenPanorama.h>
#include <algorithms/basic/RotatePanorama.h>
#include <algorithms/basic/TranslatePanorama.h>
#include <algorithms/nona/FitPanorama.h>
#include <algorithms/basic/CalculateOptimalScale.h>
#include <algorithms/basic/CalculateOptimalROI.h>
#include <algorithms/basic/LayerStacks.h>
#include <algorithms/basic/CalculateMeanExposure.h>
#include "hugin_utils/utils.h"
#include "PanoOptionsFromIni.h"

static void usage(const char* name)
{
    std::cout << name << ": change output parameters of project file" << std::endl
         << "pano_modify version " << hugin_utils::GetHuginVersion() << std::endl
         << std::endl
         << "Usage:  " << name << " [options] input.pto" << std::endl
         << std::endl
         << "  Options:" << std::endl
         << "    -o, --output=file.pto  Output Hugin PTO file. Default: <filename>_mod.pto" << std::endl
         << "    -p, --projection=x     Sets the output projection to number x" << std::endl
         << "    --projection-parameter=x    Sets the parameter of the projection" << std::endl
         << "                                (Several parameters are separated by"<<std::endl
         << "                                space or comma)" << std::endl
         << "    --fov=AUTO|HFOV|HFOVxVFOV   Sets field of view" << std::endl
         << "                                AUTO: calculates optimal fov" << std::endl
         << "                                HFOV|HFOVxVFOV: set to given fov" << std::endl
         << "    -s, --straighten       Straightens the panorama" << std::endl
         << "    -c, --center           Centers the panorama" << std::endl
         << "    --canvas=AUTO|num%|WIDTHxHEIGHT  Sets the output canvas size" << std::endl
         << "                                AUTO: calculate optimal canvas size" << std::endl
         << "                                num%: scales the optimal size by given percent" << std::endl
         << "                                WIDTHxHEIGHT: set to given size" << std::endl
         << "    --crop=AUTO|AUTOHDR|AUTOOUTSIDE|left,right,top,bottom|left,right,top,bottom%" << std::endl
         << "                                Sets the crop rectangle" << std::endl
         << "                                AUTO: autocrop panorama" << std::endl
         << "                                AUTOHDR: autocrop HDR panorama" << std::endl
         << "                                AUTOOUTSIDE: autocrop outside of all images" << std::endl
         << "                                left,right,top,bottom: to given size" << std::endl
         << "                                left,right,top,bottom%: to size relative to canvas" << std::endl
         << "    --output-exposure=AUTO|num  Sets the output exposure value to mean" << std::endl
         << "                                exposure (AUTO) or to given value" << std::endl
         << "                                Prefix number with r for relativ change" << std::endl
         << "    --output-range-compression=num  Set range compression" << std::endl
         << "                                    Value should be a real in range 0..20" << std::endl
         << "    --output-cropped-tiff    Output cropped tiffs as intermediate images" << std::endl
         << "    --output-uncropped-tiff  Output uncropped tiffs as intermediate images" << std::endl
         << "    --output-type=str       Sets the type of output" << std::endl
         << "                              Valid items are" << std::endl
         << "                                NORMAL|N: normal panorama" << std::endl
         << "                                STACKSFUSEDBLENDED|BF: LDR panorama with" << std::endl
         << "                                    blended stacks" << std::endl
         << "                                EXPOSURELAYERSFUSED|FB: LDR panorama with" << std::endl
         << "                                    fused exposure layers (any arrangement)" << std::endl
         << "                                HDR: HDR panorama" << std::endl
         << "                                REMAP: remapped images with corrected exposure" << std::endl
         << "                                REMAPORIG: remapped images with" << std::endl
         << "                                    uncorrected exposure" << std::endl
         << "                                HDRREMAP: remapped images in linear color space" << std::endl
         << "                                FUSEDSTACKS: exposure fused stacks" << std::endl
         << "                                HDRSTACKS: HDR stacks" << std::endl
         << "                                EXPOSURELAYERS: blended exposure layers" << std::endl
         << "                              and separated by a comma." << std::endl
         << "    --ldr-file=JPG|TIF|PNG  Sets the filetype for LDR panorama output" << std::endl
         << "    --ldr-compression=str   Sets the compression for LDR panorama output" << std::endl
         << "                              For TIF: NONE|PACKBITS|LZW|DEFLATE" << std::endl
         << "                              For JPEG: quality as number" << std::endl
         << "    --hdr-file=EXR|TIF      Sets the filetype for HDR panorama output" << std::endl
         << "    --hdr-compression=str   Sets the compression for HDR panorama output" << std::endl
         << "                              For TIF: NONE|PACKBITS|LZW|DEFLATE" << std::endl
         << "    --blender=ENBLEND|INTERNAL  Sets the blender to be used at stitching" << std::endl
         << "                            stage." << std::endl
         << "    --blender-args=str      Sets the arguments for the blender" << std::endl
         << "    --fusion-args=str       Sets the arguments for the fusion program" << std::endl
         << "    --hdrmerge-args=str     Setst the arguments for hdrmerge" << std::endl
         << "    --rotate=yaw,pitch,roll Rotates the whole panorama with the given angles" << std::endl
         << "    --translate=x,y,z       Translate the whole panorama with the given values" << std::endl
         << "    --interpolation=int     Sets the interpolation method" << std::endl
         << "    --ini=FILE              Read the settings from the given FILE and apply it" << std::endl
         << "    --template=TEMPLATE     Read the panorama options from the given TEMPLATE" << std::endl
         << "                            and apply it" << std::endl
         << "    -h, --help             Shows this help" << std::endl
         << std::endl;
}

int main(int argc, char* argv[])
{
    // parse arguments
    const char* optstring = "o:p:sch";

    enum
    {
        SWITCH_FOV=1000,
        SWITCH_CANVAS,
        SWITCH_CROP,
        SWITCH_ROTATE,
        SWITCH_TRANSLATE,
        SWITCH_EXPOSURE,
        SWITCH_RANGE_COMPRESSION,
        SWITCH_OUTPUT_TYPE,
        SWITCH_BLENDER,
        SWITCH_LDRFILETYPE,
        SWITCH_LDRCOMPRESSION,
        SWITCH_HDRFILETYPE,
        SWITCH_HDRCOMPRESSION,
        SWITCH_CROPPED_TIFF,
        SWITCH_UNCROPPED_TIFF,
        SWITCH_BLENDER_ARGS,
        SWITCH_FUSION_ARGS,
        SWITCH_HDRMERGE_ARGS,
        SWITCH_PROJECTION_PARAMETER,
        SWITCH_INTERPOLATION,
        SWITCH_INI_FILE,
        SWITCH_TEMPLATE
    };
    static struct option longOptions[] =
    {
        {"output", required_argument, NULL, 'o' },
        {"projection", required_argument, NULL, 'p' },
        {"projection-parameter", required_argument, NULL, SWITCH_PROJECTION_PARAMETER },
        {"fov", required_argument, NULL, SWITCH_FOV },
        {"straighten", no_argument, NULL, 's' },
        {"center", no_argument, NULL, 'c' },
        {"canvas", required_argument, NULL, SWITCH_CANVAS },
        {"crop", required_argument, NULL, SWITCH_CROP },
        {"output-cropped-tiff", no_argument, NULL, SWITCH_CROPPED_TIFF },
        {"output-uncropped-tiff", no_argument, NULL, SWITCH_UNCROPPED_TIFF },
        {"output-exposure", required_argument, NULL, SWITCH_EXPOSURE },
        {"output-range-compression", required_argument, NULL, SWITCH_RANGE_COMPRESSION },
        {"output-type", required_argument, NULL, SWITCH_OUTPUT_TYPE },
        {"blender", required_argument, NULL, SWITCH_BLENDER },
        {"blender-args", required_argument, NULL, SWITCH_BLENDER_ARGS },
        {"fusion-args", required_argument, NULL, SWITCH_FUSION_ARGS },
        {"hdrmerge-args", required_argument, NULL, SWITCH_HDRMERGE_ARGS },
        {"ldr-file", required_argument,NULL, SWITCH_LDRFILETYPE },
        {"ldr-compression", required_argument, NULL, SWITCH_LDRCOMPRESSION },
        {"hdr-file", required_argument, NULL, SWITCH_HDRFILETYPE },
        {"hdr-compression", required_argument, NULL, SWITCH_HDRCOMPRESSION },
        {"rotate", required_argument, NULL, SWITCH_ROTATE },
        {"translate", required_argument, NULL, SWITCH_TRANSLATE },
        {"interpolation", required_argument, NULL, SWITCH_INTERPOLATION},
        {"ini", required_argument, NULL, SWITCH_INI_FILE },
        {"template", required_argument, NULL, SWITCH_TEMPLATE },
        {"help", no_argument, NULL, 'h' },
        0
    };

    int projection=-1;
    std::vector<double> projParameter;
    double newHFOV=-1;
    double newVFOV=-1;
    int scale=100;
    int newWidth=-1;
    int newHeight=-1;
    int outputCroppedTiff=-1;
    vigra::Rect2D newROI(0,0,0,0);
    hugin_utils::FDiff2D newRoiRelativeLeftTop(-1.0, -1.0);
    hugin_utils::FDiff2D newRoiRelativeRightBottom(-1.0, -1.0);
    bool doFit=false;
    bool doStraighten=false;
    bool doCenter=false;
    bool doOptimalSize=false;
    enum AutoCropMode
    { NONE, AUTO, AUTOHDR, AUTOOUTSIDE};
    AutoCropMode doAutocrop=AutoCropMode::NONE;
    int c;
    double yaw = 0;
    double pitch = 0;
    double roll = 0;
    double x = 0;
    double y = 0;
    double z = 0;
    double outputExposure = -1000;
    double outputRangeCompression = -1;
    bool relativeExposure = false;
    bool calcMeanExposure = false;
    std::string outputType;
    std::string ldrfiletype;
    std::string ldrcompression;
    std::string hdrfiletype;
    std::string hdrcompression;
    std::string output;
    std::string param;
    std::string blender;
#define EMPTYARG "(empty)"
    std::string blenderArgs(EMPTYARG);
    std::string fusionArgs(EMPTYARG);
    std::string hdrMergeArgs(EMPTYARG);
    std::string iniFile;
    std::string templateFile;
    int interpolation = -1;
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
            case 'p':
                //projection
                projection=atoi(optarg);
                if((projection==0) && (strcmp(optarg,"0")!=0))
                {
                    std::cerr << hugin_utils::stripPath(argv[0]) << ": Could not parse projection number." << std::endl;
                    return 1;
                };
                if(projection>=panoProjectionFormatCount())
                {
                    std::cerr << hugin_utils::stripPath(argv[0]) << ": projection " << projection << " is an invalid projection number." << std::endl;
                    return 1;
                };
                break;
            case SWITCH_PROJECTION_PARAMETER:
                // projection parameters
                {
                    // split at space or comma
                    std::vector<std::string> parameter = hugin_utils::SplitString(optarg, ", ");
                    for(const auto& p: parameter)
                    { 
                        double d;
                        if (hugin_utils::stringToDouble(p, d))
                        {
                            projParameter.push_back(d);
                        }
                        else
                        { 
                            std::cerr << hugin_utils::stripPath(argv[0]) << ": Could not parse projection parameters \"" << optarg << "\"." << std::endl;
                            return 1;
                        };
                    };
                }
                break;
            case SWITCH_FOV:
                //field of view
                param=optarg;
                param=hugin_utils::toupper(param);
                if(param=="AUTO")
                {
                    doFit=true;
                }
                else
                {
                    double hfov, vfov;
                    int n=sscanf(optarg, "%lfx%lf", &hfov, &vfov);
                    if(n==1)
                    {
                        if(hfov>0)
                        {
                            newHFOV=hfov;
                        }
                        else
                        {
                            std::cerr << hugin_utils::stripPath(argv[0]) << ": Invalid field of view" << std::endl;
                            return 1;
                        };
                    }
                    else
                    {
                        if (n==2)
                        {
                            if(hfov>0 && vfov>0)
                            {
                                newHFOV=hfov;
                                newVFOV=vfov;
                            }
                            else
                            {
                                std::cerr << hugin_utils::stripPath(argv[0]) << ": Invalid field of view" << std::endl;
                                return 1;
                            };
                        }
                        else
                        {
                            std::cerr << hugin_utils::stripPath(argv[0]) << ": Could not parse field of view" << std::endl;
                            return 1;
                        };
                    };
                };
                break;
            case 's':
                doStraighten=true;
                break;
            case 'c':
                doCenter=true;
                break;
            case SWITCH_CANVAS:
                //canvas size
                param=optarg;
                param=hugin_utils::toupper(param);
                if(param=="AUTO")
                {
                    doOptimalSize=true;
                }
                else
                {
                    int pos=param.find("%");
                    if(pos!=std::string::npos)
                    {
                        param=param.substr(0,pos);
                        scale=atoi(param.c_str());
                        if(scale==0)
                        {
                            std::cerr << hugin_utils::stripPath(argv[0]) << ": No valid scale factor given." << std::endl;
                            return 1;
                        };
                        doOptimalSize=true;
                    }
                    else
                    {
                        int width, height;
                        int n=sscanf(optarg, "%dx%d", &width, &height);
                        if (n==2)
                        {
                            if(width>0 && height>0)
                            {
                                newWidth=width;
                                newHeight=height;
                            }
                            else
                            {
                                std::cerr << hugin_utils::stripPath(argv[0]) << ": Invalid canvas size" << std::endl;
                                return 1;
                            };
                        }
                        else
                        {
                            std::cerr << hugin_utils::stripPath(argv[0]) << ": Could not parse canvas size" << std::endl;
                            return 1;
                        };
                    };
                };
                break;
            case SWITCH_CROP:
                //crop
                param=optarg;
                param=hugin_utils::toupper(param);
                if (param == "AUTO")
                {
                    doAutocrop = AutoCropMode::AUTO;
                }
                else
                {
                    if (param == "AUTOHDR")
                    {
                        doAutocrop = AutoCropMode::AUTOHDR;
                    }
                    else
                    {
                        if (param == "AUTOOUTSIDE")
                        {
                            doAutocrop = AutoCropMode::AUTOOUTSIDE;
                        }
                        else
                        {
                            const int pos = param.find("%");
                            if (pos != std::string::npos)
                            {
                                // parse relative values
                                param = param.substr(0, pos);
                                double left, right, top, bottom;
                                const int n = sscanf(param.c_str(), "%lf,%lf,%lf,%lf", &left, &right, &top, &bottom);
                                if (n == 4)
                                {
                                    if (right > left && bottom > top && left >= 0.0 && top >= 0.0 && right <= 100.0 && bottom <= 100.0)
                                    {
                                        // check values, 0..100 %
                                        newRoiRelativeLeftTop = hugin_utils::FDiff2D(left, top);
                                        newRoiRelativeRightBottom = hugin_utils::FDiff2D(right, bottom);
                                    }
                                    else
                                    {
                                        std::cerr << hugin_utils::stripPath(argv[0]) << ": Invalid crop area" << std::endl;
                                        return 1;
                                    };
                                }
                                else
                                {
                                    std::cerr << hugin_utils::stripPath(argv[0]) << ": Could not parse crop values" << std::endl;
                                    return 1;
                                };
                            }
                            else
                            {
                                int left, right, top, bottom;
                                int n = sscanf(optarg, "%d,%d,%d,%d", &left, &right, &top, &bottom);
                                if (n == 4)
                                {
                                    if (right > left && bottom > top && left >= 0 && top >= 0)
                                    {
                                        newROI.setUpperLeft(vigra::Point2D(left, top));
                                        newROI.setLowerRight(vigra::Point2D(right, bottom));
                                    }
                                    else
                                    {
                                        std::cerr << hugin_utils::stripPath(argv[0]) << ": Invalid crop area" << std::endl;
                                        return 1;
                                    };
                                }
                                else
                                {
                                    std::cerr << hugin_utils::stripPath(argv[0]) << ": Could not parse crop values" << std::endl;
                                    return 1;
                                };
                            };
                        };
                    };
                };
                break;
            case SWITCH_CROPPED_TIFF:
                outputCroppedTiff = 1;
                break;
            case SWITCH_UNCROPPED_TIFF:
                outputCroppedTiff = 0;
                break;
            case SWITCH_EXPOSURE:
                param = optarg;
                param = hugin_utils::toupper(param);
                if (param == "AUTO")
                {
                    calcMeanExposure = true;
                }
                else
                {
                    if (!param.empty())
                    {
                        // if first character is r assume relative exposure values
                        relativeExposure = (param[0] == 'R');
                        if (relativeExposure)
                        {
                            param.erase(0, 1);
                        };
                        int n = sscanf(param.c_str(), "%lf", &outputExposure);
                        if (n != 1)
                        {
                            std::cerr << hugin_utils::stripPath(argv[0]) << ": Could not parse output exposure value." << std::endl;
                            return 1;
                        };
                    }
                    else
                    {
                        std::cerr << hugin_utils::stripPath(argv[0]) << ": Could not parse output exposure value." << std::endl;
                        return 1;
                    };
                };
                break;
            case SWITCH_RANGE_COMPRESSION:
                if(!hugin_utils::stringToDouble(std::string(optarg), outputRangeCompression))
                { 
                    std::cerr << hugin_utils::stripPath(argv[0]) << ": Could not parse output range compression (" << optarg << ")." << std::endl;
                    return 1;
                };
                if (outputRangeCompression < 0.0 || outputRangeCompression > 20.0)
                {
                    std::cerr << hugin_utils::stripPath(argv[0]) << ": range compression must be a real between 0 and 20." << std::endl;
                    return 1;
                };
                break;
            case SWITCH_OUTPUT_TYPE:
                if (!outputType.empty())
                {
                    outputType.append(",");
                };
                outputType.append(optarg);
                break;
            case SWITCH_BLENDER:
                blender = hugin_utils::tolower(hugin_utils::StrTrim(optarg));
                break;
            case SWITCH_LDRFILETYPE:
                ldrfiletype = hugin_utils::tolower(hugin_utils::StrTrim(optarg));
                break;
            case SWITCH_LDRCOMPRESSION:
                ldrcompression = hugin_utils::toupper(hugin_utils::StrTrim(optarg));
                break;
            case SWITCH_HDRFILETYPE:
                hdrfiletype = hugin_utils::tolower(hugin_utils::StrTrim(optarg));
                break;
            case SWITCH_HDRCOMPRESSION:
                hdrcompression = hugin_utils::toupper(hugin_utils::StrTrim(optarg));
                break;
            case SWITCH_BLENDER_ARGS:
                blenderArgs = optarg;
                break;
            case SWITCH_FUSION_ARGS:
                fusionArgs = optarg;
                break;
            case SWITCH_HDRMERGE_ARGS:
                hdrMergeArgs = optarg;
                break;
            case SWITCH_ROTATE:
                {
                    int n=sscanf(optarg, "%lf,%lf,%lf", &yaw, &pitch, &roll);
                    if(n!=3)
                    {
                        std::cerr << hugin_utils::stripPath(argv[0]) << ": Could not parse rotate angles values. Given: \"" << optarg << "\"" << std::endl;
                        return 1;
                    };
                };
                break;
            case SWITCH_TRANSLATE:
                {
                    int n=sscanf(optarg, "%lf,%lf,%lf", &x, &y, &z);
                    if(n!=3)
                    {
                        std::cerr << hugin_utils::stripPath(argv[0]) << ": Could not parse translation values. Given: \"" << optarg << "\"" << std::endl;
                        return 1;
                    };
                };
                break;
            case SWITCH_INTERPOLATION:
                if (!hugin_utils::stringToInt(optarg, interpolation))
                {
                    std::cerr << hugin_utils::stripPath(argv[0]) << ": Could not parse interpolation value. Given: \"" << optarg << "\"" << std::endl;
                    return 1;
                };
                if (interpolation<0 || interpolation>vigra_ext::INTERP_SINC_1024)
                {
                    std::cerr << hugin_utils::stripPath(argv[0]) << ": Interpolator " << interpolation << " is not valid." << std::endl;
                    return 1;
                }
                break;
            case SWITCH_INI_FILE:
                iniFile = optarg;
                if (!hugin_utils::FileExists(iniFile))
                {
                    std::cerr << hugin_utils::stripPath(argv[0]) << ": INI file " << iniFile << " not found." << std::endl;
                    return 1;
                };
                break;
            case SWITCH_TEMPLATE:
                templateFile = optarg;
                if (!hugin_utils::FileExists(templateFile))
                {
                    std::cerr << hugin_utils::stripPath(argv[0]) << ": Template file " << templateFile << " not found." << std::endl;
                    return 1;
                };
                break;
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

    // set some options which depends on each other
    if(doStraighten)
    {
        doCenter=false;
        doFit=true;
    };
    if(doCenter)
    {
        doFit=true;
    };

    std::string input=argv[optind];
    // read panorama
    HuginBase::Panorama pano;
    if (!pano.ReadPTOFile(input, hugin_utils::getPathPrefix(input)))
    {
        return 1;
    };

    // sets the projection
    if(projection!=-1)
    {
        HuginBase::PanoramaOptions opt = pano.getOptions();
        opt.setProjection((HuginBase::PanoramaOptions::ProjectionFormat)projection);
        pano_projection_features proj;
        if (panoProjectionFeaturesQuery(projection, &proj))
        {
            std::cout << "Setting projection to " << proj.name << std::endl;
        }
        pano.setOptions(opt);
    };
    // set projection parameters
    if (!projParameter.empty())
    {
        HuginBase::PanoramaOptions opt = pano.getOptions();
        if (projParameter.size() > opt.m_projFeatures.numberOfParameters)
        {
            std::cout << "Warning: Given " << projParameter.size() << " projection parameters, but projection supports" << std::endl
                << "         only " << opt.m_projFeatures.numberOfParameters << ". Ignoring surplus parameters." << std::endl;
            while (projParameter.size() > opt.m_projFeatures.numberOfParameters)
            {
                projParameter.pop_back();
            }
        }
        if (!projParameter.empty())
        {
            std::vector<double> newProjParameters = opt.getProjectionParameters();
            std::copy(projParameter.begin(), projParameter.end(), newProjParameters.begin());
            opt.setProjectionParameters(newProjParameters);
            // read back, new the limits have been applied
            newProjParameters = opt.getProjectionParameters();
            auto it = newProjParameters.begin();
            std::cout << "Setting projection parameters to: " << *it;
            ++it;
            while (it != newProjParameters.end())
            {
                std::cout << ", " << *it;
                ++it;
            }
            std::cout << std::endl;
            pano.setOptions(opt);
        }
        else
        {
            std::cout << "Warning: Current projection does not support projection parameters." << std::endl;
        }
    }
    // output exposure value
    if (outputExposure > -1000 || calcMeanExposure)
    {
        HuginBase::PanoramaOptions opt = pano.getOptions();
        if (calcMeanExposure)
        {
            opt.outputExposureValue = HuginBase::CalculateMeanExposure::calcMeanExposure(pano);
        }
        else
        {
            if (relativeExposure)
            {
                opt.outputExposureValue += outputExposure;
            }
            else
            {
                opt.outputExposureValue = outputExposure;
            };
        };
        std::cout << "Setting output exposure value to " << opt.outputExposureValue << std::endl;
        pano.setOptions(opt);
    };
    // output range compression
    if (outputRangeCompression >= 0.0)
    {
        HuginBase::PanoramaOptions opt = pano.getOptions();
        opt.outputRangeCompression = outputRangeCompression;
        std::cout << "Setting output range compression to " << opt.outputRangeCompression << std::endl;
        pano.setOptions(opt);
    };
    // output type: normal, fused, hdr pano..
    if (!outputType.empty())
    {
        HuginBase::PanoramaOptions opt = pano.getOptions();
        // reset all output
        // final pano
        opt.outputLDRBlended = false;
        opt.outputLDRExposureBlended = false;
        opt.outputLDRExposureLayersFused = false;
        opt.outputHDRBlended = false;
        // remapped images
        opt.outputLDRLayers = false;
        opt.outputLDRExposureRemapped = false;
        opt.outputHDRLayers = false;
        // stacks
        opt.outputLDRStacks = false;
        opt.outputHDRStacks = false;
        // exposure layers
        opt.outputLDRExposureLayers = false;
        // now parse string and set corresponding options
        std::vector<std::string> tokens = hugin_utils::SplitString(outputType, ",");
        size_t counter = 0;
        for (size_t i = 0; i < tokens.size(); i++)
        {
            std::string s = hugin_utils::toupper(hugin_utils::StrTrim(tokens[i]));
            if (s == "NORMAL" || s=="N")
            {
                opt.outputLDRBlended = true;
                std::cout << "Activate output of normal panorama." << std::endl;
                counter++;
                continue;
            };
            if (s == "STACKSFUSEDBLENDED" || s == "FB")
            {
                opt.outputLDRExposureBlended = true;
                std::cout << "Activate output of LDR panorama: Exposure fused from stacks." << std::endl;
                counter++;
                continue;
            };
            if (s == "EXPOSURELAYERSFUSED" || s == "BF")
            {
                opt.outputLDRExposureLayersFused = true;
                std::cout << "Activate output of LDR panorama: Exposure fused from any arrangement." << std::endl;
                counter++;
                continue;
            };
            if (s == "HDR")
            {
                opt.outputHDRBlended = true;
                std::cout << "Activate output of hdr panorama." << std::endl;
                counter++;
                continue;
            };
            // single remapped images
            if (s == "REMAP")
            {
                opt.outputLDRLayers = true;
                std::cout << "Activate output of remapped, exposure corrected images." << std::endl;
                counter++;
                continue;
            };
            if (s == "REMAPORIG")
            {
                opt.outputLDRExposureRemapped = true;
                std::cout << "Activate output of remapped images with unmodified exposure." << std::endl;
                counter++;
                continue;
            };
            if (s == "HDRREMAP")
            {
                opt.outputHDRLayers = true;
                std::cout << "Activate output of remapped hdr images." << std::endl;
                counter++;
                continue;
            };
            //stacks
            if (s == "FUSEDSTACKS")
            {
                opt.outputLDRStacks = true;
                std::cout << "Activate output of exposure fused stacks." << std::endl;
                counter++;
                continue;
            };
            if (s == "HDRSTACKS")
            {
                opt.outputHDRStacks = true;
                std::cout << "Activate output of HDR stacks." << std::endl;
                counter++;
                continue;
            };
            //exposure layers
            if (s == "EXPOSURELAYERS")
            {
                opt.outputLDRExposureLayers = true;
                std::cout << "Activate output of exposure layers." << std::endl;
                counter++;
                continue;
            };
            std::cout << "Unknown parameter \"" << s << "\" found in --output-type." << std::endl
                << "Ignoring this parameter." << std::endl;
        }
        if (counter > 0)
        {
            pano.setOptions(opt);
        }
        else
        {
            std::cout << "No matching output type given. The whole output-type is ignored." << std::endl;
        };
    };
    // cropped or uncropped tiff output
    if (outputCroppedTiff != -1)
    {
        HuginBase::PanoramaOptions opts = pano.getOptions();
        opts.tiff_saveROI = (outputCroppedTiff == 1);
        std::cout << "Setting support for cropped images: " << ((outputCroppedTiff == 1) ? "yes" : "no") << std::endl;
        pano.setOptions(opts);
    }
    // blender type
    if (!blender.empty())
    {
        HuginBase::PanoramaOptions opt = pano.getOptions();
        if (blender == "enblend")
        {
            opt.blendMode = HuginBase::PanoramaOptions::ENBLEND_BLEND;
            std::cout << "Setting blender type to \"ENBLEND\"." << std::endl;
        }
        else
        {
            if (blender == "internal" || blender == "verdandi")
            {
                opt.blendMode = HuginBase::PanoramaOptions::INTERNAL_BLEND;
                std::cout << "Setting blender type to \"INTERNAL\"." << std::endl;
            }
            else
            {
                std::cout << "Blender \"" << blender << "\" is not a valid blender ." << std::endl
                    << "Ignoring parameter." << std::endl;
            };
        };
        pano.setOptions(opt);
    }
    if (blenderArgs != EMPTYARG)
    {
        HuginBase::PanoramaOptions opt = pano.getOptions();
        switch (opt.blendMode)
        {
            case HuginBase::PanoramaOptions::ENBLEND_BLEND:
                opt.enblendOptions = blenderArgs;
                std::cout << "Setting enblend arguments to " << blenderArgs << std::endl;
                break;
            case HuginBase::PanoramaOptions::INTERNAL_BLEND:
                opt.verdandiOptions = blenderArgs;
                std::cout << "Setting verdandi arguments to " << blenderArgs << std::endl;
                break;
            default:
                std::cout << "Unknown blender in pto file." << std::endl;
                break;
        };
        pano.setOptions(opt);
    };
    if (fusionArgs != EMPTYARG)
    {
        HuginBase::PanoramaOptions opt = pano.getOptions();
        opt.enfuseOptions = fusionArgs;
        std::cout << "Setting enfuse arguments to " << fusionArgs << std::endl;
        pano.setOptions(opt);
    };
    if (hdrMergeArgs != EMPTYARG)
    {
        HuginBase::PanoramaOptions opt = pano.getOptions();
        opt.hdrmergeOptions = hdrMergeArgs;
        std::cout << "Setting hugin_hdrdmerge arguments to " << hdrMergeArgs << std::endl;
        pano.setOptions(opt);
    };
    // ldr output file type
    if (!ldrfiletype.empty())
    {
        HuginBase::PanoramaOptions opt = pano.getOptions();
        if (ldrfiletype == "jpg" || ldrfiletype == "png" || ldrfiletype == "tif")
        {
            opt.outputImageType = ldrfiletype;
            std::cout << "Setting ldr output to filetype \"" << ldrfiletype << "\"." << std::endl;
            pano.setOptions(opt);
        }
        else
        {
            std::cout << "LDR file format \"" << ldrfiletype << "\" is not a valid LDR output filetype." << std::endl
                << "Ignoring parameter." << std::endl;
        };
    };
    // ldr compression
    if (!ldrcompression.empty())
    {
        HuginBase::PanoramaOptions opt = pano.getOptions();
        if (opt.outputImageType == "tif")
        {
            if (ldrcompression == "NONE" || ldrcompression == "PACKBITS" || ldrcompression == "LZW" || ldrcompression == "DEFLATE")
            {
                opt.outputImageTypeCompression = ldrcompression;
                std::cout << "Setting TIF compression to \"" << ldrcompression << "\"." << std::endl;
                opt.tiffCompression = ldrcompression;
            }
            else
            {
                std::cout << "LDR compression \"" << ldrcompression << "\" is not a valid compression value for TIF files." << std::endl
                    << "Ignoring compression." << std::endl;
            }
        }
        else
        {
            if (opt.outputImageType == "jpg")
            {
                int quality = 0;
                quality = atoi(ldrcompression.c_str());
                if (quality != 0)
                {
                    if (quality>0 && quality <=100)
                    { 
                        opt.quality = quality;
                        std::cout << "Setting JPEG quality to " << quality << "." << std::endl;
                    }
                    else
                    {
                        std::cout << "Given value for JPEG quality is outside the valid range 1..100." << std::endl
                            << "Ignoring value." << std::endl;
                    };
                }
                else
                {
                    std::cout << "Could not parse \"" << ldrcompression << "\" as a valid JPEG quality number." << std::endl
                        << "Ignoring value." << std::endl;
                };
            }
            else
            {
                if (opt.outputImageType == "png")
                {
                    std::cout << "Setting compression for PNG images is not supported." << std::endl;
                }
                else
                {
                    // this should never happen
                    std::cout << "Unknown image format" << std::endl;
                };
            };
        };
        pano.setOptions(opt);
    };
    // hdr output file type
    if (!hdrfiletype.empty())
    {
        HuginBase::PanoramaOptions opt = pano.getOptions();
        if (hdrfiletype == "exr" || hdrfiletype == "tif")
        {
            opt.outputImageTypeHDR = hdrfiletype;
            std::cout << "Setting hdr output to filetype \"" << hdrfiletype << "\"." << std::endl;
            pano.setOptions(opt);
        }
        else
        {
            std::cout << "HDR file format \"" << hdrfiletype << "\" is not a valid HDR output filetype." << std::endl
                << "Ignoring parameter." << std::endl;
        };
    };
    // hdr compression
    if (!hdrcompression.empty())
    {
        HuginBase::PanoramaOptions opt = pano.getOptions();
        if (opt.outputImageTypeHDR == "tif")
        {
            if (hdrcompression == "NONE" || hdrcompression == "PACKBITS" || hdrcompression == "LZW" || hdrcompression == "DEFLATE")
            {
                opt.outputImageTypeHDRCompression = hdrcompression;
                std::cout << "Setting HDR-TIF compression to \"" << hdrcompression << "\"." << std::endl;
            }
            else
            {
                std::cout << "HDR compression \"" << hdrcompression << "\" is not a valid compression value for TIF files." << std::endl
                    << "Ignoring compression." << std::endl;
            }
        }
        else
        {
            if (opt.outputImageTypeHDR == "exr")
            {
                std::cout << "Setting compression for EXR images is not supported." << std::endl;
            }
            else
            {
                // this should never happen
                std::cout << "Unknown HDR image format" << std::endl;
            };
        };
        pano.setOptions(opt);
    };
    // rotate complete pano
    if (std::abs(yaw) + std::abs(pitch) + std::abs(roll) > 0.0)
    {
        std::cout << "Rotate panorama (yaw=" << yaw << ", pitch= " << pitch << ", roll=" << roll << ")" << std::endl;
        HuginBase::RotatePanorama(pano, yaw, pitch, roll).run();
    };
    // translate complete pano
    if(std::abs(x) + std::abs(y) + std::abs(z) > 0.0)
    {
        std::cout << "Translate panorama (x=" << x << ", y=" << y << ", z=" << z << ")" << std::endl;
        HuginBase::TranslatePanorama(pano, x, y, z).run();
    };
    // straighten
    if(doStraighten)
    {
        std::cout << "Straighten panorama" << std::endl;
        HuginBase::StraightenPanorama(pano).run();
        HuginBase::CenterHorizontally(pano).run();
    };
    // center
    if(doCenter)
    {
        std::cout << "Center panorama" << std::endl;
        HuginBase::CenterHorizontally(pano).run();
    }
    //fit fov
    if(doFit)
    {
        std::cout << "Fit panorama field of view to best size" << std::endl;
        HuginBase::PanoramaOptions opt = pano.getOptions();
        HuginBase::CalculateFitPanorama fitPano(pano);
        fitPano.run();
        opt.setHFOV(fitPano.getResultHorizontalFOV());
        opt.setHeight(hugin_utils::roundi(fitPano.getResultHeight()));
        std::cout << "Setting field of view to " << opt.getHFOV() << " x " << opt.getVFOV() << std::endl;
        pano.setOptions(opt);
    };
    //set field of view manually
    if(newHFOV>0)
    {
        HuginBase::PanoramaOptions opt = pano.getOptions();
        opt.setHFOV(newHFOV);
        if(opt.fovCalcSupported(opt.getProjection()) && newVFOV>0)
        {
            opt.setVFOV(newVFOV);
        }
        std::cout << "Setting field of view to " << opt.getHFOV() << " x " << opt.getVFOV() << std::endl;
        pano.setOptions(opt);
    };
    // calc optimal size
    if(doOptimalSize)
    {
        std::cout << "Calculate optimal size of panorama" << std::endl;
        double s = HuginBase::CalculateOptimalScale::calcOptimalScale(pano);
        HuginBase::PanoramaOptions opt = pano.getOptions();
        opt.setWidth(hugin_utils::roundi(opt.getWidth()*s*scale/100), true);
        std::cout << "Setting canvas size to " << opt.getWidth() << " x " << opt.getHeight() << std::endl;
        pano.setOptions(opt);
    };
    // set canvas size
    if(newWidth>0 && newHeight>0)
    {
        HuginBase::PanoramaOptions opt = pano.getOptions();
        opt.setWidth(newWidth);
        opt.setHeight(newHeight);
        std::cout << "Setting canvas size to " << opt.getWidth() << " x " << opt.getHeight() << std::endl;
        pano.setOptions(opt);
    };
    // auto crop
    if (doAutocrop != AutoCropMode::NONE)
    {
        std::cout << "Searching for best crop rectangle" << std::endl;
        AppBase::DummyProgressDisplay dummy;
        vigra::Rect2D roi;
        if (doAutocrop == AutoCropMode::AUTOOUTSIDE)
        {
            HuginBase::CalculateOptimalROIOutside cropPano(pano, &dummy);
            cropPano.run();
            roi = cropPano.getResultOptimalROI();
        }
        else
        {
            HuginBase::CalculateOptimalROI cropPano(pano, &dummy);
            if (doAutocrop == AutoCropMode::AUTOHDR)
            {
                cropPano.setStacks(getHDRStacks(pano, pano.getActiveImages(), pano.getOptions()));
            }
            cropPano.run();
            roi = cropPano.getResultOptimalROI();
        };
        HuginBase::PanoramaOptions opt = pano.getOptions();
        //set the ROI - fail if the right/bottom is zero, meaning all zero
        if(!roi.isEmpty())
        {
            opt.setROI(roi);
            std::cout << "Set crop size to " << roi.left() << "," << roi.top() << "," << roi.right() << "," << roi.bottom() << std::endl;
            pano.setOptions(opt);
        }
        else
        {
            std::cout << "Could not find best crop rectangle" << std::endl;
        }
    };
    //setting crop rectangle manually
    if(newROI.right() != 0 && newROI.bottom() != 0)
    {
        HuginBase::PanoramaOptions opt = pano.getOptions();
        opt.setROI(newROI);
        std::cout << "Set crop size to " << opt.getROI().left() << "," << opt.getROI().right() << "," << opt.getROI().top() << "," << opt.getROI().bottom() << std::endl;
        pano.setOptions(opt);
    };
    if (newRoiRelativeLeftTop.x > -1 && newRoiRelativeRightBottom.x > -1)
    {
        HuginBase::PanoramaOptions opt = pano.getOptions();
        opt.setROI(vigra::Rect2D(
            opt.getWidth() * newRoiRelativeLeftTop.x / 100.0, 
            opt.getHeight() * newRoiRelativeLeftTop.y / 100.0,
            opt.getWidth() * newRoiRelativeRightBottom.x / 100.0,
            opt.getHeight() * newRoiRelativeRightBottom.y / 100.0
        ));
        std::cout << "Set crop size to " << opt.getROI().left() << "," << opt.getROI().right() << "," << opt.getROI().top() << "," << opt.getROI().bottom() << std::endl;
        pano.setOptions(opt);
    }
    //setting interpolation method
    if (interpolation>=0)
    {
        HuginBase::PanoramaOptions opt = pano.getOptions();
        opt.interpolator = static_cast<vigra_ext::Interpolator>(interpolation);
        std::cout << "Set interpolation method to " << interpolation << std::endl;
        pano.setOptions(opt);
    };
    // read settings from ini file and apply it
    if (!iniFile.empty())
    {
        std::cout << "Reading panorama options from " << iniFile << std::endl;
        const HuginBase::PanoramaOptions options = ReadPanoramaOptionsFromIni(iniFile, pano);
        pano.setOptions(options);
    };
    // read settings from other pto file and apply only PanoramaOptions
    if (!templateFile.empty())
    {
        // read template panorama
        HuginBase::Panorama templatePano;
        if (!pano.ReadPTOFile(templateFile, hugin_utils::getPathPrefix(templateFile)))
        {
            return 1;
        };
        std::cout << "Applying settings from " << templateFile << std::endl;
        pano.setOptions(templatePano.getOptions());
    };
    //write output
    // Set output .pto filename if not given
    output = hugin_utils::GetOutputFilename(output, input, "mod");
    if (pano.WritePTOFile(output, hugin_utils::getPathPrefix(output)))
    {
        std::cout << std::endl << "Written output to " << output << std::endl;
    };

    return 0;
}
