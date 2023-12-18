// -*- c-basic-offset: 4 -*-
/**  @file RawImport.h
 *
 *  @brief Definition of dialog and functions to import RAW images to project file
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

#ifndef _RAW_IMPORT_H
#define _RAW_IMPORT_H

#include "panoinc_WX.h"
#include "panoinc.h"
#include "base_wx/Command.h"

/** Dialog for raw import */
class RawImportDialog : public wxDialog
{
public:
    /** Constructor, read from xrc ressource; restore last uses settings and position */
    RawImportDialog(wxWindow *parent, HuginBase::Panorama* pano, std::vector<std::string>& rawFiles);
    /** destructor, saves position */
    ~RawImportDialog();
    /** return PanoCommand for adding converted raw files to Panorama */
    PanoCommand::PanoCommand* GetPanoCommand();
    /** return true, if all raw files are from the same camera */
    bool CheckRawFiles();

protected:
    /** called when dialog is finished and does the conversion */
    void OnOk(wxCommandEvent & e);
    void OnSelectRTProcessingProfile(wxCommandEvent& e);
    void OnRawConverterSelected(wxCommandEvent & e);

private:
    /** fill list with image names */
    void FillImageChoice();
    HuginBase::Panorama* m_pano;
    wxArrayString m_rawImages;
    wxArrayString m_images;
    PanoCommand::PanoCommand* m_cmd=NULL;

    DECLARE_EVENT_TABLE()
};

#endif //_RAW_IMPORT_H
