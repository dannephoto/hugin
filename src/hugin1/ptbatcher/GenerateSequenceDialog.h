// -*- c-basic-offset: 4 -*-
/** @file GenerateSequenceDialog.h
 *
 * @brief Definition of GenerateSequenceDialog class
 *
 * @author T. Modes
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

#ifndef _GENERATESEQUENCEDIALOG_H
#define _GENERATESEQUENCEDIALOG_H

#include <vector>
#include "panoinc_WX.h"
#include "panoinc.h"
#include "BatchFrame.h"

extern "C"
{
#include "tiff.h"
#include "tiffio.h"
}

/** Dialog for generate panoramas from a sequence of images 
 *
 */
class GenerateSequenceDialog : public wxDialog
{
public:
    /** Constructor, read from xrc ressource; restore last uses settings, size and position */
    GenerateSequenceDialog(BatchFrame* batchframe, wxString xrcPrefix, wxString ptoFilename);
    /** destructor, saves size and position */
    ~GenerateSequenceDialog();
    /** return true if given template is a valid pto file, if not the dialog should not be used */
    bool IsValidPanorama() const;

protected:
    /** show select directory dialog */
    void OnSelectBasePath(wxCommandEvent& e);
    /** select which sub-directory should be used */
    void OnSelectSubDir(wxCommandEvent& e);
    /** image in list ctrl selected */
    void OnImageListSelected(wxListEvent& e);
    /** update image filename template on selected image */
    void OnUpdateImageTemplate(wxCommandEvent& e);
    /** update images filename template for all images */
    void OnUpdateAllImagesTemplate(wxCommandEvent& e);
    /** update all spin controls with current numbers */
    void OnUpdateCounters(wxSpinEvent& e);
    /** generate preview */
    void OnGeneratePreview(wxCommandEvent& e);
    /** generate stitching sequence button */
    void OnGenerateStitchingPanorama(wxCommandEvent& e);
    /** generate assistant sequence button */
    void OnGenerateAssistantPanorama(wxCommandEvent& e);

private:
    /** read pto template from file */
    void ReadPTOFile();
    /** fill the images list with current values */
    void FillImagesList();
    /** update all counters, enable/disable end value and calculate end values */
    void UpdateCounters();
    /** generate a list of all panorama and images in panoramas */
    void GenerateFileList(wxArrayString& panoSubDirs, std::vector<wxArrayString>& fileList);
    /** generates all panoramas and add them to the batch queue with given queue target */
    void DoGeneratePanorama(const Project::Target target);

    BatchFrame* m_batchframe;
    wxString m_filename;
    HuginBase::Panorama m_pano;
    bool m_validPTO{ false };
    wxTextCtrl* m_basepath;
    wxChoice* m_choiceSubDir;
    wxTextCtrl* m_subDirTextCtrl;
    long m_subDirCount;
    wxSpinCtrl* m_spinCounterP_offset, * m_spinCounterP_step, * m_spinCounterP_end;
    wxSpinCtrl* m_spinCounterI_offset, * m_spinCounterI_step, * m_spinCounterI_end;
    wxSpinCtrl* m_spinCounterX_offset, * m_spinCounterX_step, * m_spinCounterX_end;
    wxArrayString m_orignalFilenames;
    wxArrayString m_mappedFilenames;
    wxListCtrl* m_imagesListCtrl;
    wxStaticText* m_originalImage;
    wxTextCtrl* m_imageTemplate;
    wxButton* m_changeImageTemplate;
    wxButton* m_changeAllImagesTemplate;

    DECLARE_EVENT_TABLE()
};


#endif //_GENERATESEQUENCEDIALOG_H
