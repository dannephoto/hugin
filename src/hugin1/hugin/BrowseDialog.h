// -*- c-basic-offset: 4 -*-
/**  @file BrowseDialog.h
 *
 *  @brief Definition of dialog to browse directory with pto files
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

#ifndef _BROWSEPTOFILESDIALOG_H 
#define _BROWSEPTOFILESDIALOG_H

#include "wx/dialog.h"
#include "wx/dirctrl.h"
#include "wx/listctrl.h"
#include "wx/imaglist.h"
#include "wx/statbmp.h"
#include "wx/stattext.h"
#include "wx/splitter.h"
#include "wx/arrstr.h"
#include "wx/thread.h"
#include "wx/datetime.h"
#include "wx/filename.h"
#include "panodata/PanoramaOptions.h"

/** background thread for generating the preview images, */
// forward declaration for easier access on members of dialog
class ThumbnailThread;

/** class to store some information about a pto file on disc */
class PanoInfo
{
public:
    wxFileName ptoFilename;
    // number of images in pto file
    size_t nrImages = 0;
    // number of active images
    size_t nrActiveImages = 0;
    HuginBase::PanoramaOptions options;
    wxString projectionName;
    wxDateTime start, end;
    wxTimeSpan duration;
    wxString camera;
    wxString lens;
    double focalLength35 = 0.0;
    wxString focalLengthString;
    int imageIndex = -1;
    double GPSLongitude = DBL_MAX;
    double GPSLatitude = DBL_MAX;
    bool HasGPS()
    {
        return GPSLongitude != DBL_MAX && GPSLatitude != DBL_MAX;
    }
};

/** Dialog for browsing pto files */
class BrowsePTOFilesDialog : public wxDialog
{
public:
    /** Constructor, read from xrc ressource; restore last uses settings and position */
    BrowsePTOFilesDialog(wxWindow *parent, const wxString startDirectory);
    /** destructor, saves position */
    ~BrowsePTOFilesDialog();
    /** return full path of selected project */
    wxString GetSelectedProject();
    /** return last selected path */
    wxString GetSelectedPath();

protected:
    /** Saves current expression when closing dialog with Ok */
    void OnOk(wxCommandEvent& e);
    /** directory changed, load files from new directory */
    void OnDirectoryChanged(wxTreeEvent& e);
    /** new file selected, generate preview for new file */
    void OnFileChanged(wxListEvent& e);
    /** double click does open file */
    void OnDblClickListCtrl(wxMouseEvent& e);
    /** click on header to sort by column */
    void OnListColClick(wxListEvent& e);
    /** for notifing from ThumbnailThread about new generated thumbnail 
        generated thumbnail is transfered in event.GetEventObject() */
    void OnThumbnailUpdate(wxCommandEvent& e);
    /** Change display of wxListCtrl */
    void OnListTypeChanged(wxCommandEvent& e);
    /** show current pano on Openstreetmap */
    void OnShowOnMap(wxCommandEvent& e);

private:
    /** translate the index to the index of m_ptoInfo */
    long TranslateIndex(const long index);
    /** sort the items according to selected column */
    void SortItems();
    /** end background thumbnail creating thread */
    void EndThumbnailThread();
    /** read the given pto file and add all information to wxListCtrl */
    PanoInfo ParsePTOFile(const wxFileName file);
    /** add a new item to wxListCtrl and populate all columns */
    void FillPanoInfo(const PanoInfo& info, long index);
    /** update all item texts */
    void UpdateItemTexts(long newStyle);
#if !wxCHECK_VERSION(3,1,6)
    /** update the image indexes */
    void UpdateImagesIndex();
#endif
    /** generate preview for pto file with index */
    void GeneratePreview(int index);
    // access to some often needed GUI elements
    wxGenericDirCtrl* m_dirCtrl = nullptr;
    wxListCtrl* m_listCtrl = nullptr;
    wxChoice* m_listType = nullptr;
    wxButton* m_showMap = nullptr;
    // store current sorting column
    long m_sortCol = -1;
    bool m_sortAscending = true;
    wxStaticBitmap* m_previewCtrl = nullptr;
    wxStaticText* m_labelControl = nullptr;
    wxSplitterWindow* m_splitter1;
    wxSplitterWindow* m_splitter2;
    /** info about the pto file */
    std::vector<PanoInfo> m_ptoInfo;
    /** image list with all thumbnails */
    wxImageList m_thumbnails;
    /** background thumbnail creater thread */
    ThumbnailThread* m_thumbnailThread = nullptr;
    /** critical section to synchronize with ThumbnailThread */
    wxCriticalSection m_ThreadCS;
    friend class ThumbnailThread;
    DECLARE_EVENT_TABLE()
};

#endif //_BROWSEPTOFILESDIALOG_H
