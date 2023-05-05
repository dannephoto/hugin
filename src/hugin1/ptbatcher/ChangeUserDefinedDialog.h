// -*- c-basic-offset: 4 -*-

/** @file FindPanoDialog.h
 *
 *  @brief Definition of dialog to change user defined sequence
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

#ifndef _CHANGEUSERDEFINEDDIALOG_H
#define _CHANGEUSERDEFINEDDIALOG_H

#include "panoinc_WX.h"
#include "panoinc.h"

/** Dialog for changing the user defined sequence
 */
class ChangeUserDefinedSequenceDialog : public wxDialog
{
public:
    /** Constructor, read from xrc ressource; restore last uses size and position */
    ChangeUserDefinedSequenceDialog(wxWindow* parent, wxString xrcPrefix, wxString userDefinedSequence, bool assistantUserDefined);
    /** destructor, saves size and position */
    ~ChangeUserDefinedSequenceDialog();
    /** returns the newly selected sequence */
    wxString GetNewSequence();

protected:
    void UpdateStatus(wxCommandEvent& e);
    void OnChangeUserDefinedChoice(wxCommandEvent& e);
    void OnChooseFilename(wxCommandEvent& e);
    void OnOk(wxCommandEvent& e);

private:
    void FillAssistantUserDefined();
    void FillStitcherUserDefined();
    void FillUserDefinedChoice(const wxArrayString& files);

    bool m_isAssistantUserDefined;
    wxArrayString m_userFileNames;
    wxArrayString m_userHelpTexts;
    // all controls for easier access
    wxRadioButton* m_radio_default;
    wxRadioButton* m_radio_user_sequence;
    wxChoice* m_choice_user_define;
    wxStaticText* m_label_user_define;
    wxStaticText* m_label_user_define_filename;
    wxRadioButton* m_radio_external;
    wxTextCtrl* m_text_external_file;
    wxButton* m_button_external_file;

    DECLARE_EVENT_TABLE()
};

#endif //_CHANGEUSERDEFINEDDIALOG_H
