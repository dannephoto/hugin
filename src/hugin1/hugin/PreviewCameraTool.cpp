// -*- c-basic-offset: 4 -*-
/** @file PreviewCameraTool.cpp
 *
 *  @author T. Modes
 *
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


#ifdef _WIN32
#include "wx/msw/wrapwin.h"
#endif
#include "PreviewCameraTool.h"
#include "GLViewer.h"

void PreviewCameraTool::Activate()
{
    helper->NotifyMe(ToolHelper::MOUSE_PRESS, this);
    helper->NotifyMe(ToolHelper::MOUSE_MOVE, this);
    helper->NotifyMe(ToolHelper::MOUSE_WHEEL, this);
    m_moving = false;
}

void PreviewCameraTool::ChangeZoomLevel(bool zoomIn, const hugin_utils::FDiff2D& scrollPos)
{
    VisualizationState*  state = static_cast<VisualizationState*>(helper->GetVisualizationStatePtr());
    if (zoomIn)
    {
        state->SetZoomLevel((state->GetZoomLevel()) * 1.2);
        // update center to zoom in at given position
        hugin_utils::FDiff2D newScrollPos;
        newScrollPos.x = scrollPos.x / state->GetOptions()->getWidth();
        newScrollPos.y = scrollPos.y / state->GetOptions()->getHeight();
        state->SetViewingCenter(newScrollPos);
    }
    else
    {
        // out-zoom, keep current position
        state->SetZoomLevel((state->GetZoomLevel()) / 1.2);
    };
    // redraw
    state->ForceRequireRedraw();
    state->Redraw();
}

void PreviewCameraTool::MouseWheelEvent(wxMouseEvent &e)
{
    if (e.GetWheelRotation() != 0 && helper->IsMouseOverPano())
    {
        ChangeZoomLevel(e.GetWheelRotation() > 0, helper->GetMousePanoPosition());
    }
}

void PreviewCameraTool::UpdateCenter(const hugin_utils::FDiff2D& moveDist)
{
    VisualizationState* state = static_cast<VisualizationState*>(helper->GetVisualizationStatePtr());
    // move center by given distance
    hugin_utils::FDiff2D currentCenter = state->GetViewingCenter();
    currentCenter.x -= moveDist.x / state->GetOptions()->getWidth() / state->GetScale();
    currentCenter.y -= moveDist.y / state->GetOptions()->getHeight() / state->GetScale();
    state->SetViewingCenter(currentCenter);
    state->SetDirtyViewport();
    state->ForceRequireRedraw();
    state->Redraw();
}

void PreviewCameraTool::MouseButtonEvent(wxMouseEvent& e)
{
    // listen only to middle button events
    if (!m_moving && e.MiddleDown())
    {
        if (helper->IsMouseOverPano())
        {
            // start dragging
            m_moving = true;
            m_lastScreenPos = helper->GetMouseScreenPosition();
        };
    }
    else
    {
        if (m_moving && e.MiddleUp())
        {
            // end dragging, update center
            m_moving = false;
            if (helper->IsMouseOverPano())
            {
                UpdateCenter(helper->GetMouseScreenPosition() - m_lastScreenPos);
            };
        };
    };
}

void PreviewCameraTool::MouseMoveEvent(double x, double y, wxMouseEvent& e)
{
    if (m_moving)
    {
        // mouse is moving, if middle mouse button is pressed update center in real time
        if (e.MiddleIsDown())
        {
            if (helper->IsMouseOverPano())
            {
                const hugin_utils::FDiff2D currentPos = helper->GetMouseScreenPosition();
                const hugin_utils::FDiff2D diffPos = currentPos - m_lastScreenPos;
                if (diffPos.squareLength() > 100)
                {
                    // update only when mouse moved at least 10 pixel
                    UpdateCenter(diffPos);
                    m_lastScreenPos = currentPos;
                };
            };
        }
        else
        {
            // we are still moving, but middle button is not pressed any more
            // so reset moving bit
            m_moving = false;
        };
    };
}
