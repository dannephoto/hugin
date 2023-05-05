// -*- c-basic-offset: 4 -*-
/**  @file EditOutputIniDialog.h
 *
 *  @brief Definition of dialog for editing user-defined output ini file
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

#ifndef _EDITOUTPUTINIDIALOG_H
#define _EDITOUTPUTINIDIALOG_H

#include "panoinc_WX.h"
#include <wx/propgrid/propgrid.h>
#include <wx/propgrid/manager.h>

/** Dialog for editing user defined output settings (ini file) */
class EditOutputIniDialog : public wxDialog
{
public:
    /** Constructor, constructs dialog; restore last uses settings, size and position */
    EditOutputIniDialog(wxWindow* parent);
    /** destructor, saves size and position */
    ~EditOutputIniDialog();

protected:
    /** save ini and close dialog */
    void OnOk(wxCommandEvent& e);
    /** right click handler, show popup menu */
    void OnContextMenu(wxContextMenuEvent& e);
    /** add a new section */
    void OnAddSection(wxCommandEvent& e);
    /** renames a new section */
    void OnRenameSection(wxCommandEvent& e);
    /** adds a new condition to list */
    void OnAddCondition(wxCommandEvent& e);

private:
    /** read the ini file and populate the control */
    void ReadIni();
    /** write the ini to disc */
    void WriteIni();
    /** return the filename of the default ini file */
    wxFileName GetIniFileName();
    /** return wxArrayString with all sections */
    wxArrayString GetSections() const;
    /** return wxArrayString with all sub-entries of given wxPGProperty */
    wxArrayString GetChildren(wxPGProperty* prop) const;
    /** function to build context menu with all missing entries */
    void BuildAddContextMenu(wxMenu& menu, wxPGProperty* category, const bool addSeparator);
    /** add key with value to wxPropertyGrid, generate if necessary all sub properties */
    void AddKey(wxPGProperty* section, const wxString& key, const wxString& value);

    wxPropertyGridManager* m_grid;
    wxPGProperty* m_currentSection;
};


#endif //_EDITOUTPUTINIDIALOG_H
