// -*- c-basic-offset: 4 -*-
/** @file GenerateSequenceDialog.cpp
 *
 * @brief implementation of GenerateSequenceDialog class
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

#include "GenerateSequenceDialog.h"
#include "base_wx/wxPlatform.h"
#if wxCHECK_VERSION(3,1,0)
#include <wx/busyinfo.h>
#endif
#include "panoinc.h"

/** return a list of all sub-directories */
wxArrayString GetAllSubDirectories(const wxString baseDir)
{
    wxArrayString subDirs;
    wxDir dir(baseDir);
    if (!dir.IsOpened())
    {
        // could not open given directories
        return subDirs;
    }
    wxString filename;
    bool cont = dir.GetFirst(&filename, wxEmptyString, wxDIR_DIRS);
    while (cont)
    {
        subDirs.Add(filename);
        cont = dir.GetNext(&filename);
    };
    return subDirs;
}

/** return given number as string with given number of digits */
wxString GetNumberString(size_t x, size_t width)
{
    const wxString formatString = wxString::Format("%%0%dd", width);
    return wxString::Format(formatString, x);
}

BEGIN_EVENT_TABLE(GenerateSequenceDialog, wxDialog)
    EVT_BUTTON(XRCID("sequence_select_base_path"), GenerateSequenceDialog::OnSelectBasePath)
    EVT_CHOICE(XRCID("sequence_choice_subdirectory"), GenerateSequenceDialog::OnSelectSubDir)
    EVT_LIST_ITEM_SELECTED(XRCID("sequence_images_list"), GenerateSequenceDialog::OnImageListSelected)
    EVT_LIST_ITEM_DESELECTED(XRCID("sequence_images_list"), GenerateSequenceDialog::OnImageListSelected)
    EVT_BUTTON(XRCID("sequence_change_image"), GenerateSequenceDialog::OnUpdateImageTemplate)
    EVT_BUTTON(XRCID("sequence_change_all_images"), GenerateSequenceDialog::OnUpdateAllImagesTemplate)
    EVT_SPINCTRL(XRCID("sequence_p_offset"), GenerateSequenceDialog::OnUpdateCounters)
    EVT_SPINCTRL(XRCID("sequence_p_step"), GenerateSequenceDialog::OnUpdateCounters)
    EVT_SPINCTRL(XRCID("sequence_p_end"), GenerateSequenceDialog::OnUpdateCounters)
    EVT_SPINCTRL(XRCID("sequence_i_offset"), GenerateSequenceDialog::OnUpdateCounters)
    EVT_SPINCTRL(XRCID("sequence_i_step"), GenerateSequenceDialog::OnUpdateCounters)
    EVT_SPINCTRL(XRCID("sequence_i_end"), GenerateSequenceDialog::OnUpdateCounters)
    EVT_SPINCTRL(XRCID("sequence_x_offset"), GenerateSequenceDialog::OnUpdateCounters)
    EVT_SPINCTRL(XRCID("sequence_x_step"), GenerateSequenceDialog::OnUpdateCounters)
    EVT_SPINCTRL(XRCID("sequence_x_end"), GenerateSequenceDialog::OnUpdateCounters)
    EVT_BUTTON(XRCID("sequence_generate_preview"), GenerateSequenceDialog::OnGeneratePreview)
    EVT_BUTTON(XRCID("sequence_generate_stitching"), GenerateSequenceDialog::OnGenerateStitchingPanorama)
    EVT_BUTTON(XRCID("sequence_generate_assistant"), GenerateSequenceDialog::OnGenerateAssistantPanorama)
END_EVENT_TABLE()

