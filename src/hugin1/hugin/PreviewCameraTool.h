// -*- c-basic-offset: 4 -*-
/** @file PreviewCameraTool.h
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


#ifndef __PREVIEW_CAMERA_TOOL_H__
#define __PREVIEW_CAMERA_TOOL_H__

#include "Tool.h"
#include "ToolHelper.h"


/**
 * tool for the manipulation of the opengl 'camera' properties
 * It handles zooming in/out of the main preview window
 */
class PreviewCameraTool : public PreviewTool
{
    public:
        explicit PreviewCameraTool(PreviewToolHelper* helper) : PreviewTool (helper) {};
        virtual ~PreviewCameraTool() {};

        void Activate();
        /** event for mouse wheel processing */
        void MouseWheelEvent(wxMouseEvent &);
        /** event for mouse button processing */
        void MouseButtonEvent(wxMouseEvent &e);
        /** event for mouse move */
        void MouseMoveEvent(double x, double y, wxMouseEvent& e);
    private:
        /** update zoom level */
        void ChangeZoomLevel(bool zoomIn, const hugin_utils::FDiff2D& scrollPos);
        /** update center, used for panning */
        void UpdateCenter(const hugin_utils::FDiff2D& moveDist);

        /** flag to indicate that we are moving the view with middle mouse button */
        bool m_moving;
        /** last screen position during moving , needed for real-time update */
        hugin_utils::FDiff2D m_lastScreenPos;
};

#endif /* __PREVIEW_CAMERA_TOOL_H__ */

