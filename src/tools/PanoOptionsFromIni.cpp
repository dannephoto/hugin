// -*- c-basic-offset: 4 -*-

/** @file PanoOptionsFromIni.h
 *
 *  @brief read settings from ini file and apply only applicable values
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

#include "PanoOptionsFromIni.h"
#include "IniParser.h"
#include <cmath>
#include <algorithms/nona/CenterHorizontally.h>
#include <algorithms/basic/StraightenPanorama.h>
#include <algorithms/nona/FitPanorama.h>
#include <algorithms/basic/CalculateOptimalScale.h>
#include <algorithms/basic/CalculateOptimalROI.h>
#include <algorithms/basic/LayerStacks.h>
#include <algorithms/basic/CalculateMeanExposure.h>

// helper struct to pass information about the whole panorama to the condition checks
struct PanoProperties
{
    double HFOV = 0;
    double VFOV = 0;
    std::string filename;
    std::string path;
};

// cache to store projection names
static std::vector<std::string> projectionNames;
static std::map<HuginBase::BaseSrcPanoImage::Projection, std::string> lensTypeNames;
void ReadProjectionNames()
{
    // cache list of projection names
    const int nP = panoProjectionFormatCount();
    projectionNames.resize(nP);
    for (int n = 0; n < nP; ++n)
    {
        pano_projection_features proj;
        if (panoProjectionFeaturesQuery(n, &proj))
        {
            projectionNames[n] = hugin_utils::tolower(std::string(proj.name));
        };
    };
    // populate lens type map
    lensTypeNames[HuginBase::SrcPanoImage::RECTILINEAR] = "rectilinear";
    lensTypeNames[HuginBase::SrcPanoImage::PANORAMIC] = "cylindrical";
    lensTypeNames[HuginBase::SrcPanoImage::CIRCULAR_FISHEYE] = "fisheye";
    lensTypeNames[HuginBase::SrcPanoImage::FULL_FRAME_FISHEYE] = "full frame fisheye";
    lensTypeNames[HuginBase::SrcPanoImage::EQUIRECTANGULAR] = "equirectangular";
    lensTypeNames[HuginBase::SrcPanoImage::FISHEYE_ORTHOGRAPHIC] = "orthographic";
    lensTypeNames[HuginBase::SrcPanoImage::FISHEYE_STEREOGRAPHIC] = "stereographic";
    lensTypeNames[HuginBase::SrcPanoImage::FISHEYE_EQUISOLID] = "equisolid";
    lensTypeNames[HuginBase::SrcPanoImage::FISHEYE_THOBY] = "fisheye thoby";
}

/** check if given string matches the value with the corresponding operator */
bool CheckString(const std::string& variable, const std::string& operatorString, const std::string& value)
{
    if (operatorString == "==")
    {
        return variable == value;
    };
    if (operatorString == "!=" || operatorString == "<>")
    {
        return variable != value;
    };
    if (operatorString == "=~")
    {
        // variable contains value
        return variable.find(value) != std::string::npos;
    };
    if (operatorString == "!~")
    {
        // variable does not contain value
        return variable.find(value) == std::string::npos;
    };
    std::cerr << "Invalid operator \"" << operatorString << "\"" << std::endl;
    return false;
}

/** general number comparison function */
template<class Number>
bool CheckNumber(const Number variable, const std::string& operatorString, const Number value)
{
    if (operatorString == "==")
    {
        return variable == value;
    };
    if (operatorString == "!=" || operatorString == "<>")
    {
        return variable != value;
    };
    if (operatorString == "<")
    {
        return variable < value;
    };
    if (operatorString == "<=")
    {
        return variable <= value;
    };
    if (operatorString == ">")
    {
        return variable > value;
    };
    if (operatorString == ">=")
    {
        return variable >= value;
    };
    std::cerr << "Invalid operator \"" << operatorString << "\"" << std::endl;
    return false;
}

