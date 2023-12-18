// -*- c-basic-offset: 4 -*-
/** @file PanoCommand.cpp
*
*  @author Pablo d'Angelo <pablo.dangelo@web.de>
*/
/*
*  This is free software; you can redistribute it and/or
*  modify it under the terms of the GNU General Public
*  License as published by the Free Software Foundation; either
*  version 2 of the License, or (at your option) any later version.
*
*  This software is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
*  Lesser General Public License for more details.
*
*  You should have received a copy of the GNU General Public
*  License along with this software. If not, see
*  <http://www.gnu.org/licenses/>.
*
*/

#include "PanoCommand.h"
#include <fstream>
#include <panotools/PanoToolsUtils.h>

#include <algorithms/nona/CenterHorizontally.h>
#include <algorithms/nona/FitPanorama.h>
#include <algorithms/basic/RotatePanorama.h>
#include <algorithms/basic/TranslatePanorama.h>
#include <algorithms/basic/CalculateMeanExposure.h>
#include <algorithms/basic/StraightenPanorama.h>
#include <panodata/ParseExp.h>

namespace PanoCommand
{
    CombinedPanoCommand::~CombinedPanoCommand()
    {
        for (std::vector<PanoCommand*>::iterator it = commands.begin(); it != commands.end(); ++it)
        {
            delete *it;
        }
    }

    bool CombinedPanoCommand::processPanorama(HuginBase::Panorama & pano)
    {
        bool result = true;
        for (std::vector<PanoCommand*>::iterator it = commands.begin(); it != commands.end(); ++it)
        {
            result &= (**it).processPanorama(pano);
        }
        /// @todo Should I revert if processing fails?
        return result;
    };

    bool NewPanoCmd::processPanorama(HuginBase::Panorama& pano)
    {
        pano.reset();
        return true;
    }

    bool AddImagesCmd::processPanorama(HuginBase::Panorama& pano)
    {
        std::vector<HuginBase::SrcPanoImage>::const_iterator it;
        for (it = imgs.begin(); it != imgs.end(); ++it)
        {
            pano.addImage(*it);
        }
        return true;
    }

    bool RemoveImageCmd::processPanorama(HuginBase::Panorama& pano)
    {
        pano.removeImage(imgNr);
        return true;
    }

    bool RemoveImagesCmd::processPanorama(HuginBase::Panorama& pano)
    {
        for (HuginBase::UIntSet::reverse_iterator it = imgNrs.rbegin();
            it != imgNrs.rend(); ++it)
        {
            pano.removeImage(*it);
        }
        return true;
    }

    bool UpdateVariablesCmd::processPanorama(HuginBase::Panorama& pano)
    {
        pano.updateVariables(vars);
        HuginBase::PTools::calcCtrlPointErrors(pano);
        return true;
    }

    bool UpdateCPsCmd::processPanorama(HuginBase::Panorama& pano)
    {
        HuginBase::CPVector::const_iterator it;
        unsigned int i = 0;
        for (it = cps.begin(); it != cps.end(); ++it, i++)
        {
            pano.changeControlPoint(i, *it);
        }
        if (updateCPError)
        {
            HuginBase::PTools::calcCtrlPointErrors(pano);
        };

        return true;
    }

    bool UpdateVariablesCPCmd::processPanorama(HuginBase::Panorama& pano)
    {
        pano.updateVariables(vars);
        pano.updateCtrlPointErrors(cps);
        return true;
    }

    bool UpdateVariablesCPSetCmd::processPanorama(HuginBase::Panorama& pano)
    {
        pano.updateVariables(m_imgs, vars);
        pano.updateCtrlPointErrors(m_imgs, cps);
        pano.markAsOptimized();
        return true;
    }

    bool UpdateImageVariablesCmd::processPanorama(HuginBase::Panorama& pano)
    {
        pano.updateVariables(imgNr, vars);
        HuginBase::PTools::calcCtrlPointErrors(pano);
        return true;
    }

