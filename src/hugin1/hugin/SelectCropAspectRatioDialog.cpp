// -*- c-basic-offset: 4 -*-

/** @file SelectCropAspectRatioDialog.cpp
 *
 *	@brief implementation of dialog for selecting and editing aspect ratios of crop
 *
 *  @author T. Modes
 *
 */

/*  This is free software; you can redistribute it and/or
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

#include "SelectCropAspectRatioDialog.h"
#include "panoinc_WX.h"
#include "hugin/huginApp.h"
#include <wx/textfile.h>

static const wxString separator = "@@@";

SelectAspectRatioDialog::SelectAspectRatioDialog(wxWindow* parent)
{
    wxXmlResource::Get()->LoadDialog(this, parent, "choice_aspect_dlg");
#ifdef __WXMSW__
    wxIconBundle myIcons(huginApp::Get()->GetXRCPath() + "data/hugin.ico", wxBITMAP_TYPE_ICO);
    SetIcons(myIcons);
#else
    wxIcon myIcon(huginApp::Get()->GetXRCPath() + "data/hugin.png", wxBITMAP_TYPE_PNG);
    SetIcon(myIcon);
#endif
    m_aspectList = XRCCTRL(*this, "aspect_listbox", wxListBox);
    FillListBox();
    XRCCTRL(*this, "wxID_OK", wxButton)->Bind(wxEVT_BUTTON, &SelectAspectRatioDialog::OnOk, this);
    XRCCTRL(*this, "add_aspect_button", wxButton)->Bind(wxEVT_BUTTON, &SelectAspectRatioDialog::OnAddAspectRatio, this);
    XRCCTRL(*this, "remove_aspect_button", wxButton)->Bind(wxEVT_BUTTON, &SelectAspectRatioDialog::OnRemoveAspectRatio, this);
    m_aspectList->Bind(wxEVT_LISTBOX, &SelectAspectRatioDialog::OnListBoxSelect, this);
    ListBoxSelectionChanged();
    m_aspectList->SendSelectionChangedEvent(wxEVT_LISTBOX);
    RestoreFramePosition(this, "SelectCropAspectRatioDialog");
}

SelectAspectRatioDialog::~SelectAspectRatioDialog()
{
    // store frame position
    StoreFramePosition(this, "SelectCropAspectRatioDialog");
    // store list of aspect ratios
    SaveAspectRatios();
}

double SelectAspectRatioDialog::GetSelectedAspectRatio() const
{
    const int selection = m_aspectList->GetSelection();
    if (selection != wxNOT_FOUND && selection < m_aspectRatios.size())
    {
        return m_aspectRatios[selection];
    }
    return 1.0;
}

void SelectAspectRatioDialog::OnOk(wxCommandEvent& e)
{
    const int selection = m_aspectList->GetSelection();
    if (selection != wxNOT_FOUND)
    {
        EndModal(wxID_OK);
    };
}

/** check the input of the add aspect ratio dialog */
bool CheckInputs(wxWindow* parent, wxTextCtrl* labelCtrl, wxTextCtrl* aspectRatioCtrl, double& value)
{
    // the label should contain at least one character, trim whitespace
    const wxString label = labelCtrl->GetValue().Trim(true).Trim(false);
    if (label.IsEmpty() || label.Find(separator) != wxNOT_FOUND)
    {
        // empty label or label contains our separator
        wxMessageBox(_("You must provide a valid label."), _("Warning"), wxOK | wxICON_ERROR, parent);
        return false;
    };
    // check if it is a simple number
    wxString valueString = aspectRatioCtrl->GetValue();
    int pos = valueString.Find(":");
    if (pos != wxNOT_FOUND)
    {
        // we have a : in the string, assume it is format value 1:value 2
        if (pos > 0)
        {
            double val1, val2 = 0;
            if (valueString.Mid(0, pos).ToDouble(&val1) && valueString.Mid(pos + 1).ToDouble(&val2))
            {
                if (val2 > 0.0001)
                {
                    value = val1 / val2;
                    if (value > 1e-4 && value < 10000)
                    {
                        return true;
                    }
                    else
                    {
                        wxMessageBox(wxString::Format(_("The aspect ratio \"%s\" is not in the valid range."), aspectRatioCtrl->GetValue().c_str()), _("Warning"), wxOK | wxICON_ERROR, parent);
                        return false;
                    };
                };
            };
        };
    }
    else
    {
        // try to read the string as a simple number
        if (valueString.ToDouble(&value))
        {
            if (value > 1e-4 && value < 10000)
            {
                return true;
            }
            else
            {
                wxMessageBox(wxString::Format(_("The aspect ratio \"%s\" is not in the valid range."), aspectRatioCtrl->GetValue().c_str()), _("Warning"), wxOK | wxICON_ERROR, parent);
                return false;
            };
        };
    };
    // not a valid input, issue warning
    wxMessageBox(wxString::Format(_("The input \"%s\" is not a valid number."), aspectRatioCtrl->GetValue().c_str()), _("Warning"), wxOK | wxICON_ERROR, parent);
    return false;
}