GenerateSequenceDialog::GenerateSequenceDialog(BatchFrame* batchframe, wxString xrcPrefix, wxString ptoFilename)
{
    // load our children. some children might need special
    // initialization. this will be done later.
    wxXmlResource::Get()->LoadDialog(this,batchframe, "generate_sequence_dialog");

#ifdef __WXMSW__
    wxIconBundle myIcons(xrcPrefix+ "data/ptbatcher.ico",wxBITMAP_TYPE_ICO);
    SetIcons(myIcons);
#else
    wxIcon myIcon(xrcPrefix + "data/ptbatcher.png",wxBITMAP_TYPE_PNG);
    SetIcon(myIcon);
#endif
    m_batchframe=batchframe;
    m_filename = ptoFilename;
    ReadPTOFile();
    if (IsValidPanorama())
    {
        this->SetLabel(wxString::Format(_("Generating sequence from %s"), m_filename.c_str()));
        m_basepath = XRCCTRL(*this, "sequence_base_path", wxTextCtrl);
        m_basepath->SetValue(wxPathOnly(m_filename));

        m_choiceSubDir = XRCCTRL(*this, "sequence_choice_subdirectory", wxChoice);
        m_subDirTextCtrl = XRCCTRL(*this, "sequence_directory_name", wxTextCtrl);

        m_imagesListCtrl = XRCCTRL(*this, "sequence_images_list", wxListCtrl);
        m_imagesListCtrl->InsertColumn(0, _("Template image name"));
        m_imagesListCtrl->InsertColumn(1, _("Sequence image name"));
        FillImagesList();

        m_originalImage = XRCCTRL(*this, "sequence_orignal_image_text", wxStaticText);
        m_imageTemplate = XRCCTRL(*this, "sequence_image_text", wxTextCtrl);
        m_changeImageTemplate = XRCCTRL(*this, "sequence_change_image", wxButton);
        m_changeAllImagesTemplate = XRCCTRL(*this, "sequence_change_all_images", wxButton);

        m_spinCounterP_offset = XRCCTRL(*this, "sequence_p_offset", wxSpinCtrl);
        m_spinCounterP_step = XRCCTRL(*this, "sequence_p_step", wxSpinCtrl);
        m_spinCounterP_end = XRCCTRL(*this, "sequence_p_end", wxSpinCtrl);
        m_spinCounterI_offset = XRCCTRL(*this, "sequence_i_offset", wxSpinCtrl);
        m_spinCounterI_step = XRCCTRL(*this, "sequence_i_step", wxSpinCtrl);
        m_spinCounterI_end = XRCCTRL(*this, "sequence_i_end", wxSpinCtrl);
        m_spinCounterX_offset = XRCCTRL(*this, "sequence_x_offset", wxSpinCtrl);
        m_spinCounterX_step = XRCCTRL(*this, "sequence_x_step", wxSpinCtrl);
        m_spinCounterX_end = XRCCTRL(*this, "sequence_x_end", wxSpinCtrl);

        wxCommandEvent dummy;
        OnSelectSubDir(dummy);
        // UpdateCounters();

        //set parameters
        wxConfigBase* config = wxConfigBase::Get();
        // restore position and size
        int dx, dy;
        wxDisplaySize(&dx, &dy);
        bool maximized = config->Read("/GenerateSequenceDialog/maximized", 0l) != 0;
        if (maximized)
        {
            this->Maximize();
        }
        else
        {
            //size
            int w = config->Read("/GenerateSequenceDialog/width", -1l);
            int h = config->Read("/GenerateSequenceDialog/height", -1l);
            if (w > 0 && w <= dx)
            {
                this->SetClientSize(w, h);
            }
            else
            {
                this->Fit();
            }
            //position
            int x = config->Read("/GenerateSequenceDialog/positionX", -1l);
            int y = config->Read("/GenerateSequenceDialog/positionY", -1l);
            if (y >= 0 && x >= 0 && x < dx && y < dy)
            {
                this->Move(x, y);
            }
            else
            {
                this->Move(0, 44);
            }
        }

        m_imagesListCtrl->SetColumnWidth(0, config->Read("/GenerateSequenceDialog/ImageListColumn0Width", 200l));
        m_imagesListCtrl->SetColumnWidth(1, config->Read("/GenerateSequenceDialog/ImageListColumn1Width", 200l));
        XRCCTRL(*this, "sequence_naming", wxChoice)->SetSelection(config->Read("/GenerateSequenceDialog/NamingConvention", 0l));        
    };
    XRCCTRL(*this, "sequence_counter_help", wxTextCtrl)->SetBackgroundColour(this->GetBackgroundColour());
};