    bool UpdateImagesVariablesCmd::processPanorama(HuginBase::Panorama& pano)
    {
        HuginBase::UIntSet::iterator it;
        HuginBase::VariableMapVector::const_iterator v_it = vars.begin();
        for (it = change.begin(); it != change.end(); ++it)
        {
            pano.updateVariables(*it, *v_it);
            ++v_it;
        }
        HuginBase::PTools::calcCtrlPointErrors(pano);
        return true;
    }

    bool UpdateVariablesByParseExpression::processPanorama(HuginBase::Panorama& pano)
    {
        if (!m_expression.empty())
        {
            Parser::PanoParseExpression(pano, m_expression);
            HuginBase::PTools::calcCtrlPointErrors(pano);
            return true;
        }
        else
        {
            return false;
        };
    }

    bool UpdateOptimizeVectorCmd::processPanorama(HuginBase::Panorama & pano)
    {
        pano.setOptimizeVector(m_optvec);
        return true;
    }


    bool UpdateOptimizerSwitchCmd::processPanorama(HuginBase::Panorama & pano)
    {
        pano.setOptimizerSwitch(m_mode);
        return true;
    }

    bool UpdatePhotometricOptimizerSwitchCmd::processPanorama(HuginBase::Panorama & pano)
    {
        pano.setPhotometricOptimizerSwitch(m_mode);
        return true;
    }

    bool SetVariableCmd::processPanorama(HuginBase::Panorama& pano)
    {
        HuginBase::UIntSet::iterator it;
        for (it = images.begin(); it != images.end(); ++it)
        {
            pano.updateVariable(*it, var);
        }
        HuginBase::PTools::calcCtrlPointErrors(pano);

        return true;
    }

    bool CenterPanoCmd::processPanorama(HuginBase::Panorama& pano)
    {
        HuginBase::CenterHorizontally(pano).run();
        // adjust canvas size
        HuginBase::CalculateFitPanorama fitPano(pano);
        fitPano.run();
        HuginBase::PanoramaOptions opts = pano.getOptions();
        opts.setHFOV(fitPano.getResultHorizontalFOV());
        opts.setHeight(hugin_utils::roundi(fitPano.getResultHeight()));
        pano.setOptions(opts);
        HuginBase::PTools::calcCtrlPointErrors(pano);

        return true;
    }

    bool StraightenPanoCmd::processPanorama(HuginBase::Panorama& pano)
    {
        HuginBase::StraightenPanorama(pano).run();

        HuginBase::PanoramaOptions opts = pano.getOptions();
        if (opts.getHFOV()<360)
        {
            // center non 360 deg panos
            HuginBase::CenterHorizontally(pano).run();
        };
        // adjust canvas size
        HuginBase::CalculateFitPanorama fitPano(pano);
        fitPano.run();
        opts.setHFOV(fitPano.getResultHorizontalFOV());
        opts.setHeight(hugin_utils::roundi(fitPano.getResultHeight()));
        pano.setOptions(opts);
        HuginBase::PTools::calcCtrlPointErrors(pano);

        return true;
    }

    void UpdateSingleCtrlPointError(const HuginBase::Panorama& pano, HuginBase::ControlPoint& point)
    {
        HuginBase::UIntSet imgs;
        imgs.insert(point.image1Nr);
        imgs.insert(point.image2Nr);
        HuginBase::Panorama copyPano = pano.duplicate();
        HuginBase::CPVector cpVector;
        // copy only the given cp to the duplicated pano
        cpVector.push_back(point);
        copyPano.setCtrlPoints(cpVector);
        // create subpano with only images for the single cp
        HuginBase::Panorama subPano = copyPano.getSubset(imgs);
        // finally calc cp error
        HuginBase::PTools::calcCtrlPointErrors(subPano);
        point.error = subPano.getCtrlPoint(0).error;
    }

    bool AddCtrlPointCmd::processPanorama(HuginBase::Panorama& pano)
    {
        if (point.image1Nr > point.image2Nr)
        {
            // if the first image number is bigger than the second, mirror the point
            point.mirror();
        };
        // calculate cp error only for newly added control point
        UpdateSingleCtrlPointError(pano, point);
        pano.addCtrlPoint(point);
        return true;
    }

