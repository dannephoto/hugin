// -*- c-basic-offset: 4 -*-
/** @file OverviewCameraTool.h
 *
 *  @author Darko Makreshanski
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
#include "OverviewCameraTool.h"
#include "GLViewer.h"

const double PanosphereOverviewCameraTool::limit_low = 1.2;
const double PanosphereOverviewCameraTool::limit_high = 5.0;

void PanosphereOverviewCameraTool::Activate()
{
    helper->NotifyMe(ToolHelper::MOUSE_MOVE, this);
    helper->NotifyMe(ToolHelper::MOUSE_PRESS, this);
    helper->NotifyMe(ToolHelper::MOUSE_WHEEL, this);
    helper->NotifyMe(ToolHelper::KEY_PRESS, this);
    down = false;
}



void PanosphereOverviewCameraTool::MouseMoveEvent(double x, double y, wxMouseEvent & e)
{
    if (down)
    {
        if (e.ButtonIsDown(wxMOUSE_BTN_ANY))
        {
            hugin_utils::FDiff2D pos = helper->GetMouseScreenPosition();
            PanosphereOverviewVisualizationState* state = static_cast<PanosphereOverviewVisualizationState*>(helper->GetVisualizationStatePtr());
            //FIXME: include a scale factor for the panosphere
            double scale = (state->getR() - state->getSphereRadius()) / 40000.0;
            if (state->isInsideView())
            {
                if (e.ButtonIsDown(wxMOUSE_BTN_MIDDLE))
                {
                    // invert mouse for middle button panning in inside view
                    state->setAngX(start_angx - (pos.x - start_x) * scale);
                    double ey = (pos.y - start_y) * scale + start_angy;
                    if (ey >= M_PI / 2.0) { ey = M_PI / 2.0 - 0.0001; }
                    if (ey <= -M_PI / 2.0) { ey = -M_PI / 2.0 + 0.0001; }
                    state->setAngY(ey);
                }
                else
                {
                    state->setAngX((pos.x - start_x) * scale + start_angx);
                    double ey = start_angy - (pos.y - start_y) * scale;
                    if (ey >= M_PI / 2.0) { ey = M_PI / 2.0 - 0.0001; }
                    if (ey <= -M_PI / 2.0) { ey = -M_PI / 2.0 + 0.0001; }
                    state->setAngY(ey);
                };
            }
            else
            {
                // outside view
                state->setAngX((pos.x - start_x) * scale + start_angx);
                double ey = (pos.y - start_y) * scale + start_angy;
                if (ey >= M_PI / 2.0) { ey = M_PI / 2.0 - 0.0001; }
                if (ey <= -M_PI / 2.0) { ey = -M_PI / 2.0 + 0.0001; }
                state->setAngY(ey);
            };
            state->Redraw();
        }
        else
        {
            // no button pressed any more, reset flag
            down = false;
        };
    };
}

void PanosphereOverviewCameraTool::MouseButtonEvent(wxMouseEvent &e)
{
//    DEBUG_DEBUG("mouse ov drag button");
    if (e.ButtonDown())
    {
        PanosphereOverviewVisualizationState* state = static_cast<PanosphereOverviewVisualizationState*>(helper->GetVisualizationStatePtr());
        if (state->isInsideView() || (!helper->IsMouseOverPano() || e.CmdDown() || e.AltDown() || e.MiddleDown()))
        {
            down = true;
            hugin_utils::FDiff2D pos = helper->GetMouseScreenPosition();
            start_x = pos.x;
            start_y = pos.y;
            start_angx = state->getAngX();
            start_angy = state->getAngY();
        };
    };
    if (e.ButtonUp())
    {
        if (down)
        {
            down = false;
        };
    };
}


void PanosphereOverviewCameraTool::ChangeZoomLevel(bool zoomIn, double scale) {
    PanosphereOverviewVisualizationState*  state = static_cast<PanosphereOverviewVisualizationState*>(helper->GetVisualizationStatePtr());
    double radius = state->getSphereRadius();
    if (zoomIn) {
        if (state->getR() > limit_low * radius) {
            state->setR((state->getR() - radius) / scale + radius);
            state->SetDirtyViewport();
            state->ForceRequireRedraw();
            state->Redraw();
        }
    } else {
        if (state->getR() < limit_high * radius) {
            state->setR((state->getR() - radius) * scale + radius);
            state->SetDirtyViewport();
            state->ForceRequireRedraw();
            state->Redraw();
        }
    }
}

void PanosphereOverviewCameraTool::ChangeFOV(bool zoomIn)
{
    PanosphereOverviewVisualizationState* state = static_cast<PanosphereOverviewVisualizationState*>(helper->GetVisualizationStatePtr());
    if (zoomIn)
    {
        state->setFOV(state->getFOV() / 1.1);
        state->SetDirtyViewport();
        state->ForceRequireRedraw();
        state->Redraw();
    }
    else
    {
        state->setFOV(state->getFOV() * 1.1);
        state->SetDirtyViewport();
        state->ForceRequireRedraw();
        state->Redraw();
    };
}
void PanosphereOverviewCameraTool::MouseWheelEvent(wxMouseEvent &e)
{
    if (e.GetWheelRotation() != 0)
    {
        if (static_cast<PanosphereOverviewVisualizationState*>(helper->GetVisualizationStatePtr())->isInsideView())
        {
            ChangeFOV(e.GetWheelRotation() > 0);
        }
        else
        {
            ChangeZoomLevel(e.GetWheelRotation() > 0);
        };
    }
}

void PanosphereOverviewCameraTool::KeypressEvent(int keycode, int modifiers, bool pressed) {
//    std::cout << "kc: " << keycode << " " << modifiers << std::endl;
//    std::cout << "cmd: " << wxMOD_CMD << " " << wxMOD_CONTROL << " " << WXK_ADD << " " << WXK_SUBTRACT << std::endl;
//    std::cout << "cmd: " << wxMOD_CMD << " " << wxMOD_CONTROL << " " << WXK_NUMPAD_ADD << " " << WXK_NUMPAD_SUBTRACT << std::endl;
    if (pressed)
    if (modifiers == wxMOD_CMD) {
        if (keycode == WXK_ADD) {
            ChangeZoomLevel(true);
        } else if (keycode == WXK_SUBTRACT) {
            ChangeZoomLevel(false);
        }
    }
}

void PlaneOverviewCameraTool::Activate()
{
    helper->NotifyMe(ToolHelper::MOUSE_MOVE, this);
    helper->NotifyMe(ToolHelper::MOUSE_PRESS, this);
    helper->NotifyMe(ToolHelper::MOUSE_WHEEL, this);
    down = false;
}

void PlaneOverviewCameraTool::MouseMoveEvent(double x, double y, wxMouseEvent & e)
{
    if (down) {
        PlaneOverviewVisualizationState*  state = static_cast<PlaneOverviewVisualizationState*>(helper->GetVisualizationStatePtr());
//
        //same code as in tool helper to get position on the z-plane but with initial position
        double d = state->getR();

        int tcanv_w, tcanv_h;
        state->GetViewer()->GetClientSize(&tcanv_w,&tcanv_h);

        double canv_w, canv_h;
        canv_w = tcanv_w;
        canv_h = tcanv_h;
        
        double fov = state->getFOV();

        double fovy, fovx;
        if (canv_w > canv_h) {
            fovy = DEG_TO_RAD(fov);
            fovx = 2 * atan( tan(fovy / 2.0) * canv_w / canv_h);
        } else {
            fovx = DEG_TO_RAD(fov);
            fovy = 2 * atan( tan(fovx / 2.0) * canv_h / canv_w);
        }

        double vis_w, vis_h;
        vis_w = 2.0 * tan ( fovx / 2.0 ) * d;
        vis_h = 2.0 * tan ( fovy / 2.0 ) * d;

        //position of the mouse on the z=0 plane
        double prim_x, prim_y;
        prim_x = (double) x / canv_w * vis_w - vis_w / 2.0 + start_pos_x;
        prim_y = ((double) y / canv_h * vis_h - vis_h / 2.0 - start_pos_y);

//        DEBUG_DEBUG("mouse ov tool 1 " << state->getX() << " " << state->getY());
        state->setX((-prim_x + start_x) + start_pos_x);
        state->setY((prim_y - start_y) + start_pos_y);
//        DEBUG_DEBUG("mouse ov tool 2 " << state->getX() << " " << state->getY());
        state->ForceRequireRedraw();
        state->Redraw();
    }
}

void PlaneOverviewCameraTool::MouseButtonEvent(wxMouseEvent &e)
{
    PlaneOverviewToolHelper * thelper = static_cast<PlaneOverviewToolHelper*>(helper);
    PlaneOverviewVisualizationState*  state = static_cast<PlaneOverviewVisualizationState*>(helper->GetVisualizationStatePtr());
//    DEBUG_DEBUG("mouse ov drag button");
    if (((e.CmdDown() || e.AltDown()) && e.LeftDown()) || e.MiddleDown()) {
        down = true;
        start_x = thelper->getPlaneX();
        start_y = thelper->getPlaneY();
        start_pos_x = state->getX();
        start_pos_y = state->getY();
    }
    if (e.LeftUp() || e.MiddleUp()) {
        if (down) {
            down = false;
        }
    }
}

void PlaneOverviewCameraTool::ChangeZoomLevel(bool zoomIn, double scale) {
    PlaneOverviewVisualizationState*  state = static_cast<PlaneOverviewVisualizationState*>(helper->GetVisualizationStatePtr());
    if (zoomIn) {
        state->setR(state->getR() / scale);
    } else {
        state->setR(state->getR() * scale);
    }
    state->SetDirtyViewport();
    state->ForceRequireRedraw();
    state->Redraw();
}

void PlaneOverviewCameraTool::MouseWheelEvent(wxMouseEvent &e)
{
    if (e.GetWheelRotation() != 0) {
        ChangeZoomLevel(e.GetWheelRotation() > 0);
    }
}

void PlaneOverviewCameraTool::KeypressEvent(int keycode, int modifiers, bool pressed) {
    if (pressed)
    if (modifiers == wxMOD_CMD) {
        if (keycode == WXK_ADD) {
            ChangeZoomLevel(true);
        } else if (keycode == WXK_SUBTRACT) {
            ChangeZoomLevel(false);
        }
    }
}