/** check if given integer matches the condition (operator and value) */
bool CheckInt(const int variable, const std::string& operatorString, const std::string& stringValue)
{
    int value;
    if (!hugin_utils::stringToInt(stringValue, value))
    {
        std::cerr << "Invalid number \"" << stringValue << "\"" << std::endl;
        return false;
    };
    return CheckNumber<int>(variable, operatorString, value);
}

/** check if given double matches the condition (operator and value) */
bool CheckDouble(const double variable, const std::string& operatorString, const std::string& stringValue)
{
    double value;
    if (!hugin_utils::stringToDouble(stringValue, value))
    {
        std::cerr << "Invalid number \"" << stringValue << "\"" << std::endl;
        return false;
    };
    return CheckNumber<double>(variable, operatorString, value);
}

/** check if given condition is fulfilled by given panorama and PanoProperties */
bool CheckCondition(const std::string& condition, const HuginBase::Panorama& pano, const PanoProperties& panoProps)
{
    size_t pos = condition.find_first_of("=<>!~");
    if (pos != std::string::npos)
    {
        // operator found
        const std::string variable = condition.substr(0, pos);
        std::string operatorString;
        std::string value;
        size_t pos2 = condition.find_first_not_of("=<>!~", pos);
        if (pos2 != std::string::npos)
        {
            // value after operator found, extract operator and value
            operatorString = condition.substr(pos, pos2 - pos);
            value = condition.substr(pos2);
            if (value.empty())
            {
                // empty value
                std::cerr << "Missing value in comparison \"" << condition << "\"." << std::endl;
                return false;
            };
            // now do the comparison of the different variables
            if (variable == "CameraModel")
            {
                return CheckString(pano.getImage(0).getExifModel(), operatorString, value);
            };
            if (variable == "CameraMaker")
            {
                return CheckString(pano.getImage(0).getExifMake(), operatorString, value);
            };
            if (variable == "Lens")
            {
                return CheckString(pano.getImage(0).getExifLens(), operatorString, value);
            };
            if (variable == "HFOV")
            {
                return CheckDouble(pano.getImage(0).getHFOV(), operatorString, value);
            };
            if (variable == "LensType")
            {
                if (projectionNames.empty())
                {
                    // make sure projection names are read from libpano
                    ReadProjectionNames();
                };
                return CheckString(lensTypeNames[pano.getImage(0).getProjection()], operatorString, hugin_utils::tolower(value));
            };
            if (variable == "Focallength")
            {
                return CheckDouble(pano.getImage(0).getExifFocalLength(), operatorString, value);
            };
            if (variable == "Focallength35mm")
            {
                return CheckDouble(pano.getImage(0).getExifFocalLength(), operatorString, value);
            };
            if (variable == "Filename")
            {
                return CheckString(panoProps.filename, operatorString, value);
            };
            if (variable == "Path")
            {
                return CheckString(panoProps.path, operatorString, value);
            };
            if (variable == "ImageCount")
            {
                return CheckInt(pano.getNrOfImages(), operatorString, value);
            };
            if (variable == "PanoHFOV")
            {
                return CheckDouble(panoProps.HFOV, operatorString, value);
            };
            if (variable == "PanoVFOV")
            {
                return CheckDouble(panoProps.VFOV, operatorString, value);
            };
        };
    };
    return false;
}

void SetProjection(const std::string& projectionString, HuginBase::PanoramaOptions& options)
{
    if (projectionNames.empty())
    {
        ReadProjectionNames();
    };
    // now compare strings
    for (int i = 0; i < projectionNames.size(); ++i)
    {
        if (projectionNames[i] == projectionString)
        {
            std::cout << "Setting projection to " << projectionString << std::endl;
            options.setProjection(static_cast<HuginBase::PanoramaOptions::ProjectionFormat>(i));
            break;
        };
    };
}