    bool AddCtrlPointsCmd::processPanorama(HuginBase::Panorama& pano)
    {
        HuginBase::UIntSet imgs;
        for (HuginBase::CPVector::iterator it = cps.begin(); it != cps.end(); ++it)
        {
            if ((*it).image1Nr > (*it).image2Nr)
            {
                (*it).mirror();
            };
            imgs.insert(it->image1Nr);
            imgs.insert(it->image2Nr);
        }
        // calculate cp error only for newly added control point
        // create subpano with only images for the new cp
        HuginBase::Panorama copyPano = pano.duplicate();
        copyPano.setCtrlPoints(cps);
        HuginBase::Panorama subPano = copyPano.getSubset(imgs);
        // finally calc cp error
        HuginBase::PTools::calcCtrlPointErrors(subPano);
        for (auto it = cps.begin(); it != cps.end(); ++it)
        {
            it->error = subPano.getCtrlPoint(it - cps.begin()).error;
            pano.addCtrlPoint(*it);
        };
        return true;
    }

    bool RemoveCtrlPointCmd::processPanorama(HuginBase::Panorama& pano)
    {
        pano.removeCtrlPoint(pointNr);
        return true;
    }

    bool RemoveCtrlPointsCmd::processPanorama(HuginBase::Panorama& pano)
    {
        for (HuginBase::UIntSet::reverse_iterator it = m_points.rbegin(); it != m_points.rend(); ++it)
        {
            pano.removeCtrlPoint(*it);
        }
        return true;
    }

    bool ChangeCtrlPointCmd::processPanorama(HuginBase::Panorama& pano)
    {
        // calculate cp error only for the changed control point
        UpdateSingleCtrlPointError(pano, point);
        // finally change the point
        pano.changeControlPoint(pNr, point);
        return true;
    }

    bool SetActiveImagesCmd::processPanorama(HuginBase::Panorama& pano)
    {
        HuginBase::UIntSet::iterator it;
        for (unsigned int i = 0; i < pano.getNrOfImages(); i++)
        {
            if (set_contains(m_active, i))
            {
                pano.activateImage(i, true);
            }
            else
            {
                pano.activateImage(i, false);
            };
        }
        return true;
    }

    bool SwapImagesCmd::processPanorama(HuginBase::Panorama& pano)
    {
        pano.swapImages(m_i1, m_i2);
        return true;
    }

    bool MoveImageCmd::processPanorama(HuginBase::Panorama& pano)
    {
        pano.moveImage(m_i1, m_i2);
        return true;
    }

    bool MergePanoCmd::processPanorama(HuginBase::Panorama& pano)
    {
        pano.mergePanorama(newPano);
        HuginBase::PTools::calcCtrlPointErrors(pano);
        return true;
    }

    bool UpdateSrcImageCmd::processPanorama(HuginBase::Panorama& pano)
    {
        pano.setSrcImage(imgNr, img);
        HuginBase::PTools::calcCtrlPointErrors(pano);
        return true;
    }

    bool UpdateSrcImagesCmd::processPanorama(HuginBase::Panorama& pano)
    {
        int i = 0;
        for (HuginBase::UIntSet::iterator it = imgNrs.begin(); it != imgNrs.end(); ++it)
        {
            pano.setSrcImage(*it, imgs[i]);
            i++;
        }
        HuginBase::PTools::calcCtrlPointErrors(pano);
        return true;
    }

    bool SetPanoOptionsCmd::processPanorama(HuginBase::Panorama& pano)
    {
        pano.setOptions(options);
        HuginBase::PTools::calcCtrlPointErrors(pano);
        return true;
    }

    LoadPTProjectCmd::LoadPTProjectCmd(HuginBase::Panorama & p, const std::string  & filename, const std::string & prefix)
        : PanoCommand(p), filename(filename), prefix(prefix)
    {
        m_clearDirty = true;
    }

    bool LoadPTProjectCmd::processPanorama(HuginBase::Panorama& pano)
    {
        if (!pano.ReadPTOFile(filename))
        {
            return false;
        }
        return true;
    }

    bool RotatePanoCmd::processPanorama(HuginBase::Panorama& pano)
    {
        HuginBase::RotatePanorama(pano, y, p, r).run();
        HuginBase::PTools::calcCtrlPointErrors(pano);
        return true;
    }