void SelectAspectRatioDialog::OnAddAspectRatio(wxCommandEvent& e)
{
    wxDialog dlg;
    wxXmlResource::Get()->LoadDialog(&dlg, this, "add_aspect_dlg");
    wxTextCtrl* labelCtrl = XRCCTRL(dlg, "aspect_label_textctrl", wxTextCtrl);
    wxTextCtrl* aspectRatioCtrl = XRCCTRL(dlg, "aspect_ratio_textctrl", wxTextCtrl);
    aspectRatioCtrl->SetValue("2:1");
    // check if input is valid when pressing ok
    dlg.Bind(wxEVT_COMMAND_BUTTON_CLICKED, [labelCtrl, aspectRatioCtrl, &dlg](wxCommandEvent &e)
        {
            double val;
            if (CheckInputs(&dlg, labelCtrl, aspectRatioCtrl, val))
            {
                dlg.EndModal(wxID_OK);
            };
        }, XRCID("wxID_OK"));
    if (dlg.ShowModal() == wxID_OK)
    {
        double value;
        if (CheckInputs(&dlg, labelCtrl, aspectRatioCtrl, value))
        {
            m_aspectRatioStrings.push_back(labelCtrl->GetValue().Trim(true).Trim(false));
            m_aspectList->AppendString(m_aspectRatioStrings.back());
            m_aspectRatios.push_back(value);
            ListBoxSelectionChanged();
        };
    };
}

void SelectAspectRatioDialog::OnRemoveAspectRatio(wxCommandEvent& e)
{
    const int selection = m_aspectList->GetSelection();
    // selection should be valid and list should contain at least one item
    if (selection != wxNOT_FOUND && selection < m_aspectRatioStrings.size() && m_aspectRatioStrings.size() > 1)
    {
        m_aspectRatioStrings.RemoveAt(selection);
        m_aspectRatios.erase(m_aspectRatios.begin() + selection);
        m_aspectList->Delete(selection);
        m_aspectList->SetSelection(-1);
        ListBoxSelectionChanged();
    }
    else
    {
        wxBell();
    };
}

void SelectAspectRatioDialog::OnListBoxSelect(wxCommandEvent& e)
{
    ListBoxSelectionChanged();
};

void SelectAspectRatioDialog::ListBoxSelectionChanged()
{
    const int selection = m_aspectList->GetSelection();
    // selection should be valid and list should contain at least one item
    XRCCTRL(*this, "wxID_OK", wxButton)->Enable(selection != wxNOT_FOUND);
    XRCCTRL(*this, "remove_aspect_button", wxButton)->Enable(selection != wxNOT_FOUND && m_aspectRatioStrings.size() > 1);
}

void SelectAspectRatioDialog::FillListBox()
{
    LoadAspectRatios();
    if (m_aspectRatioStrings.IsEmpty())
    {
        // could not read from file, populate with default values
        const auto addAspect = [this](const double v1, const double v2)
        {
            this->m_aspectRatioStrings.Add(wxString::Format("%.0f:%.0f", v1, v2));
            this->m_aspectRatios.push_back(v1 / v2);
        };
        addAspect(2, 1);
        addAspect(3, 1);
        addAspect(4, 1);
        addAspect(3, 2);
        addAspect(4, 3);
        addAspect(16, 9);
        addAspect(1, 1);
    };
    // add to list
    m_aspectList->Append(m_aspectRatioStrings);
}

wxFileName SelectAspectRatioDialog::GetAspectRatioFilename() const
{
    return wxFileName(hugin_utils::GetUserAppDataDir(), "crop_aspectratio.txt");
}

void SelectAspectRatioDialog::LoadAspectRatios()
{
    wxTextFile textFile(GetAspectRatioFilename().GetFullPath());
    if (textFile.Exists())
    {
        // text file exists, open it
        textFile.Open();
        // iterate all lines
        for (wxString line = textFile.GetFirstLine(); !textFile.Eof(); line = textFile.GetNextLine())
        {
            // remove whitespaces
            line = line.Trim(true).Trim(false);
            const size_t pos = line.Find(separator);
            if (pos != wxNOT_FOUND && pos > 0 && pos + separator.length() < line.length())
            {
                const wxString aspectRatio = line.Mid(pos + separator.length());
                if (aspectRatio.IsEmpty())
                {
                    continue;
                };
                double ratio;
                // read aspect ratio, use C locale
                if (aspectRatio.ToCDouble(&ratio))
                {
                    m_aspectRatioStrings.push_back(line.Mid(0, pos));
                    m_aspectRatios.push_back(ratio);
                };
            };
        };
    };
}

void SelectAspectRatioDialog::SaveAspectRatios()
{
    wxTextFile textFile(GetAspectRatioFilename().GetFullPath());
    if (textFile.Exists())
    {
        // text file exists, open it
        textFile.Open();
    }
    else
    {
        // file does not exist, create a new one
        textFile.Create();
    }
    // clear all existing items in file
    textFile.Clear();
    // write all items
    for (size_t i = 0; i < m_aspectRatioStrings.size(); ++i)
    {
        // use C locale for writing to be on the safe side for number formatting
        textFile.AddLine(m_aspectRatioStrings[i] + separator + wxString::FromCDouble(m_aspectRatios[i]));
    };
    // write to disc and close
    textFile.Write();
    textFile.Close();
}