void SetFOV(const std::string& fov, HuginBase::Panorama& pano, HuginBase::PanoramaOptions& options)
{
    if (fov == "auto")
    {
        std::cout << "Fit panorama field of view to best size" << std::endl;
        HuginBase::CalculateFitPanorama fitPano(pano);
        fitPano.run();
        options.setHFOV(fitPano.getResultHorizontalFOV());
        options.setHeight(hugin_utils::roundi(fitPano.getResultHeight()));
        std::cout << "Setting field of view to " << options.getHFOV() << " x " << options.getVFOV() << std::endl;
    }
    else
    {
        double newHFOV = 0.0;
        double newVFOV = 0.0;
        const int n = sscanf(fov.c_str(), "%lfx%lf", &newHFOV, &newVFOV);
        if ((n == 1 || n == 2) && newHFOV > 0.0)
        {
            options.setHFOV(newHFOV);
            if (options.fovCalcSupported(options.getProjection()) && newVFOV > 0)
            {
                options.setVFOV(newVFOV);
            };
            std::cout << "Setting field of view to " << options.getHFOV() << " x " << options.getVFOV() << std::endl;
        };
    };
}

void SetCanvas(const std::string& canvas, HuginBase::Panorama& pano, HuginBase::PanoramaOptions& options)
{
    if (canvas == "auto" || canvas.back() == '%')
    {
        int scale = 100;
        if (canvas.back() == '%')
        {
            if (!hugin_utils::stringToInt(canvas.substr(0, canvas.length() - 1), scale))
            {
                std::cout << "Invalid scale factor \"" << canvas << "\" for canvas" << std::endl;
                scale = 0;
            };
        };
        if (scale > 0)
        {
            std::cout << "Calculate optimal size of panorama" << std::endl;
            double s = HuginBase::CalculateOptimalScale::calcOptimalScale(pano);
            options.setWidth(hugin_utils::roundi(options.getWidth() * s * scale / 100), true);
            std::cout << "Setting canvas size to " << options.getWidth() << " x " << options.getHeight() << std::endl;
        };
    }
    else
    {
        int width = 0;
        int height = 0;
        int n = sscanf(canvas.c_str(), "%dx%d", &width, &height);
        if (n == 2 && width > 0 && height > 0)
        {
            options.setWidth(width);
            options.setHeight(height);
            std::cout << "Setting canvas size to " << options.getWidth() << " x " << options.getHeight() << std::endl;
        }
        else
        {
            std::cerr << "Could not parse canvas size \"" << canvas << "\"." << std::endl;
        };
    };
}

void SetCrop(const std::string& cropString, HuginBase::Panorama& pano, HuginBase::PanoramaOptions& options)
{
    if (cropString == "auto" || cropString == "autohdr" || cropString == "autooutside")
    {
        std::cout << "Searching for best crop rectangle" << std::endl;
        AppBase::DummyProgressDisplay dummy;
        // we need to apply the current options before calculation of crop
        pano.setOptions(options);
        vigra::Rect2D roi;

        if (cropString == "autooutside")
        {
            HuginBase::CalculateOptimalROIOutside cropPano(pano, &dummy);
            cropPano.run();
            roi = cropPano.getResultOptimalROI();
        }
        else
        {
            HuginBase::CalculateOptimalROI cropPano(pano, &dummy);
            if (cropString == "autohdr")
            {
                cropPano.setStacks(getHDRStacks(pano, pano.getActiveImages(), options));
            }
            cropPano.run();
            roi = cropPano.getResultOptimalROI();
        };
        //set the ROI - fail if the right/bottom is zero, meaning all zero
        if (!roi.isEmpty())
        {
            options.setROI(roi);
            std::cout << "Set crop size to " << roi.left() << "," << roi.top() << "," << roi.right() << "," << roi.bottom() << std::endl;
        }
        else
        {
            std::cerr << "Could not find best crop rectangle" << std::endl;
        }
    }
    else
    {
        if (cropString.back() == '%')
        {
            // relative crop
            double left, right, top, bottom;
            const int n = sscanf(cropString.substr(0, cropString.length() - 1).c_str(), "%lf,%lf,%lf,%lf", &left, &right, &top, &bottom);
            bool validCrop = false;
            if (n == 4 && right > left && bottom > top && left >= 0.0 && top >= 0.0 && right <= 100.0 && bottom <= 100.0)
            {
                // check values, 0..100 %
                options.setROI(vigra::Rect2D(options.getWidth() * left / 100.0, options.getHeight() * top / 100.0,
                    options.getWidth() * right / 100.0, options.getHeight() * bottom / 100.0));
                std::cout << "Set crop size to " << options.getROI().left() << "," << options.getROI().top() << "," <<
                    options.getROI().right() << "," << options.getROI().bottom() << std::endl;
                validCrop = true;
            };
            if (!validCrop)
            {
                std::cerr << "Invalid crop area \"" << cropString << "\"" << std::endl;
            };
        }
        else
        {
            // absolute crop
            int left, right, top, bottom;
            int n = sscanf(cropString.c_str(), "%d,%d,%d,%d", &left, &right, &top, &bottom);
            bool validCrop = false;
            if (n == 4 && right > left && bottom > top && left >= 0 && top >= 0)
            {
                options.setROI(vigra::Rect2D(left, top, right, bottom));
                std::cout << "Set crop size to " << options.getROI().left() << "," << options.getROI().top() << "," <<
                    options.getROI().right() << "," << options.getROI().bottom() << std::endl;
                validCrop = true;
            };
            if (!validCrop)
            {
                std::cerr << "Invalid crop area \"" << cropString << "\"" << std::endl;
            };
        };
    };
}