    bool TranslatePanoCmd::processPanorama(HuginBase::Panorama& pano)
    {
        HuginBase::TranslatePanorama(pano, X, Y, Z).run();
        HuginBase::PTools::calcCtrlPointErrors(pano);
        return true;
    }

    bool UpdateFocalLengthCmd::processPanorama(HuginBase::Panorama& pano)
    {
        pano.UpdateFocalLength(imgNrs, m_focalLength);
        HuginBase::PTools::calcCtrlPointErrors(pano);
        return true;
    }

    bool UpdateCropFactorCmd::processPanorama(HuginBase::Panorama& pano)
    {
        //search all image with the same lens, otherwise the crop factor is updated via links, 
        //but not the hfov if the image is not the given HuginBase::UIntSet
        HuginBase::UIntSet allImgWithSameLens;
        HuginBase::UIntSet testedLens;
        HuginBase::StandardImageVariableGroups variable_groups(pano);
        HuginBase::ImageVariableGroup & lenses = variable_groups.getLenses();
        for (HuginBase::UIntSet::const_iterator it = imgNrs.begin(); it != imgNrs.end(); ++it)
        {
            allImgWithSameLens.insert(*it);
            unsigned int lensNr = lenses.getPartNumber(*it);
            if (set_contains(testedLens, lensNr))
            {
                continue;
            };
            testedLens.insert(lensNr);
            for (unsigned int i = 0; i<pano.getNrOfImages(); i++)
            {
                if (lenses.getPartNumber(i) == lensNr)
                {
                    allImgWithSameLens.insert(i);
                };
            };
        };
        pano.UpdateCropFactor(allImgWithSameLens, m_cropFactor);
        HuginBase::PTools::calcCtrlPointErrors(pano);
        return true;
    }

    bool ChangePartNumberCmd::processPanorama(HuginBase::Panorama& pano)
    {
        // it might change as we are setting them
        std::size_t new_new_part_number = new_part_number;
        HuginBase::ImageVariableGroup group(variables, pano);
        for (HuginBase::UIntSet::iterator it = image_numbers.begin(); it != image_numbers.end(); ++it)
        {
            group.switchParts(*it, new_new_part_number);
            // update the lens number if it changes.
            new_new_part_number = group.getPartNumber(*it);
        }
        return true;
    }

    bool ChangePartImagesLinkingCmd::processPanorama(HuginBase::Panorama& pano)
    {
        HuginBase::ImageVariableGroup group(groupVariables, pano);
        if (new_linked_state)
        {
            for (HuginBase::UIntSet::iterator imageIt = image_numbers.begin(); imageIt != image_numbers.end(); ++imageIt)
            {
                // link the variables
                for (std::set<HuginBase::ImageVariableGroup::ImageVariableEnum>::iterator variableIt = changeVariables.begin();
                    variableIt != changeVariables.end(); ++variableIt)
                {
                    group.linkVariableImage(*variableIt, *imageIt);
                }
            }
        }
        else
        {
            for (HuginBase::UIntSet::iterator imageIt = image_numbers.begin(); imageIt != image_numbers.end(); ++imageIt)
            {
                // unlink the variable
                for (std::set<HuginBase::ImageVariableGroup::ImageVariableEnum>::iterator variableIt = changeVariables.begin();
                    variableIt != changeVariables.end(); ++variableIt)
                {
                    group.unlinkVariableImage(*variableIt, *imageIt);
                    group.updatePartNumbers();
                }
            }
        }
        return true;
    }

    bool LinkLensVarsCmd::processPanorama(HuginBase::Panorama& pano)
    {
        HuginBase::StandardImageVariableGroups variable_groups(pano);
        HuginBase::ImageVariableGroup & lenses = variable_groups.getLenses();
        std::set<HuginBase::ImageVariableGroup::ImageVariableEnum>::iterator it;
        for (it = variables.begin(); it != variables.end(); ++it)
        {
            lenses.linkVariablePart(*it, lens_number);
        }
        return true;
    }

