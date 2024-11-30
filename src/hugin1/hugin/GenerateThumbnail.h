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

#ifndef GENERATE_THUMBNAIL_H
#define GENERATE_THUMBNAIL_H

#include <string>
#include <panodata/Panorama.h>
#include <vigra/stdimage.hxx>
#include "wx/gdicmn.h"

/** generate thumbnail image for given pto_filename 
  * @param[in] pto_filename filename of pto file for which the thumbnail should be generated
  * @param[in] size maximal width and height of generated thumbnail
  * @param[out] panoImage vigra::BRGBImage holding the preview image data
  * @param[out] panoMask alpha channel of the preview image
  * @param[out] iccProfile iccProfile for the given panorama read from the input files
  */
bool GenerateThumbnail(const std::string pto_filename, const wxSize size, vigra::BRGBImage& panoImage, vigra::BImage& panoMask, vigra::ImageImportInfo::ICCProfile& iccProfile);

#endif