void SetExposure(const std::string& exposure, HuginBase::Panorama& pano, HuginBase::PanoramaOptions& options)
{
    if (exposure == "auto")
    {
        options.outputExposureValue = HuginBase::CalculateMeanExposure::calcMeanExposure(pano);
        std::cout << "Setting output exposure value to " << options.outputExposureValue << std::endl;
    }
    else
    {
        if (!exposure.empty() && exposure.front() == 'r')
        {
            double exposureValue;
            if (hugin_utils::stringToDouble(exposure.substr(1), exposureValue))
            {
                options.outputExposureValue += exposureValue;
                std::cout << "Setting output exposure value to " << options.outputExposureValue << std::endl;
            }
            else
            {
                std::cerr << "Value \"" << exposure << "\" is not a valid exposure value." << std::endl;
            };
        }
        else
        {
            double exposureValue;
            if (hugin_utils::stringToDouble(exposure, exposureValue))
            {
                options.outputExposureValue = exposureValue;
                std::cout << "Setting output exposure value to " << options.outputExposureValue << std::endl;
            }
            else
            {
                std::cerr << "Value \"" << exposure << "\" is not a valid exposure value." << std::endl;
            };
        };
    };
}

void SetOutputType(const std::string& outputType, HuginBase::PanoramaOptions& options)
{
    const std::vector<std::string> outputStrings = hugin_utils::SplitString(outputType, ",");
    if (!outputStrings.empty())
    {
        // reset all output
        // final pano
        options.outputLDRBlended = false;
        options.outputLDRExposureBlended = false;
        options.outputLDRExposureLayersFused = false;
        options.outputHDRBlended = false;
        // remapped images
        options.outputLDRLayers = false;
        options.outputLDRExposureRemapped = false;
        options.outputHDRLayers = false;
        // stacks
        options.outputLDRStacks = false;
        options.outputHDRStacks = false;
        // exposure layers
        options.outputLDRExposureLayers = false;
        for (const auto& output : outputStrings)
        {
            const std::string s = hugin_utils::StrTrim(output);
            if (s == "NORMAL" || s == "N")
            {
                options.outputLDRBlended = true;
                std::cout << "Activate output of normal panorama." << std::endl;
                continue;
            };
            if (s == "STACKSFUSEDBLENDED" || s == "FB")
            {
                options.outputLDRExposureBlended = true;
                std::cout << "Activate output of LDR panorama: Exposure fused from stacks." << std::endl;
                continue;
            };
            if (s == "EXPOSURELAYERSFUSED" || s == "BF")
            {
                options.outputLDRExposureLayersFused = true;
                std::cout << "Activate output of LDR panorama: Exposure fused from any arrangement." << std::endl;
                continue;
            };
            if (s == "HDR")
            {
                options.outputHDRBlended = true;
                std::cout << "Activate output of hdr panorama." << std::endl;
                continue;
            };
            // single remapped images
            if (s == "REMAP")
            {
                options.outputLDRLayers = true;
                std::cout << "Activate output of remapped, exposure corrected images." << std::endl;
                continue;
            };
            if (s == "REMAPORIG")
            {
                options.outputLDRExposureRemapped = true;
                std::cout << "Activate output of remapped images with unmodified exposure." << std::endl;
                continue;
            };
            if (s == "HDRREMAP")
            {
                options.outputHDRLayers = true;
                std::cout << "Activate output of remapped hdr images." << std::endl;
                continue;
            };
            //stacks
            if (s == "FUSEDSTACKS")
            {
                options.outputLDRStacks = true;
                std::cout << "Activate output of exposure fused stacks." << std::endl;
                continue;
            };
            if (s == "HDRSTACKS")
            {
                options.outputHDRStacks = true;
                std::cout << "Activate output of HDR stacks." << std::endl;
                continue;
            };
            //exposure layers
            if (s == "EXPOSURELAYERS")
            {
                options.outputLDRExposureLayers = true;
                std::cout << "Activate output of exposure layers." << std::endl;
                continue;
            };
            std::cerr << "Unknown parameter \"" << s << "\" found in Output-type." << std::endl
                << "Ignoring this parameter." << std::endl;
        };
    };
}

