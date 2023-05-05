// -*- c-basic-offset: 4 -*-
/** @file SplitButton.h
 *
 *  @author T. Modes
 *
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

#ifndef __SPLIT_BUTTON_H__
#define __SPLIT_BUTTON_H__

#include <wx/wx.h>
#include <wx/menu.h>
#include <wx/xrc/xmlres.h>

class SplitButton : public wxPanel
{
public:
    /** constructor */
    SplitButton():wxPanel() {};
    SplitButton(wxWindow* parent,
        wxWindowID id = wxID_ANY,
        const wxString& label = wxEmptyString,
        const wxPoint& pos = wxDefaultPosition,
        const wxSize& size = wxDefaultSize,
        const wxString& name = wxT("splitbutton")
    )
    {
        Create(parent, id, label, pos, size, name);
    };
    /** for two-step creation */
    bool Create(wxWindow* parent, 
        wxWindowID id = wxID_ANY,
        const wxString& label = wxEmptyString,
        const wxPoint& pos = wxDefaultPosition, 
        const wxSize& size = wxDefaultSize, 
        const wxString& name = wxT("splitbutton")
    );
    ~SplitButton();
    /** returns a pointer to the drop down menu */
    wxMenu* GetSplitButtonMenu();
    /** enable or disable the control */
    bool Enable(bool enable = true) override;
    /** returns the currently active bitmap */
    wxBitmap GetBitmap() const;
    /** sets bitmap and direction */
    void SetBitmap(const wxBitmap& bitmap);
    /** loads the drop down menu from the XRC ressource */
    wxMenu* LoadMenu(const wxString& name);
    /** set the label string */
    void SetLabel(const wxString& label);
    /** return the label of the button */
    wxString GetLabel() const;

protected:
    /** some event handler to mimic default mouse over behaviour */
    void OnKillFocus(wxFocusEvent& event);
    void OnMouseLeave(wxMouseEvent& event);
    void OnMouseEnter(wxMouseEvent& event);
    /** handle left mouse button on button and drop down button */
    void OnLeftButtonUp(wxMouseEvent& event);
    void OnLeftButtonDown(wxMouseEvent& event);
    /** paint handler to draw to the button */
    void OnPaint(wxPaintEvent& WXUNUSED(event));
    /** return true if buttons has a bitmap assigned */
    bool HasBitmap() const;
    /** update the internal size calculation */
    void UpdateMinSize();

private:
    int m_stateButton = 0;
    int m_stateMenu = 0;
    bool m_IsEnabled = true;
    const int m_arrowButtonWidth = 20;
    wxString m_label;
    wxMenu* m_dropDownMenu = nullptr;
    wxBitmap m_bitmap;
};

/** xrc handler for split button */
class SplitButtonXmlHandler : public wxXmlResourceHandler
{
    DECLARE_DYNAMIC_CLASS(SplitButtonXmlHandler)

public:
    SplitButtonXmlHandler();
    virtual wxObject* DoCreateResource();
    virtual bool CanHandle(wxXmlNode* node);
};

#endif /*__SPLIT_BUTTON_H__*/