GenerateSequenceDialog::~GenerateSequenceDialog()
{
    wxConfigBase* config=wxConfigBase::Get();
    if(!this->IsMaximized())
    {
        wxSize sz = this->GetClientSize();
        config->Write("/GenerateSequenceDialog/width", sz.GetWidth());
        config->Write("/GenerateSequenceDialog/height", sz.GetHeight());
        wxPoint ps = this->GetPosition();
        config->Write("/GenerateSequenceDialog/positionX", ps.x);
        config->Write("/GenerateSequenceDialog/positionY", ps.y);
        config->Write("/GenerateSequenceDialog/maximized", 0);
    }
    else
    {
        config->Write(wxT("/GenerateSequenceDialog/maximized"), 1l);
    };
    config->Write("/GenerateSequenceDialog/ImageListColumn0Width", m_imagesListCtrl->GetColumnWidth(0));
    config->Write("/GenerateSequenceDialog/ImageListColumn1Width", m_imagesListCtrl->GetColumnWidth(1));
    config->Write("/GenerateSequenceDialog/NamingConvention", XRCCTRL(*this, "sequence_naming", wxChoice)->GetSelection());
}

bool GenerateSequenceDialog::IsValidPanorama() const
{
    return m_validPTO && m_pano.getNrOfImages() > 0;
}

void GenerateSequenceDialog::ReadPTOFile()
{
    std::ifstream prjfile((const char*)m_filename.mb_str(HUGIN_CONV_FILENAME));
    if (prjfile.good())
    {
        HuginBase::PanoramaMemento newPano;
        int ptoVersion = 0;
        if (newPano.loadPTScript(prjfile, ptoVersion, ""))
        {
            m_pano.setMemento(newPano);
            m_validPTO = true;
        };
        for (size_t i = 0; i < m_pano.getNrOfImages(); ++i)
        {
            const wxString imageFilename(wxString(m_pano.getImage(i).getFilename().c_str(), HUGIN_CONV_FILENAME));
            m_orignalFilenames.Add(imageFilename);
            m_mappedFilenames.Add(imageFilename);
        };
    };
}