void SetLDRCompression(const std::string ldrCompression, HuginBase::PanoramaOptions& options)
{
    if (options.outputImageType == "tif")
    {
        if (ldrCompression == "NONE" || ldrCompression == "PACKBITS" || ldrCompression == "LZW" || ldrCompression == "DEFLATE")
        {
            options.outputImageTypeCompression = ldrCompression;
            std::cout << "Setting TIF compression to \"" << ldrCompression << "\"." << std::endl;
            options.tiffCompression = ldrCompression;
        }
        else
        {
            std::cout << "LDR compression \"" << ldrCompression << "\" is not a valid compression value for TIF files." << std::endl;
        }
    }
    else
    {
        if (options.outputImageType == "jpg")
        {
            int quality = 0;
            quality = atoi(ldrCompression.c_str());
            if (quality != 0)
            {
                if (quality > 0 && quality <= 100)
                {
                    options.quality = quality;
                    std::cout << "Setting JPEG quality to " << quality << "." << std::endl;
                }
                else
                {
                    std::cerr << "Given value for JPEG quality is outside the valid range 1..100." << std::endl;
                };
            }
            else
            {
                std::cerr << "Could not parse \"" << ldrCompression << "\" as a valid JPEG quality number." << std::endl;
            };
        }
        else
        {
            if (options.outputImageType == "png")
            {
                std::cerr << "Setting compression for PNG images is not supported." << std::endl;
            }
            else
            {
                // this should never happen
                std::cerr << "Unknown image format" << std::endl;
            };
        };
    };
}

void SetHDRCompression(const std::string hdrCompression, HuginBase::PanoramaOptions& options)
{
    if (options.outputImageTypeHDR == "tif")
    {
        if (hdrCompression == "NONE" || hdrCompression == "PACKBITS" || hdrCompression == "LZW" || hdrCompression == "DEFLATE")
        {
            options.outputImageTypeHDRCompression = hdrCompression;
            std::cout << "Setting HDR-TIF compression to \"" << hdrCompression << "\"." << std::endl;
        }
        else
        {
            std::cerr << "HDR compression \"" << hdrCompression << "\" is not a valid compression value for TIF files." << std::endl;
        }
    }
    else
    {
        if (options.outputImageTypeHDR == "exr")
        {
            std::cerr << "Setting compression for EXR images is not supported." << std::endl;
        }
        else
        {
            // this should never happen
            std::cerr << "Unknown HDR image format" << std::endl;
        };
    };
}

