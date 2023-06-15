// -*- c-basic-offset: 4 -*-

/** @file SplitButton.cpp
 *
 *  @brief implementation of a SplitButton
 *
 *  @author T. Modes
 *
 *  implementation based an stackexchange answer from Anil8753
 *  see https://stackoverflow.com/questions/37076881/push-dropdown-button-wxwidgets
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

#include "SplitButton.h"
#include <wx/event.h>
#include <wx/renderer.h>
#include <algorithm>

bool SplitButton::Create(wxWindow* parent, wxWindowID id, const wxString& label, const wxPoint& pos, const wxSize& size, const wxString& name)
{
    if (!wxPanel::Create(parent, id, pos, size, wxBORDER_NONE | wxTAB_TRAVERSAL))
    {
        return false;
    }
    m_label = label;

    if (size == wxDefaultSize)
    {
        UpdateMinSize();
    }

    Bind(wxEVT_PAINT, &SplitButton::OnPaint, this);
    Bind(wxEVT_LEFT_UP, &SplitButton::OnLeftButtonUp, this);
    Bind(wxEVT_LEFT_DOWN, &SplitButton::OnLeftButtonDown, this);
    Bind(wxEVT_KILL_FOCUS, &SplitButton::OnKillFocus, this);
    Bind(wxEVT_LEAVE_WINDOW, &SplitButton::OnMouseLeave, this);
    Bind(wxEVT_ENTER_WINDOW, &SplitButton::OnMouseEnter, this);

    m_dropDownMenu = new wxMenu();
    return true;
}

SplitButton::~SplitButton()
{
    delete m_dropDownMenu;
    m_dropDownMenu = nullptr;
}

wxMenu* SplitButton::GetSplitButtonMenu()
{
    return m_dropDownMenu;
}

void SplitButton::OnKillFocus(wxFocusEvent& event)
{
    m_stateButton = 0;
    m_stateMenu = 0;
    Refresh();

    event.Skip();
}

void SplitButton::OnMouseLeave(wxMouseEvent& event)
{
    m_stateButton = 0;
    m_stateMenu = 0;
    Refresh();

    event.Skip();
}

void SplitButton::OnMouseEnter(wxMouseEvent& event)
{
    m_stateButton = wxCONTROL_CURRENT;
    m_stateMenu = wxCONTROL_CURRENT;
    Refresh();

    event.Skip();
}

void SplitButton::OnLeftButtonUp(wxMouseEvent& event)
{
    m_stateButton = 0;
    m_stateMenu = 0;

    Refresh();

    int x = -1;
    int y = -1;
    event.GetPosition(&x, &y);

    if (x < (GetSize().GetWidth() - m_arrowButtonWidth))
    {
        wxEvtHandler* pEventHandler = GetEventHandler();
        wxASSERT(pEventHandler);

        pEventHandler->CallAfter([=]()
            {
                wxCommandEvent evt(wxEVT_BUTTON, this->GetId());
                evt.SetEventObject(this);
                GetEventHandler()->ProcessEvent(evt);
            });
    }

    event.Skip();
}

void SplitButton::OnLeftButtonDown(wxMouseEvent& event)
{
    int x = -1;
    int y = -1;
    event.GetPosition(&x, &y);

    if (x >= (GetSize().GetWidth() - m_arrowButtonWidth))
    {
        m_stateButton = 0;
        m_stateMenu = wxCONTROL_PRESSED;
        Refresh();

        wxSize size = GetSize();
        wxPoint position;
        position.x = 0;
        position.y = size.GetHeight();
        // draw menu only when menu is assigned and has items
        if (m_dropDownMenu && m_dropDownMenu->GetMenuItemCount() > 0)
        {
            PopupMenu(m_dropDownMenu, position);
        };

        m_stateMenu = 0;
        Refresh();
    }
    else
    {
        m_stateButton = wxCONTROL_PRESSED;
        m_stateMenu = wxCONTROL_PRESSED;
        Refresh();
    }

    event.Skip();
}

void SplitButton::OnPaint(wxPaintEvent& WXUNUSED(event))
{
    wxPaintDC dc(this);
    wxSize size = GetSize();
    const int width = size.GetWidth() - m_arrowButtonWidth;

    // Draw first part of button
    wxRect r1;
    r1.x = 0;
    r1.y = 0;
    r1.width = width + 2;
    r1.height = size.GetHeight();

    wxRendererNative::Get().DrawPushButton(this, dc, r1, m_stateButton);

    // Determine text color based on dark mode
    wxColour textColor;
#ifdef __WXMAC__
    bool darkModeEnabled = wxSystemSettings::GetAppearance().IsDark();
    if (darkModeEnabled)
        textColor = m_IsEnabled ? wxColour(0, 0, 0) : wxColour(192, 192, 192); // Black color for dark mode, if not enabled use gray
    else
        textColor = wxSystemSettings::GetColour(m_IsEnabled ? wxSYS_COLOUR_BTNTEXT : wxSYS_COLOUR_GRAYTEXT);
#else
    textColor = wxSystemSettings::GetColour(m_IsEnabled ? wxSYS_COLOUR_BTNTEXT : wxSYS_COLOUR_GRAYTEXT);
#endif

    dc.SetTextForeground(textColor);

    // draw label and bitmap
    if (HasBitmap())
    {
        dc.DrawLabel(m_label, m_bitmap, r1, wxALIGN_CENTER);
    }
    else
    {
        dc.DrawLabel(m_label, r1, wxALIGN_CENTER);
    };

    // Draw second part of button
    wxRect r2;
    r2.x = width - 2;
    r2.y = 0;
    r2.width = m_arrowButtonWidth;
    r2.height = size.GetHeight();

    wxRendererNative::Get().DrawPushButton(this, dc, r2, m_stateMenu);
    wxRendererNative::Get().DrawDropArrow(this, dc, r2, m_stateMenu);
}

bool SplitButton::HasBitmap() const
{
    return m_bitmap.IsOk() && m_bitmap.GetWidth() > 0 && m_bitmap.GetHeight() > 0;
}

void SplitButton::UpdateMinSize()
{
    wxSize size = wxButton::GetDefaultSize();
    wxSize textSize = GetTextExtent(m_label);
    // add size of drop down button
    textSize.SetWidth(textSize.GetWidth() + m_arrowButtonWidth + 20);
    wxSize bitmapSize;
    if (HasBitmap())
    {
        bitmapSize = m_bitmap.GetSize();
#ifdef __WXMSW__
        // add some border to match the size of other buttons
        bitmapSize.IncBy(0, 8);
#endif
    };
    size.SetWidth(std::max(size.GetWidth(), bitmapSize.GetWidth() + textSize.GetWidth()));
    size.SetHeight(std::max(size.GetHeight(), std::max(bitmapSize.GetHeight(), textSize.GetHeight())));
#ifdef __WXGTK__
    // add a little border to match to other buttons
    size.SetHeight(size.GetHeight() + 7);
#endif
    SetMinSize(size);
    InvalidateBestSize();
}

bool SplitButton::Enable(bool enable)
{
    m_IsEnabled = enable;
    wxPanel::Enable(m_IsEnabled);

    if (m_IsEnabled)
    {
        m_stateButton = 0;
        m_stateMenu = 0;
}
    else
    {
        m_stateButton = wxCONTROL_DISABLED;
        m_stateMenu = wxCONTROL_DISABLED;
    }
    Refresh();
    return enable;
}

wxBitmap SplitButton::GetBitmap() const
{
    if (HasBitmap())
    {
        return m_bitmap;
    };
    return wxNullBitmap;
}

void SplitButton::SetBitmap(const wxBitmap& bitmap)
{
    m_bitmap = bitmap;
    // update minimum size
    UpdateMinSize();
}

wxMenu* SplitButton::LoadMenu(const wxString& name)
{
    delete m_dropDownMenu;
    m_dropDownMenu = wxXmlResource::Get()->LoadMenu(name);
    return m_dropDownMenu;
}

void SplitButton::SetLabel(const wxString& label)
{
    m_label = label;
    UpdateMinSize();
}

wxString SplitButton::GetLabel() const
{
    return m_label;
}

SplitButtonXmlHandler::SplitButtonXmlHandler() : wxXmlResourceHandler()
{
    AddWindowStyles();
}

wxObject *SplitButtonXmlHandler::DoCreateResource()
{
    XRC_MAKE_INSTANCE(control, SplitButton)
    control->Create(m_parentAsWindow, GetID(), GetText(wxT("label")), GetPosition(), GetSize(), GetName());
    if (GetParamNode("bitmap"))
    {
        control->SetBitmap(GetBitmap("bitmap", wxART_BUTTON));
    };

    SetupWindow(control);
    return control;
}

bool SplitButtonXmlHandler::CanHandle(wxXmlNode *node)
{
    return IsOfClass(node, wxT("SplitButton"));
}

IMPLEMENT_DYNAMIC_CLASS(SplitButtonXmlHandler, wxXmlResourceHandler)