void GenerateSequenceDialog::FillImagesList()
{
    long selItem = -1;
    if (m_imagesListCtrl->GetSelectedItemCount() > 0)
    {
        // remember selected item
        selItem = m_imagesListCtrl->GetNextItem(selItem, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
    };
    m_imagesListCtrl->DeleteAllItems();
    DEBUG_ASSERT(m_orignalFilenames.size() == m_mappedFilenames.size());
    for (size_t i = 0; i < m_orignalFilenames.size(); ++i)
    {
        const size_t index = m_imagesListCtrl->GetItemCount();
        m_imagesListCtrl->InsertItem(index, m_orignalFilenames[i]);
        m_imagesListCtrl->SetItem(index, 1, m_mappedFilenames[i]);
    };
    // restore selected item
    if (selItem != -1)
    {
        m_imagesListCtrl->SetItemState(selItem, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
    };
}

void GenerateSequenceDialog::OnSelectBasePath(wxCommandEvent& e)
{
    wxDirDialog dlg(this, _("Specify a directory to search for projects in"),
        m_basepath->GetValue(), wxDD_DEFAULT_STYLE | wxDD_DIR_MUST_EXIST);
    if (dlg.ShowModal() == wxID_OK)
    {
        m_basepath->SetValue(dlg.GetPath());
        OnSelectSubDir(e);
    };
}

void GenerateSequenceDialog::OnSelectSubDir(wxCommandEvent& e)
{
    // show text ctrl only when "use name template" use selected
    m_subDirTextCtrl->Enable(m_choiceSubDir->GetSelection() == 2);
    if (m_choiceSubDir->GetSelection() != 2)
    {
        // clear input wxTextCtrl if not used
        m_subDirTextCtrl->Clear();
    };
    if (m_choiceSubDir->GetSelection() == 1)
    {
        // get list of all sub-directories
        wxArrayString subDirs = GetAllSubDirectories(m_basepath->GetValue());
        m_subDirCount = subDirs.size();
    }
    else
    {
        m_subDirCount = 0;
    };
    UpdateCounters();
}

void GenerateSequenceDialog::OnImageListSelected(wxListEvent& e)
{
    if (m_imagesListCtrl->GetSelectedItemCount() > 0)
    {
        // enable all controls
        // if one image is selected in list control
        m_imageTemplate->Enable(true);
        m_changeImageTemplate->Enable(true);
        m_changeAllImagesTemplate->Enable(true);
        // set text
        long item = -1;
        item = m_imagesListCtrl->GetNextItem(item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
        m_originalImage->SetLabel(m_orignalFilenames[item]);
        m_imageTemplate->SetValue(m_mappedFilenames[item]);
    }
    else
    {
        // no item selected, disable all related ctrl
        m_imageTemplate->Enable(false);
        m_changeImageTemplate->Enable(false);
        m_changeAllImagesTemplate->Enable(false);
    }
}

void GenerateSequenceDialog::OnUpdateImageTemplate(wxCommandEvent& e)
{
    if (m_imagesListCtrl->GetSelectedItemCount() > 0)
    {
        // find selected item
        long item = -1;
        item = m_imagesListCtrl->GetNextItem(item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
        // now update value
        m_mappedFilenames[item] = m_imageTemplate->GetValue();
        FillImagesList();
    }
    else
    {
        wxBell();
    };
}

void GenerateSequenceDialog::OnUpdateAllImagesTemplate(wxCommandEvent& e)
{
    for (size_t i = 0; i < m_mappedFilenames.size(); ++i)
    {
        // update values
        m_mappedFilenames[i] = m_imageTemplate->GetValue();
    };
    FillImagesList();
}

void GenerateSequenceDialog::OnUpdateCounters(wxSpinEvent& e)
{
    UpdateCounters();
}

void GenerateSequenceDialog::UpdateCounters()
{
    // update %p counter
    size_t pano_count;
    if (m_choiceSubDir->GetSelection() == 1)
    {
        // iterate all sub-directories selected
        m_spinCounterP_end->Enable(false);
        pano_count = m_subDirCount;
        if (pano_count > 0)
        {
            m_spinCounterP_end->SetValue(m_spinCounterP_offset->GetValue() + m_spinCounterI_step->GetValue() * (pano_count - 1));
        }
        else
        {
            m_spinCounterP_end->SetValue(m_spinCounterP_offset->GetValue());
        };
    }
    else
    {
        m_spinCounterP_end->Enable(true);
        m_spinCounterP_end->SetRange(m_spinCounterP_offset->GetValue(), m_spinCounterP_end->GetMax());
        if (m_spinCounterP_end->GetValue() < m_spinCounterP_offset->GetValue())
        {
            m_spinCounterP_end->SetValue(m_spinCounterP_offset->GetValue());
        };
        pano_count = (m_spinCounterP_end->GetValue() - m_spinCounterP_offset->GetValue()) / m_spinCounterP_step->GetValue() + 1;
    };
    // update end value for image counter
    m_spinCounterI_end->Enable(false);
    m_spinCounterI_end->SetValue(m_spinCounterI_offset->GetValue() + m_spinCounterI_step->GetValue() * (m_pano.getNrOfImages() - 1));
    // update end value for x counter
    m_spinCounterX_end->Enable(false);
    m_spinCounterX_end->SetValue(m_spinCounterX_offset->GetValue() + m_spinCounterX_step->GetValue() * (m_pano.getNrOfImages() * pano_count - 1));
}

void GenerateSequenceDialog::GenerateFileList(wxArrayString& panoSubDirs, std::vector<wxArrayString>& fileList)
{
    // read all counter values
    // first update all counters if necessary
    UpdateCounters();
    const size_t pano_offset = m_spinCounterP_offset->GetValue();
    const size_t pano_step = m_spinCounterP_step->GetValue();
    const size_t pano_end = m_spinCounterP_end->GetValue();
    const size_t pano_width = XRCCTRL(*this, "sequence_p_width", wxSpinCtrl)->GetValue();
    const size_t img_offset = m_spinCounterI_offset->GetValue();
    const size_t img_step = m_spinCounterI_step->GetValue();
    const size_t img_width = XRCCTRL(*this, "sequence_i_width", wxSpinCtrl)->GetValue();
    const size_t x_offset = m_spinCounterX_offset->GetValue();
    const size_t x_step = m_spinCounterX_step->GetValue();
    const size_t x_width = XRCCTRL(*this, "sequence_x_width", wxSpinCtrl)->GetValue();
    const size_t pano_count = (pano_end - pano_offset) / pano_step + 1;
    const size_t img_count = m_pano.getNrOfImages();
    // generate list of all sub-directories
    if (m_choiceSubDir->GetSelection() == 1)
    {
        panoSubDirs = GetAllSubDirectories(m_basepath->GetValue());
        if (panoSubDirs.IsEmpty())
        {
            wxBell();
            return;
        };
    }
    else
    {
        wxString panoNameTemplate = m_subDirTextCtrl->GetValue();
        panoSubDirs.resize(pano_count);
        if (!panoNameTemplate.IsEmpty())
        {
            size_t panoCounter = pano_offset;
            for (size_t i = 0; i < pano_count; ++i, panoCounter += pano_step)
            {
                panoSubDirs[i] = panoNameTemplate;
                // replace placeholder
                panoSubDirs[i].Replace("%p", GetNumberString(panoCounter, pano_width), true);
            };
        };
    };
    // now iterate all panoramas and generate file list
    size_t panoCounter = pano_offset;
    size_t xCounter = x_offset;
    for (size_t i = 0; i < pano_count; ++i, panoCounter += pano_step)
    {
        wxFileName currentPath;
        currentPath.SetPath(m_basepath->GetValue());
        if (!panoSubDirs[i].IsEmpty())
        {
            currentPath.AppendDir(panoSubDirs[i]);
        };
        size_t imgCounter = img_offset;
        wxArrayString panoFileList;
        for (size_t j = 0; j < m_pano.getNrOfImages(); ++j, imgCounter += img_step, xCounter += x_step)
        {
            // replace all counter variables
            wxString file = m_mappedFilenames[j];
            // replace all placeholders
            file.Replace("%p", GetNumberString(panoCounter, pano_width), true);
            file.Replace("%i", GetNumberString(imgCounter, img_width), true);
            file.Replace("%x", GetNumberString(xCounter, x_width), true);
            wxFileName fileName(file);
            fileName.MakeAbsolute(currentPath.GetPath());
            panoFileList.Add(fileName.GetFullPath());
        }
        fileList.push_back(panoFileList);
    };
}

void GenerateSequenceDialog::OnGeneratePreview(wxCommandEvent& e)
{
    wxWindowDisabler winDisable;
#if wxCHECK_VERSION(3,1,0)
    wxBusyInfo waitInfo(
        wxBusyInfoFlags()
        .Parent(this)
        .Text(_("Generating preview list. Please wait..."))
        .Icon(GetIcon())
    );
#else
    wxBusyCursor waitCursor;
#endif
    // generate a preview of all images in text form
    wxArrayString subDirList;
    std::vector<wxArrayString> fileList;
    GenerateFileList(subDirList, fileList);
    wxTextCtrl* preview = XRCCTRL(*this, "sequence_preview", wxTextCtrl);
    preview->Clear();
    wxString text;
#ifdef __WXMSW__
    const wxString linebreak("\r\n");
#else
    const wxString linebreak("\n");
#endif
    if (fileList.empty())
    {
        text.Append(_("No matching sub-directories found."));
    }
    else
    {
        // iterate above all panoramas
        for (size_t i = 0; i < fileList.size(); ++i)
        {
            text.Append(wxString::Format(_("Panorama %d"), i));
            text.Append(linebreak);
            if (!subDirList[i].IsEmpty())
            {
                text.Append(wxString::Format(_("Sub-directory: %s"), subDirList[i].c_str()));
                text.Append(linebreak);
            };
            // iterate above all files in current pano
            bool missingFiles = false;
            for (size_t j = 0; j < m_pano.getNrOfImages(); ++j)
            {
                text.Append("\t");
                text.Append(fileList[i][j]);
                if (!wxFileName::FileExists(fileList[i][j]))
                {
                    missingFiles = true;
                    text.Append(" <-- ");
                    text.Append(_("File not found"));
                };
                text.Append(linebreak);
            };
            if (missingFiles)
            {
                text.Append("\t");
                text.Append(_("This panorama will be skipped becaused of missing files."));
                text.Append(linebreak);
            };
        };
    };
    preview->SetValue(text);
}

/** copy the image files from pano to newPano -> this corresponds to apply the template
    newPano to pano
    the pano with applied template can be found in newPano */
void ApplyTemplate(const HuginBase::Panorama& pano, HuginBase::Panorama& newPano)
{
    // check image sizes, and correct parameters if required.
    HuginBase::VariableMapVector vars(newPano.getNrOfImages());
    for (unsigned int i = 0; i < newPano.getNrOfImages(); i++)
    {
        // check if image size is correct
        const HuginBase::SrcPanoImage& oldSrcImg = pano.getImage(i);
        HuginBase::SrcPanoImage newSrcImg = newPano.getSrcImage(i);

        // just keep the file name
        newSrcImg.setFilename(oldSrcImg.getFilename());
        if (oldSrcImg.getSize() != newSrcImg.getSize())
        {
            // adjust size properly.
            newSrcImg.resize(oldSrcImg.getSize(), &vars[i]);
        }
        newPano.setSrcImage(i, newSrcImg);
    }
    // now update all possible linked variables
    for (unsigned int i = 0; i < newPano.getNrOfImages(); ++i)
    {
        if (!vars[i].empty())
        {
            newPano.updateVariables(i, vars[i]);
        };
    };
    // remove all control points, they are only valid for the original template
    newPano.setCtrlPoints(HuginBase::CPVector());
}

/** generate filename from given settings and panorama
    append a number if file already exists */
bool GetNewProjectFilename(long index, const HuginBase::Panorama& pano, const wxString basePath, wxFileName& projectFile, unsigned int currentIndex)
{
    wxString mask;
    projectFile.SetPath(basePath);
    projectFile.SetExt(wxT("pto"));
    if (!projectFile.IsDirWritable())
    {
        return false;
    };
    switch (index)
    {
        case 0:
            // panoramaXX.pto
            mask = wxString::Format("panorama%d", currentIndex);
            break;
        case 1:
            // First file - last file.pto
            {
                const wxFileName f1(wxString(pano.getImage(0).getFilename().c_str(), HUGIN_CONV_FILENAME));
                const wxFileName f2(wxString(pano.getImage(pano.getNrOfImages() - 1).getFilename().c_str(), HUGIN_CONV_FILENAME));
                mask = f1.GetName() + wxT("-") + f2.GetName();
            }
            break;
        case 2:
            // Foldername.pto
            {
                wxArrayString folders = projectFile.GetDirs();
                if (folders.GetCount() == 0)
                {
                    return false;
                }
                mask = folders.Last();
            }
            break;
        case 3:
            // Template from preferences
            {
                wxFileName newProject(getDefaultProjectName(pano));
                mask = newProject.GetName();
                projectFile.SetName(mask);
            };
            break;
        default:
            mask = wxString::Format("panorama%d", currentIndex);
    };

    projectFile.SetName(mask);
    if (projectFile.FileExists())
    {
        unsigned int i = 1;
        mask = mask.Append("_%d");
        projectFile.SetName(wxString::Format(mask, i));
        while (projectFile.FileExists())
        {
            i++;
            projectFile.SetName(wxString::Format(mask, i));
            //security fall through
            if (i > 1000)
            {
                return false;
            };
        };
    };
    return true;
}

void GenerateSequenceDialog::DoGeneratePanorama(const Project::Target target)
{
    wxWindowDisabler winDisable;
#if wxCHECK_VERSION(3,1,0)
    wxBusyInfo waitInfo(
        wxBusyInfoFlags()
            .Parent(this)
            .Text(_("Generating panorama files. Please wait..."))
            .Icon(GetIcon())
    );
#else
    wxBusyCursor waitCursor;
#endif
    wxArrayString subDirList;
    std::vector<wxArrayString> fileList;
    GenerateFileList(subDirList, fileList);
    if (fileList.empty())
    {
        wxMessageBox(_("No matching sub-directories found."),
#ifdef __WXMSW__
            wxT("PTBatcherGUI"),
#else
            wxEmptyString,
#endif
            wxOK | wxICON_EXCLAMATION, NULL);
    }
    else
    {
        const wxString basePath = m_basepath->GetValue();
        wxArrayString ptoFileList;

        for (size_t i = 0; i < fileList.size(); ++i)
        {
            // check that all files exists
            bool missingFiles = false;
            for (size_t j = 0; j < m_pano.getNrOfImages(); ++j)
            {
                if (!wxFileName::FileExists(fileList[i][j]))
                {
                    missingFiles = true;
                    break;
                };
            };
            if (!missingFiles)
            {
                // now build the new panorama object
                HuginBase::Panorama newPano;
                bool allFilesReadable = true;
                for (size_t j = 0; j < m_pano.getNrOfImages(); ++j)
                {
                    HuginBase::SrcPanoImage image;
                    image.setFilename(std::string(fileList[i][j].mb_str(HUGIN_CONV_FILENAME)));
                    if (!image.checkImageSizeKnown())
                    {
                        allFilesReadable = false;
                        break;
                    };
                    newPano.addImage(image);
                };
                if (allFilesReadable)
                {
                    HuginBase::Panorama newGeneratedPano = m_pano.duplicate();
                    ApplyTemplate(newPano, newGeneratedPano);
                    // write to disc
                    wxFileName projectFile;
                    if (!GetNewProjectFilename(XRCCTRL(*this, "sequence_naming", wxChoice)->GetSelection(), newGeneratedPano, basePath, projectFile, i))
                    {
                        break;
                    };
                    std::ofstream script(projectFile.GetFullPath().mb_str(HUGIN_CONV_FILENAME));
                    script.exceptions(std::ofstream::eofbit | std::ofstream::failbit | std::ofstream::badbit);
                    if (!script.good())
                    {
                        break;
                    };
                    HuginBase::UIntSet all;
                    fill_set(all, 0, newGeneratedPano.getNrOfImages() - 1);
                    try
                    {
                        newGeneratedPano.printPanoramaScript(script, newGeneratedPano.getOptimizeVector(), newGeneratedPano.getOptions(), all, false, 
                            hugin_utils::getPathPrefix(std::string(projectFile.GetFullPath().mb_str(HUGIN_CONV_FILENAME))));
                    }
                    catch (...)
                    {
                        break;
                    };
                    script.close();
                    // all done, remember pto file name
                    ptoFileList.Add(projectFile.GetFullPath());
                };
            };
        };
        // all done, now add the pto files to queue
        if (!ptoFileList.IsEmpty())
        {
            m_batchframe->AddArrayToList(ptoFileList, target);
        };
        EndModal(wxID_OK);
    };
}

void GenerateSequenceDialog::OnGenerateStitchingPanorama(wxCommandEvent& e)
{
    DoGeneratePanorama(Project::STITCHING);
}

void GenerateSequenceDialog::OnGenerateAssistantPanorama(wxCommandEvent& e)
{
    DoGeneratePanorama(Project::DETECTING);
}