/** process the given section, check if all conditions are fulfilled and if so apply all wished settings */
void ProcessSection(const IniParser& iniParser, const std::string& section, HuginBase::Panorama& pano, HuginBase::PanoramaOptions& options, const PanoProperties& panoProps)
{
    const std::vector<std::string> keys = iniParser.GetKeys(section);
    if (!keys.empty())
    {
        bool isConditionFullfilled = true;
        for (const auto& key : keys)
        {
            // check if conidition if fullfilled
            if (key.compare(0, 9, "Condition") == 0)
            {
                isConditionFullfilled = isConditionFullfilled && CheckCondition(iniParser.GetKey(section, key, std::string()), pano, panoProps);
                if (!isConditionFullfilled)
                {
                    // one condition is not fulfilled, we don't need to check the other one
                    break;
                };
            };
        };
        if (isConditionFullfilled)
        {
            std::cout << "Applying settings from \"" << section << "\"" << std::endl;
            // condition is fullfilled, now do all wished settings
            if (iniParser.HasKey(section, "Projection"))
            {
                // projection
                const std::string projection = hugin_utils::tolower(hugin_utils::StrTrim(iniParser.GetKey(section, "Projection", std::string())));
                if (!projection.empty())
                {
                    SetProjection(projection, options);
                };
            };
            if (iniParser.HasKey(section, "FOV"))
            {
                // field of view
                const std::string fov = hugin_utils::tolower(hugin_utils::StrTrim(iniParser.GetKey(section, "FOV", std::string())));
                if (!fov.empty())
                {
                    SetFOV(fov, pano, options);
                };
            };
            if (iniParser.HasKey(section, "Canvas"))
            {
                const std::string canvas = hugin_utils::tolower(hugin_utils::StrTrim(iniParser.GetKey(section, "Canvas", std::string())));
                if (!canvas.empty())
                {
                    SetCanvas(canvas, pano, options);
                };
            };
            if (iniParser.HasKey(section, "Crop"))
            {
                const std::string cropString = hugin_utils::tolower(hugin_utils::StrTrim(iniParser.GetKey(section, "Crop", std::string())));
                if (!cropString.empty())
                {
                    SetCrop(cropString, pano, options);
                };
            };
            if (iniParser.HasKey(section, "OutputExposure"))
            {
                // output exposure
                const std::string exposure = hugin_utils::tolower(hugin_utils::StrTrim(iniParser.GetKey(section, "OutputExposure", std::string())));
                if (!exposure.empty())
                {
                    SetExposure(exposure, pano, options);
                };
            };
            if (iniParser.HasKey(section, "OutputType"))
            {
                const std::string outputType = hugin_utils::toupper(hugin_utils::StrTrim(iniParser.GetKey(section, "OutputType", std::string())));
                SetOutputType(outputType, options);
            };
            if (iniParser.HasKey(section, "Blender"))
            {
                const std::string blender = hugin_utils::tolower(hugin_utils::StrTrim(iniParser.GetKey(section, "Blender", std::string())));
                if (blender == "enblend")
                {
                    options.blendMode = HuginBase::PanoramaOptions::ENBLEND_BLEND;
                    std::cout << "Setting blender type to \"ENBLEND\"." << std::endl;
                }
                else
                {
                    if (blender == "internal" || blender == "verdandi")
                    {
                        options.blendMode = HuginBase::PanoramaOptions::INTERNAL_BLEND;
                        std::cout << "Setting blender type to \"INTERNAL\"." << std::endl;
                    }
                    else
                    {
                        std::cerr << "Blender \"" << blender << "\" is not a valid blender ." << std::endl
                            << "Ignoring parameter." << std::endl;
                    };
                };
            };
            if (iniParser.HasKey(section, "BlenderArgs"))
            {
                const std::string blenderArgs = hugin_utils::tolower(hugin_utils::StrTrim(iniParser.GetKey(section, "BlenderArgs", std::string())));
                switch (options.blendMode)
                {
                    case HuginBase::PanoramaOptions::ENBLEND_BLEND:
                        options.enblendOptions = blenderArgs;
                        std::cout << "Setting enblend arguments to " << blenderArgs << std::endl;
                        break;
                    case HuginBase::PanoramaOptions::INTERNAL_BLEND:
                        options.verdandiOptions = blenderArgs;
                        std::cout << "Setting verdandi arguments to " << blenderArgs << std::endl;
                        break;
                    default:
                        std::cout << "Unknown blender in pto file." << std::endl;
                        break;
                };
            };
            if (iniParser.HasKey(section, "LDRFileType"))
            {
                const std::string ldrFileType = hugin_utils::tolower(hugin_utils::StrTrim(iniParser.GetKey(section, "LDRFileType", std::string())));
                if (!ldrFileType.empty())
                {
                    if (ldrFileType == "jpg" || ldrFileType == "png" || ldrFileType == "tif")
                    {
                        options.outputImageType = ldrFileType;
                        std::cout << "Setting ldr output to filetype \"" << ldrFileType << "\"." << std::endl;
                    }
                    else
                    {
                        std::cerr << "LDR file format \"" << ldrFileType << "\" is not a valid LDR output filetype." << std::endl;
                    };
                };
            };
            if (iniParser.HasKey(section, "LDRCompression"))
            {
                const std::string ldrCompression = hugin_utils::toupper(hugin_utils::StrTrim(iniParser.GetKey(section, "LDRCompression", std::string())));
                SetLDRCompression(ldrCompression, options); 
            };
            if (iniParser.HasKey(section, "HDRFileType"))
            {
                const std::string hdrFileType = hugin_utils::tolower(hugin_utils::StrTrim(iniParser.GetKey(section, "HDRFileType", std::string())));
                if (!hdrFileType.empty())
                {
                    if (hdrFileType == "exr" || hdrFileType == "tif")
                    {
                        options.outputImageTypeHDR = hdrFileType;
                        std::cout << "Setting hdr output to filetype \"" << hdrFileType << "\"." << std::endl;
                    }
                    else
                    {
                        std::cerr << "HDR file format \"" << hdrFileType << "\" is not a valid HDR output filetype." << std::endl;
                    };
                };
            };
            if (iniParser.HasKey(section, "HDRCompression"))
            {
                const std::string hdrCompression = hugin_utils::toupper(hugin_utils::StrTrim(iniParser.GetKey(section, "HDRCompression", std::string())));
                SetHDRCompression(hdrCompression, options);
            };
        }
        else
        {
            std::cout << "Ignoring settings from \"" << section << "\" because condition is not fulfilled." << std::endl;
        };
    };
}

