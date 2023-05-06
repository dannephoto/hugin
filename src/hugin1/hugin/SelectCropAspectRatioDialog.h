// -*- c-basic-offset: 4 -*-

/**  @file SelectCropAspectRatioDialog.h
 *
 *  @brief Definition of dialog for selecting and editing aspect ratios of crop
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

#ifndef _SELECTCROPASPECTRATIODIALOG_H
#define _SELECTCROPASPECTRATIODIALOG_H

#include <wx/dialog.h>
#include <wx/listbox.h>
#include <wx/filename.h>
#include <vector>

class SelectAspectRatioDialog :public wxDialog
{
public:
    /** constructor, build dialog and load settings from file */
    SelectAspectRatioDialog(wxWindow* parent);
    /** destructor, save position and list of aspect ratios */
    ~SelectAspectRatioDialog();
    /** returns the selected aspect ratio */
    double GetSelectedAspectRatio() const;
protected:
    /** handler for Ok, check for valid inputs */
    void OnOk(wxCommandEvent& e);
    /** handler for adding new aspect ratio setting */
    void OnAddAspectRatio(wxCommandEvent& e);
    /** handler for removing selected aspect ratio */
    void OnRemoveAspectRatio(wxCommandEvent& e);
    /** event handler for activation/deactivation of buttons if necessary */
    void OnListBoxSelect(wxCommandEvent& e);
private:
    /** implementation of activation/deactivation of buttons if necessary */
    void ListBoxSelectionChanged();
    /** read the settings from the file and add them to the list box
        if the file does not exist, populate list with default settings */
    void FillListBox();
    /** returns the file name for the list of aspect ratios */
    wxFileName GetAspectRatioFilename() const;
    /** load aspect ratios from file */
    void LoadAspectRatios();
    /** save the aspect ratios to the file */
    void SaveAspectRatios();
    /** array with names of all aspect ratios */
    wxArrayString m_aspectRatioStrings;
    /** vector with values of all aspect ratios, ratio=width/height */
    std::vector<double> m_aspectRatios;
    /** pointer to wxListBox of aspect ratios */
    wxListBox* m_aspectList;
};

#endif //_SELECTCROPASPECTRATIODIALOG_H