    /// @todo avoid copying image data in processPanorama
#define image_variable( name, type, default_value )\
    bool ChangeImage##name##Cmd::processPanorama(HuginBase::Panorama& pano)\
        {\
        for (HuginBase::UIntSet::iterator it = image_numbers.begin(); it != image_numbers.end(); it++)\
        {\
            HuginBase::SrcPanoImage img = pano.getSrcImage(*it);\
            img.set##name(value);\
            pano.setSrcImage(*it, img);\
        }\
        return true;\
        };
#include <panodata/image_variables.h>
#undef image_variable

    bool NewPartCmd::processPanorama(HuginBase::Panorama& pano)
    {
        // unlink all the variables in the first image.
        DEBUG_ASSERT(!image_numbers.empty());
        unsigned int image_index = *image_numbers.begin();
        for (std::set<HuginBase::ImageVariableGroup::ImageVariableEnum>::iterator it = vars.begin(); it != vars.end(); ++it)
        {
            switch (*it)
            {
#define image_variable( name, type, default_value )\
                    case HuginBase::ImageVariableGroup::IVE_##name:\
                        pano.unlinkImageVariable##name(image_index);\
                        break;
#include <panodata/image_variables.h>
#undef image_variable
            }
        }
        // now the first image should have a new part in the group.
        // we want to switch the rest of the images to the new part.
        HuginBase::ImageVariableGroup group(vars, pano);
        for (HuginBase::UIntSet::iterator it = ++image_numbers.begin(); it != image_numbers.end(); ++it)
        {
            std::size_t part_number = group.getPartNumber(image_index);
            group.switchParts(*it, part_number);
        }
        return true;
    }

    bool UpdateMaskForImgCmd::processPanorama(HuginBase::Panorama& pano)
    {
        pano.updateMasksForImage(m_img, m_mask);
        return true;
    }

    bool UpdateWhiteBalance::processPanorama(HuginBase::Panorama& pano)
    {
        pano.updateWhiteBalance(m_red, m_blue);
        return true;
    }

    bool ResetToMeanExposure::processPanorama(HuginBase::Panorama& pano)
    {
        HuginBase::PanoramaOptions opts = pano.getOptions();
        opts.outputExposureValue = HuginBase::CalculateMeanExposure::calcMeanExposure(pano);
        pano.setOptions(opts);
        return true;
    }

    bool DistributeImagesCmd::processPanorama(HuginBase::Panorama& pano)
    {
        const size_t nrImages = pano.getNrOfImages();
        if (nrImages>0)
        {
            const HuginBase::SrcPanoImage& img = pano.getImage(0);
            const double hfov = img.getHFOV();
            size_t imgsPerRow;
            //distribute all images
            //for rectilinear images calculate number of rows
            if (img.getProjection() == HuginBase::SrcPanoImage::RECTILINEAR)
            {
                imgsPerRow = std::max(3, int(360 / (0.8*hfov)));
                imgsPerRow = std::min(imgsPerRow, nrImages);
            }
            else
            {
                //all other images do in one row to prevent cluttered images with fisheye images and the like
                imgsPerRow = nrImages;
            };
            double offset = 0.75*hfov;
            if ((imgsPerRow - 1.0)*offset>360)
            {
                offset = 360 / (imgsPerRow - 1.0);
            };
            double yaw = -(imgsPerRow - 1.0) / 2.0*offset;
            double pitch = 0;
            if (imgsPerRow<nrImages)
            {
                pitch = (-(std::ceil(double(nrImages) / double(imgsPerRow)) - 1.0) / 2.0*offset);
            };
            HuginBase::VariableMapVector varsVec = pano.getVariables();
            size_t counter = 0;
            for (size_t i = 0; i<nrImages; i++)
            {
                HuginBase::VariableMap::iterator it = varsVec[i].find("y");
                if (it != varsVec[i].end())
                {
                    it->second.setValue(yaw);
                };
                it = varsVec[i].find("p");
                if (it != varsVec[i].end())
                {
                    it->second.setValue(pitch);
                };
                yaw += offset;
                counter++;
                if (counter == imgsPerRow)
                {
                    counter = 0;
                    pitch += offset;
                    yaw = -(imgsPerRow - 1.0) / 2.0*offset;
                };
            };
            pano.updateVariables(varsVec);
        };
        return true;
    }

} // namespace PanoCommand