HuginBase::PanoramaOptions ReadPanoramaOptionsFromIni(const std::string& iniFile, HuginBase::Panorama& pano)
{
    HuginBase::PanoramaOptions options=pano.getOptions();
    if (pano.getNrOfImages() == 0)
    {
        return options;
    };
    IniParser iniParser;
    iniParser.Read(iniFile);
    // read EXIF data for first image
    HuginBase::SrcPanoImage image0 = pano.getImage(0);
    image0.readEXIF();
    pano.setSrcImage(0, image0);
    // calculate fov for whole pano
    PanoProperties panoProps;
    {
        HuginBase::CalculateFitPanorama fitPano(pano);
        fitPano.run();
        HuginBase::PanoramaOptions generalOptions(options);
        generalOptions.setHFOV(fitPano.getResultHorizontalFOV());
        generalOptions.setHeight(hugin_utils::roundi(fitPano.getResultHeight()));
        panoProps.HFOV = generalOptions.getHFOV();
        panoProps.VFOV = generalOptions.getVFOV();
        // store some more variables
        const std::string filename = hugin_utils::GetAbsoluteFilename(pano.getImage(0).getFilename());
        panoProps.filename = hugin_utils::stripPath(filename);
        panoProps.path = hugin_utils::getPathPrefix(filename);
    }
    // now read settings from ini file
    const std::vector<std::string> sections = iniParser.GetSections();
    if (!sections.empty())
    {
        for (const auto& section : sections)
        {
            // now process current section
            ProcessSection(iniParser, section, pano, options, panoProps);
        };
    };
    return options;
}
