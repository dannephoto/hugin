// -*- c-basic-offset: 4 -*-

/** @file checkpto.cpp
 *
 *  @brief helper program for assistant
 *
 *
 *  @author Thomas Modes
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

#include <hugin_config.h>

#include <fstream>
#include <sstream>
#include <random>
#include <getopt.h>

#include <panodata/Panorama.h>
#include "algorithms/optimizer/ImageGraph.h"
#include "hugin_base/panotools/PanoToolsUtils.h"
#include "hugin_base/panodata/StandardImageVariableGroups.h"
#include "algorithms/basic/CalculateCPStatistics.h"
#include "algorithms/basic/LayerStacks.h"
#include "hugin_base/hugin_utils/filesystem.h"
#include "vigra/impexalpha.hxx"

static void usage(const char* name)
{
    std::cout << name << ": report the number of image groups in a project" << std::endl
         << name << " version " << hugin_utils::GetHuginVersion() << std::endl
         << std::endl
         << "Usage:  " << name << " input.pto" << std::endl
         << std::endl
         << name << " examines the connections between images in a project and" << std::endl
         << "reports back the number of parts or image groups in that project" << std::endl
         << std::endl
         << "Further switches:" << std::endl
         << "  --print-output-info     Print more information about the output" << std::endl
         << "  --print-lens-info       Print more information about lenses" << std::endl
         << "  --print-stack-info      Print more information about assigned stacks" << std::endl
         << "  --print-image-info      Print information about image files" << std::endl
         << "  --create-missing-images Creates placeholder images for non-existing" << std::endl
         << "                          images in same directory as the pto file" << std::endl
         << std::endl
         << name << " is used by the assistant" << std::endl
         << std::endl;
}

void PrintImageGroup(const std::vector<HuginBase::UIntSet>& imageGroup, const std::string& prefix=std::string())
{
    for (size_t i=0; i < imageGroup.size(); i++)
    {
        if (!prefix.empty())
        {
            std::cout << prefix << " " << i << ": ";
        };
        std::cout << "[";
        size_t c=0;
        for (HuginBase::UIntSet::const_iterator it = imageGroup[i].begin(); it != imageGroup[i].end(); ++it)
        {
            std::cout << (*it);
            if (c+1 != imageGroup[i].size())
            {
                std::cout << ", ";
            }
            ++c;
        }
        std::cout << "]";
        if (prefix.empty() && (i+1 != imageGroup.size()))
        {
            std::cout << ", ";
        }
        std::cout << std::endl;
    }
};

void InspectRGBImage(const vigra::FRGBImage &image)
{
    vigra::FindMinMax<float> minmax;
    vigra::inspectImage(srcImageRange(image, vigra::RedAccessor<vigra::RGBValue<float>>()), minmax);
    std::cout << "    Red channel: " << minmax.min << "-" << minmax.max << std::endl;
    minmax.reset();
    vigra::inspectImage(srcImageRange(image, vigra::GreenAccessor<vigra::RGBValue<float>>()), minmax);
    std::cout << "    Green channel: " << minmax.min << "-" << minmax.max << std::endl;
    minmax.reset();
    vigra::inspectImage(srcImageRange(image, vigra::BlueAccessor<vigra::RGBValue<float>>()), minmax);
    std::cout << "    Blue channel: " << minmax.min << "-" << minmax.max << std::endl;
}

void InspectGrayscaleImage(const vigra::FImage &image, const std::string text)
{
    vigra::FindMinMax<float> minmax;
    vigra::inspectImage(srcImageRange(image), minmax);
    std::cout << "    " << text << ": " << minmax.min << "-" << minmax.max << std::endl;
}

void PrintImageInfo(const HuginBase::Panorama& pano)
{
    // print info for all images
    std::cout << std::endl;
    for (int imgNr = 0; imgNr < pano.getNrOfImages(); ++imgNr)
    {
        const std::string filename = pano.getImage(imgNr).getFilename();
        std::cout << "Image " << imgNr << ": " << filename;
        if (hugin_utils::FileExists(filename))
        {
            if (vigra::isImage(filename.c_str()))
            {
                vigra::ImageImportInfo info(filename.c_str());
                fs::path file(filename);
                std::cout << std::endl
                    << "    File type: " << info.getFileType() << std::endl;
                const auto fileSize = fs::file_size(file);
                std::cout << "    File size: ";
                if (fileSize > 1000)
                {
                    std::cout << fileSize / 1000 << " kB" << std::endl;
                }
                else
                {
                    std::cout << fileSize << " B" << std::endl;
                };
                std::cout << "    Pixel type: " << info.getPixelType() << std::endl
                    << "    Pixel size: " << info.width() << "x" << info.height() << std::endl
                    << "    Resolution: " << info.getXResolution() << "/" << info.getYResolution() << std::endl
                    << "    Offset: " << info.getPosition() << std::endl
                    << "    Canvas size: " << info.getCanvasSize() << std::endl
                    << "    ICC profile: " << (info.getICCProfile().empty() ? "no" : hugin_utils::GetICCDesc(info.getICCProfile())) << std::endl
                    << "    Bands: " << info.numBands() << std::endl
                    << "    Extra bands: " << info.numExtraBands() << std::endl;
                if (info.numImages() > 1)
                {
                    std::cout << "    Subimages: " << info.numImages() << " (reading index " << info.getImageIndex() << ")" << std::endl;
                };
                if (info.numExtraBands() == 0)
                {
                    // no mask
                    if (info.numBands() == 3)
                    {
                        //RGB image
                        vigra::FRGBImage image(info.size());
                        vigra::importImage(info, destImage(image));
                        InspectRGBImage(image);
                    }
                    else
                    {
                        if (info.numBands() == 1)
                        {
                             // grayscale image
                            vigra::FImage image(info.size());
                            vigra::importImage(info, destImage(image));
                            InspectGrayscaleImage(image, "Grey channel");
                        }
                        else
                        {
                            std::cout << "    Only RGB or grayscale images supported" << std::endl;
                        };
                    };
                    std::cout << std::endl;
                }
                else
                {
                    if (info.numExtraBands() == 1)
                    {
                        // single mask
                        if (info.numBands() == 4)
                        {
                            // RGB image with mask
                            vigra::FRGBImage image(info.size());
                            vigra::FImage mask(info.size());
                            vigra::importImageAlpha(info, destImage(image), destImage(mask));
                            InspectRGBImage(image);
                            InspectGrayscaleImage(mask, "Mask");
                        }
                        else
                        {
                            if (info.numBands() == 2)
                            {
                                // grayscale image with mask
                                vigra::FImage image(info.size());
                                vigra::FImage mask(info.size());
                                vigra::importImageAlpha(info, destImage(image), destImage(mask));
                                InspectGrayscaleImage(image, "Grey channel");
                                InspectGrayscaleImage(mask, "Mask");
                            }
                            else
                            {
                                std::cout << "    Only RGB or grayscale images supported" << std::endl;
                            };
                        };
                        std::cout << std::endl;
                    }
                    else
                    {
                        std::cout << "    Only images with one or no alpha channel supported" << std::endl;
                    };
                };
            }
            else
            {
                // no recognized image type
                fs::path file(filename);
                std::cout << std::endl << "    not recognized by vigra as image file" << std::endl
                    << "    File size: " << fs::file_size(file) / 1024 << " kiB" << std::endl << std::endl;
            };
        }
        else
        {
            // file does not exists
            std::cout << " does not exists." << std::endl <<std::endl;
        };
    };
}

void CreateMissingImages(HuginBase::Panorama& pano, const std::string& output)
{
    // init the random generator
    std::mt19937 rng(0);
    std::uniform_int_distribution<> distribIndex(0, 128);
    auto randIndex = std::bind(distribIndex, rng);

    bool requiresPTOrewrite = false;
    // store path to pto file for easier access
    const std::string ptoPath = hugin_utils::getPathPrefix(hugin_utils::GetAbsoluteFilename(output));
    const fs::path imagePath(ptoPath);
    // check all images
    for (int imgNr = 0; imgNr < pano.getNrOfImages(); ++imgNr)
    {
        fs::path srcFile(pano.getImage(imgNr).getFilename());
        std::cout << "Image " << imgNr << ": " << srcFile.string();
        if (fs::exists(srcFile))
        {
            std::cout << " exists." << std::endl;
        }
        else
        {
            // image does not exists
            srcFile = fs::absolute(srcFile);
            fs::path newImage(imagePath);
            newImage /= srcFile.filename();
            // file is not in the same directory as pto file, adjust path
            if (newImage != srcFile)
            {
                requiresPTOrewrite = true;
                pano.setImageFilename(imgNr, newImage.string());
            };
            std::cout << " does not exist. Creating " << newImage.filename() << std::endl;
            // now create dummy image and save it to disc
            vigra::UInt8RGBImage image(pano.getImage(imgNr).getWidth(), pano.getImage(imgNr).getHeight(),
                vigra::RGBValue<vigra::UInt8>(64 + randIndex(), 64 + randIndex(), 64 + randIndex()));
            vigra::ImageExportInfo imgExportInfo(newImage.string().c_str());
            vigra::exportImage(vigra::srcImageRange(image), imgExportInfo);
        };
    };
    if (requiresPTOrewrite)
    {
        // resave pto file in case path to some images has changed
        std::cout << std::endl << "Writing " << output << std::endl;
        std::ofstream of(output.c_str());
        HuginBase::UIntSet imgs;
        fill_set(imgs, 0, pano.getNrOfImages() - 1);
        pano.printPanoramaScript(of, pano.getOptimizeVector(), pano.getOptions(), imgs, false, ptoPath);
    };
}

int main(int argc, char* argv[])
{
    // parse arguments
    const char* optstring = "h";
    enum
    {
        PRINT_OUTPUT_INFO=1000,
        PRINT_LENS_INFO=1004,
        PRINT_STACK_INFO=1005,
        CREATE_DUMMY_IMAGES=1006,
        PRINT_IMAGE_INFO,
    };
    static struct option longOptions[] =
    {
        { "print-output-info", no_argument, NULL, PRINT_OUTPUT_INFO },
        { "print-lens-info", no_argument, NULL, PRINT_LENS_INFO },
        { "print-stack-info", no_argument, NULL, PRINT_STACK_INFO },
        { "print-image-info", no_argument, NULL, PRINT_IMAGE_INFO },
        { "create-missing-images", no_argument, NULL, CREATE_DUMMY_IMAGES },
        { "help", no_argument, NULL, 'h' },
        0
    };

    int c;
    bool printOutputInfo=false;
    bool printLensInfo = false;
    bool printStackInfo = false;
    bool printImageInfo = false;
    bool createDummyImages = false;
    int optionIndex = 0;
    while ((c = getopt_long (argc, argv, optstring, longOptions,nullptr)) != -1)
    {
        switch (c)
        {
            case 'h':
                usage(hugin_utils::stripPath(argv[0]).c_str());
                return 0;
            case PRINT_OUTPUT_INFO:
                printOutputInfo=true;
                break;
            case PRINT_LENS_INFO:
                printLensInfo = true;
                break;
            case PRINT_STACK_INFO:
                printStackInfo = true;
                break;
            case PRINT_IMAGE_INFO:
                printImageInfo = true;
                break;
            case CREATE_DUMMY_IMAGES:
                createDummyImages = true;
                break;
            case '?':
            case ':':
                // missing argument or invalid switch
                return 1;
                break;
            default:
                // this should not happen
                abort ();
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
        return -1;
    }


    std::string input=argv[optind];
    // filename for output if needed
    std::string output;
    if (createDummyImages)
    {
        output = input.substr(0, input.length() - 4).append("_dummy.pto");
    };

    HuginBase::Panorama pano;
    std::ifstream prjfile(input.c_str());
    if (!prjfile.good())
    {
        std::cerr << "could not open script : " << input << std::endl;
        return -1;
    }
    pano.setFilePrefix(hugin_utils::getPathPrefix(input));
    AppBase::DocumentData::ReadWriteError err = pano.readData(prjfile);
    if (err != AppBase::DocumentData::SUCCESSFUL)
    {
        std::cerr << "error while parsing panos tool script: " << input << std::endl
            << "DocumentData::ReadWriteError code: " << err << std::endl;
        return -1;
    }

    HuginBase::ConstStandardImageVariableGroups variable_groups(pano);
    std::cout << std::endl
              << "Opened project " << input << std::endl << std::endl
              << "Project contains" << std::endl
              << pano.getNrOfImages() << " images" << std::endl
              << variable_groups.getLenses().getNumberOfParts() << " lenses" << std::endl
              << variable_groups.getStacks().getNumberOfParts() << " stacks" << std::endl
              << pano.getNrOfCtrlPoints() << " control points" << std::endl << std::endl;
    //cp statistics
    if(pano.getNrOfCtrlPoints()>0)
    {
        double min;
        double max;
        double mean;
        double var;
        HuginBase::PTools::calcCtrlPointErrors(pano);
        HuginBase::CalculateCPStatisticsError::calcCtrlPntsErrorStats(pano, min, max, mean, var);
        if(max>0)
        {
            std::cout << "Control points statistics" << std::endl
                      << std::fixed << std::setprecision(2)
                      << "\tMean error        : " << mean << std::endl
                      << "\tStandard deviation: " << sqrt(var) << std::endl
                      << "\tMinimum           : " << min << std::endl
                      << "\tMaximum           : " << max << std::endl;
        };
    };
    HuginGraph::ImageGraph graph(pano);
    const HuginGraph::ImageGraph::Components comps = graph.GetComponents();
    int returnValue=0;
    if(comps.size()==1)
    {
        std::cout << "All images are connected." << std::endl;
        // return value must be 0, otherwise the assistant does not continue
        returnValue=0;
    }
    else
    {
        std::cout << "Found unconnected images!" << std::endl
                  << "There are " << comps.size() << " image groups." << std::endl;

        std::cout << "Image groups: " << std::endl;
        for (size_t i=0; i < comps.size(); i++)
        {
            std::cout << "[";
            HuginGraph::ImageGraph::Components::value_type::const_iterator it;
            size_t c=0;
            for (it = comps[i].begin(); it != comps[i].end(); ++it)
            {
                std::cout << (*it);
                if (c+1 != comps[i].size())
                {
                    std::cout << ", ";
                }
                c++;
            }
            std::cout << "]";
            if (i+1 != comps.size())
            {
                std::cout << ", " << std::endl;
            }
        }
        returnValue=comps.size();
    };
    std::cout << std::endl;
    if (printLensInfo)
    {
        std::cout << std::endl << "Lenses:" << std::endl;
        PrintImageGroup(variable_groups.getLenses().getPartsSet(), "Lens");
    };
    if (printStackInfo)
    {
        std::cout << std::endl << "Stacks:" << std::endl;
        PrintImageGroup(variable_groups.getStacks().getPartsSet(), "Stack");
    };
    if (printOutputInfo)
    {
        HuginBase::UIntSet outputImages=HuginBase::getImagesinROI(pano, pano.getActiveImages());
        std::vector<HuginBase::UIntSet> stacks=HuginBase::getHDRStacks(pano, outputImages, pano.getOptions());
        std::cout << std::endl << "Output contains" << std::endl
             << stacks.size() << " images stacks:" << std::endl;
        PrintImageGroup(stacks);
        std::vector<HuginBase::UIntSet> layers=HuginBase::getExposureLayers(pano, outputImages, pano.getOptions());
        std::cout << std::endl << std::endl << "and " << layers.size() << " exposure layers:" << std::endl;
        PrintImageGroup(layers);
    };
    if (printImageInfo)
    {
        std::cout << "Supported file formats: " << vigra::impexListFormats() << std::endl
            << "Supported extensions: " << vigra::impexListExtensions() << std::endl;
        PrintImageInfo(pano);
    };
    if (createDummyImages)
    {
        CreateMissingImages(pano, output);
    };
    return returnValue;
}
