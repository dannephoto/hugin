// -*- c-basic-offset: 4 -*-

/** @file PreviewEditCPTool.cpp
 *
 *  @author T. Modes
 *
 *  @brief implementation of ToolHelper for editing control points in the pano space
 *
 */

/*
 *  This program is free software; you can redistribute it and/or
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

#include "panoinc_WX.h"
#include "panoinc.h"
#include "hugin/PreviewEditCPTool.h"
#include <map>
#include "base_wx/CommandHistory.h"
#include "base_wx/wxPanoCommand.h"

#include <wx/platform.h>
#ifdef __WXMAC__
#include <OpenGL/gl.h>
#else
#ifdef __WXMSW__
#include <vigra/windows.h>
#endif
#include <GL/gl.h>
#endif

#include "GLPreviewFrame.h"

// we want to handle the mouse events and to draw over the panorama when in use, so make sure we get notice.
void PreviewEditCPTool::Activate()
{
    helper->NotifyMe(PreviewToolHelper::MOUSE_PRESS, this);
    helper->NotifyMe(PreviewToolHelper::MOUSE_MOVE, this);
    helper->NotifyMe(PreviewToolHelper::REALLY_DRAW_OVER_IMAGES, this);
    m_mouseDown = false;
    helper->SetStatusMessage(_("Drag a rectangle and select the desired action for the control points in the selected rectangle. Alt + drag to create a line control point."));
};

// The panorama has been drawn, now draw the selection rectangle.
void PreviewEditCPTool::ReallyAfterDrawImagesEvent()
{
    if (m_mouseDown)
    {
        glDisable(GL_TEXTURE_2D);
        const hugin_utils::FDiff2D dist(m_currentPosScreen - m_startPosScreen);
        // select color depending on size
        if (m_line || (fabs(dist.x) > 10 && fabs(dist.y) > 10))
        {
            glColor3f(1.0f, 1.0f, 0.0f);
        }
        else
        {
            // selection is too small, use a gray color
            glColor3f(0.7f, 0.7f, 0.7f);
        }
        if (m_line)
        {
            glBegin(GL_LINES);
            glVertex2f(m_startPos.x, m_startPos.y);
            glVertex2f(m_currentPos.x, m_currentPos.y);
            glEnd();
        }
        else
        {
            glBegin(GL_LINES);
            glVertex2f(m_startPos.x, m_startPos.y);
            glVertex2f(m_currentPos.x, m_startPos.y);
            glVertex2f(m_currentPos.x, m_startPos.y);
            glVertex2f(m_currentPos.x, m_currentPos.y);
            glVertex2f(m_currentPos.x, m_currentPos.y);
            glVertex2f(m_startPos.x, m_currentPos.y);
            glVertex2f(m_startPos.x, m_currentPos.y);
            glVertex2f(m_startPos.x, m_startPos.y);
            glEnd();
        };
        glEnable(GL_TEXTURE_2D);
    };
};

// update selection rectangle
void PreviewEditCPTool::MouseMoveEvent(double x, double y, wxMouseEvent & e)
{
    if (m_mouseDown)
    {
        m_currentPos = helper->GetMousePanoPosition();
        m_currentPosScreen = helper->GetMouseScreenPosition();
        // force redrawing
        helper->GetVisualizationStatePtr()->ForceRequireRedraw();
        helper->GetVisualizationStatePtr()->Redraw();
    };
}

// handle mouse buttons
void PreviewEditCPTool::MouseButtonEvent(wxMouseEvent &e)
{
    if (e.GetButton() && m_menuPopup)
    {
        // catch first mouse click after menu popup
        // to correctly handle dismiss of popup menu
        m_menuPopup = false;
    }
    else
    {
        if (e.ButtonDown(wxMOUSE_BTN_LEFT) && !m_mouseDown)
        {
            // start selecting rectangle
            m_mouseDown = true;
            m_startPos = helper->GetMousePanoPosition();
            m_startPosScreen = helper->GetMouseScreenPosition();
            m_CPinROI.clear();
            m_line = e.AltDown();
        }
        else
        {
            if (e.ButtonUp(wxMOUSE_BTN_LEFT) && m_mouseDown)
            {
                // finish selecting
                m_mouseDown = false;
                m_currentPos = helper->GetMousePanoPosition();
                m_currentPosScreen = helper->GetMouseScreenPosition();
                if (m_line)
                {
                    // now add the line cp
                    AddLineCP(m_startPos, m_currentPos);
                }
                else
                {
                    const hugin_utils::FDiff2D dist(m_currentPosScreen - m_startPosScreen);
                    if (fabs(dist.x) > 10 && fabs(dist.y) > 10)
                    {
                        // build menu
                        wxMenu menu;
                        const vigra::Rect2D roi = GetSelectedROI();
                        // selected roi should have at least 10 pixel width and height in pano space
                        if (!roi.isEmpty() && roi.width() > 10 && roi.height() > 10)
                        {
                            menu.Append(ID_CREATE_CP, _("Create control points here"));
                        };
                        FindCPInRect(m_startPos, m_currentPos);
                        if (!m_CPinROI.empty())
                        {
                            menu.Append(ID_REMOVE_CP, wxString::Format(_("Remove %lu control points"), static_cast<unsigned long int>(m_CPinROI.size())));
                        };
                        if (!menu.GetMenuItems().IsEmpty())
                        {
                            m_menuPopup = true;
                            helper->GetPreviewFrame()->PopupMenu(&menu);
                        }
                        else
                        {
                            wxBell();
                            helper->GetVisualizationStatePtr()->ForceRequireRedraw();
                            helper->GetVisualizationStatePtr()->Redraw();
                        };
                    }
                    else
                    {
                        wxBell();
                        // we need to redraw so that the selection rectangle vanishes
                        helper->GetVisualizationStatePtr()->ForceRequireRedraw();
                        helper->GetVisualizationStatePtr()->Redraw();
                    };
                };
            };
        };
    };
};

// find all cp in the given rectangle
void PreviewEditCPTool::FindCPInRect(const hugin_utils::FDiff2D& pos1, const hugin_utils::FDiff2D& pos2)
{
    HuginBase::Panorama* pano = helper->GetPanoramaPtr();
    HuginBase::UIntSet activeImages = pano->getActiveImages();
    const hugin_utils::FDiff2D panoPos1(pos1.x < pos2.x ? pos1.x : pos2.x, pos1.y < pos2.y ? pos1.y : pos2.y);
    const hugin_utils::FDiff2D panoPos2(pos1.x > pos2.x ? pos1.x : pos2.x, pos1.y > pos2.y ? pos1.y : pos2.y);
    m_CPinROI.clear();
    if (!activeImages.empty())
    {
        // create transformation objects
        typedef std::map<size_t, HuginBase::PTools::Transform*> TransformMap;
        TransformMap transformations;
        for (HuginBase::UIntSet::iterator it = activeImages.begin(); it != activeImages.end(); ++it)
        {
            HuginBase::PTools::Transform* trans = new HuginBase::PTools::Transform();
            trans->createInvTransform(pano->getImage(*it), pano->getOptions());
            transformations.insert(std::make_pair(*it, trans));
        };
        HuginBase::CPVector cps = pano->getCtrlPoints();
        for (HuginBase::CPVector::const_iterator cpIt = cps.begin(); cpIt != cps.end(); ++cpIt)
        {
            // check cp only if both images are visible
            if (set_contains(activeImages, cpIt->image1Nr) && set_contains(activeImages, cpIt->image2Nr))
            {
                hugin_utils::FDiff2D pos1;
                hugin_utils::FDiff2D pos2;
                // remove all control points, where both points are inside the given rectangle
                if (transformations[cpIt->image1Nr]->transformImgCoord(pos1, hugin_utils::FDiff2D(cpIt->x1, cpIt->y1)) &&
                    transformations[cpIt->image2Nr]->transformImgCoord(pos2, hugin_utils::FDiff2D(cpIt->x2, cpIt->y2)))
                {
                    if (panoPos1.x <= pos1.x && pos1.x <= panoPos2.x && panoPos1.y <= pos1.y && pos1.y <= panoPos2.y &&
                        panoPos1.x <= pos2.x && pos2.x <= panoPos2.x && panoPos1.y <= pos2.y && pos2.y <= panoPos2.y)
                    {
                        m_CPinROI.insert(cpIt - cps.begin());
                    };
                };
            };
        };
        for (TransformMap::iterator it = transformations.begin(); it != transformations.end(); ++it)
        {
            delete it->second;
        };
    };
};

// return the selected ROI
vigra::Rect2D PreviewEditCPTool::GetSelectedROI()
{
    vigra::Point2D p1;
    p1.x = std::min(m_startPos.x, m_currentPos.x);
    p1.y = std::min(m_startPos.y, m_currentPos.y);
    vigra::Point2D p2;
    p2.x = std::max(m_startPos.x, m_currentPos.x);
    p2.y = std::max(m_startPos.y, m_currentPos.y);
    return vigra::Rect2D(p1, p2) & vigra::Rect2D(helper->GetPanoramaPtr()->getOptions().getSize());
};

// for correctly handling the popup menu 
// when selecting the popup menu or dismiss the popup menu we get an additional mouse
// click, we need to ignore this one, this function sets the state back
void PreviewEditCPTool::SetMenuProcessed()
{
    m_menuPopup = false;
};

void PreviewEditCPTool::AddLineCP(const hugin_utils::FDiff2D& pos1, const hugin_utils::FDiff2D& pos2)
{
    HuginBase::UIntSet imgs1 = helper->GetImagesUnderPos(pos1);
    HuginBase::UIntSet imgs2 = helper->GetImagesUnderPos(pos2);
    HuginBase::UIntSet imgIntersection;
    std::set_intersection(imgs1.begin(), imgs1.end(), imgs2.begin(), imgs2.end(), std::inserter(imgIntersection, imgIntersection.begin()));
    if (!imgIntersection.empty())
    {
        // start and end point are on the same image
        HuginBase::Panorama* pano = helper->GetPanoramaPtr();
        HuginBase::ControlPoint cp;
        cp.image1Nr = *imgIntersection.begin();

        HuginBase::PTools::Transform transform;
        transform.createTransform(pano->getImage(cp.image1Nr), pano->getOptions());
        double image_x, image_y;
        transform.transformImgCoord(image_x, image_y, pos1.x, pos1.y);
        cp.x1 = image_x;
        cp.y1 = image_y;
        transform.transformImgCoord(image_x, image_y, pos2.x, pos2.y);
        cp.image2Nr = cp.image1Nr;
        cp.x2 = image_x;
        cp.y2 = image_y;
        if (abs(pos1.x - pos2.x) < abs(pos1.y - pos2.y))
        {
            cp.mode = HuginBase::ControlPoint::X;
        }
        else
        {
            cp.mode = HuginBase::ControlPoint::Y;
        };
        PanoCommand::GlobalCmdHist::getInstance().addCommand(new PanoCommand::AddCtrlPointCmd(*pano, cp));
    }
    else
    {
        wxBell();
        // we need to redraw so that the selection vanishes
        helper->GetVisualizationStatePtr()->ForceRequireRedraw();
        helper->GetVisualizationStatePtr()->Redraw();
    };
}