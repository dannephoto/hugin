// -*- c-basic-offset: 4 -*-

/** @file GenerateThumbnail.h
 *
 *  @brief generate thumbnail from given pto file
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

#include "GenerateThumbnail.h"
#include "algorithms/nona/NonaFileStitcher.h"
#include "hugin_base/nona/ImageRemapper.h"
#include "hugin_base/nona/Stitcher.h"

bool GenerateThumbnail(const std::string pto_filename, const wxSize size, vigra::BRGBImage& panoImage, vigra::BImage& panoMask, vigra::ImageImportInfo::ICCProfile& iccProfile)
{
    // read pto file
    HuginBase::Panorama pano;
    if (!pano.ReadPTOFile(pto_filename, hugin_utils::getPathPrefix(pto_filename)))
    {
        return false;
    };
    // check that all images are available
    for (long i = 0; i < pano.getNrOfImages(); ++i)
    {
        if (!hugin_utils::FileExists(pano.getImage(i).getFilename()))
        {
            return false;
        };
    };
    if (pano.getActiveImages().empty())
    {
        // no active images in pto file
        return false;
    }
    // calculate scale factor
    HuginBase::PanoramaOptions opts (pano.getOptions());
    const vigra::Rect2D roi(opts.getROI());
    const double scale = std::max(roi.width() * 1.0 / size.GetWidth(), roi.height() * 1.0 / size.GetHeight());
    opts.setWidth(opts.getWidth() / scale);
    // TO DO: check that images has enough pixel
    vigra::Rect2D newROI(opts.getROI());
    newROI.addSize(vigra::Size2D(std::min(size.GetWidth() - newROI.width(), 0), std::min(size.GetHeight() - newROI.height(), 0)));
    opts.setROI(newROI);
    // now remapped and stitch images
    AppBase::ProgressDisplay* pdisp = new AppBase::DummyProgressDisplay;
    //prepare some settings
    opts.tiff_saveROI = false;
    opts.outputFormat = HuginBase::PanoramaOptions::PNG;
    opts.outputImageType = "png";
    opts.outputMode = HuginBase::PanoramaOptions::OUTPUT_LDR;
    opts.remapUsingGPU = false;
    if (pano.getImage(0).getResponseType() == HuginBase::BaseSrcPanoImage::RESPONSE_EMOR)
    {
        // get the emor parameters.
        opts.outputEMoRParams = pano.getSrcImage(0).getEMoRParams();
    }
    else
    {
        // clear the parameters to indicatate these should not be used
        opts.outputEMoRParams.clear();
    };
    HuginBase::Nona::AdvancedOptions advOptions;
    HuginBase::Nona::SetAdvancedOption(advOptions, "saveIntermediateImages", false);
    HuginBase::Nona::SetAdvancedOption(advOptions, "hardSeam", true);
    HuginBase::Nona::FileRemapper<vigra::BRGBImage, vigra::BImage> remapper;
    HuginBase::Nona::WeightedStitcher<vigra::BRGBImage, vigra::BImage> stitcher(pano, pdisp);
    remapper.setAdvancedOptions(advOptions);
    // create panorama canvas
    vigra::BRGBImage preview(opts.getWidth(), opts.getHeight());
    vigra::BImage previewMask(opts.getWidth(), opts.getHeight());
    // finally stitch and merge images
    stitcher.init(opts, pano.getActiveImages());
    stitcher.stitch(opts, pano.getActiveImages(), std::string(), preview, previewMask, remapper, advOptions);
    // HuginBase::Nona::WeightedStitcher is using the full canvas size
    // so we copy out only the crop portion into the final image data
    panoImage.resize(stitcher.GetPanoROI().size());
    panoMask.resize(panoImage.size());
    vigra::copyImage(vigra::srcImageRange(preview, stitcher.GetPanoROI()), vigra::destImage(panoImage));
    vigra::copyImage(vigra::srcImageRange(previewMask, stitcher.GetPanoROI()), vigra::destImage(panoMask));
    iccProfile = stitcher.GetICCProfile();
    delete pdisp;
    return true;
}
