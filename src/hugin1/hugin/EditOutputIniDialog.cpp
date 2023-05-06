// -*- c-basic-offset: 4 -*-

/** @file EditOutputIniDialog.cpp
 *
 *	@brief implementation of dialog for editing user-defined output ini file
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

#include "EditOutputIniDialog.h"
#include "hugin_config.h"
#include "panoinc_WX.h"
#include "hugin/huginApp.h"
#include <wx/fileconf.h>
#include <wx/wfstream.h>
#include <wx/sstream.h>
#include <wx/propgrid/advprops.h>

static wxArrayString BlenderArray;
static wxArrayString ProjectionArray;
static wxArrayString OutputTypeArray;
static wxArrayString LDRFileTypeArray;
static wxArrayString HDRFileTypeArray;

void InitArrays()
{
    BlenderArray.Add("Enblend");
    BlenderArray.Add("Internal");

    int nP = panoProjectionFormatCount();
    for (int n = 0; n < nP; n++)
    {
        pano_projection_features proj;
        if (panoProjectionFeaturesQuery(n, &proj))
        {
            ProjectionArray.Add(wxString(proj.name, wxConvLocal));
        };
    };

    OutputTypeArray.Add("Normal");
    OutputTypeArray.Add("StacksFusedBlended");
    OutputTypeArray.Add("ExposureLayersFused");
    OutputTypeArray.Add("HDR");
    OutputTypeArray.Add("Remap");
    OutputTypeArray.Add("RemapOrig");
    OutputTypeArray.Add("HDRRemap");
    OutputTypeArray.Add("FusedStacks");
    OutputTypeArray.Add("HDRStacks");
    OutputTypeArray.Add("ExposureLayers");

    LDRFileTypeArray.Add("jpg");
    LDRFileTypeArray.Add("tif");
    LDRFileTypeArray.Add("png");

    HDRFileTypeArray.Add("exr");
    HDRFileTypeArray.Add("tif");
}

/** default ini, if no one exists load this one */
static const wxString defaultIni
{
    "[Cylindrical]\n"
    "Canvas=70%\n"
    "Condition1=ImageCount>1\n"
    "Condition2=PanoVFOV<100\n"
    "Condition3=PanoHFOV>=100\n"
    "Crop=auto\n"
    "FOV=auto\n"
    "Projection=Cylindrical\n"
    "[Equirectangular]\n"
    "Canvas=70%\n"
    "Condition1=ImageCount>1\n"
    "Condition2=PanoVFOV>=100\n"
    "Crop=auto\n"
    "FOV=auto\n"
    "Projection=Equirectangular\n"
    "[Rectilinear]\n"
    "Canvas=70%\n"
    "Condition1=ImageCount>1\n"
    "Condition2=PanoHFOV<100\n"
    "Condition3=PanoVFOV<100\n"
    "Crop=auto\n"
    "FOV=auto\n"
    "Projection=Rectilinear"
};

/** check if given string is in wxArrayString, do comparison case insentive or case sensitive */
bool contains(const wxArrayString& stringArray, const wxString& string, bool caseInSensitive = true)
{
    if (caseInSensitive)
    {
        for (size_t i = 0; i < stringArray.GetCount(); ++i)
        {
            if (stringArray[i].CmpNoCase(string) == 0)
            {
                return true;
            };
        };
    }
    else
    {
        for (size_t i = 0; i < stringArray.GetCount(); ++i)
        {
            if (stringArray[i].Cmp(string) == 0)
            {
                return true;
            };
        };
    };
    return false;
}

EditOutputIniDialog::EditOutputIniDialog(wxWindow* parent) :wxDialog(parent, wxID_ANY, _("Edit assistant output settings"), wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
#ifdef __WXMSW__
    wxIconBundle myIcons(huginApp::Get()->GetXRCPath() + wxT("data/hugin.ico"), wxBITMAP_TYPE_ICO);
    SetIcons(myIcons);
#else
    wxIcon myIcon(huginApp::Get()->GetXRCPath() + wxT("data/hugin.png"), wxBITMAP_TYPE_PNG);
    // set the icon in the title bar
    SetIcon(myIcon);
#endif

    wxSizer* topSizer = new wxBoxSizer(wxVERTICAL);
    m_grid = new wxPropertyGridManager(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxPG_AUTO_SORT | wxPG_DESCRIPTION | wxPG_SPLITTER_AUTO_CENTER);
    // bind context menu only to the grid
    // when binding to whole control it would also be invoked above the text area below
    m_grid->GetGrid()->Bind(wxEVT_CONTEXT_MENU, &EditOutputIniDialog::OnContextMenu, this);
    topSizer->Add(m_grid, wxSizerFlags(1).Expand().Border(wxALL, 10));
    wxStdDialogButtonSizer* buttonSizer = CreateStdDialogButtonSizer(wxOK | wxCANCEL);
    buttonSizer->GetAffirmativeButton()->Bind(wxEVT_BUTTON, &EditOutputIniDialog::OnOk, this);
    topSizer->Add(buttonSizer, wxSizerFlags().Expand().Border(wxALL, 6));
    SetSizerAndFit(topSizer);
    InitArrays();
    ReadIni();
    RestoreFramePosition(this, "EditOutputIniDialog");
}

EditOutputIniDialog::~EditOutputIniDialog()
{
    StoreFramePosition(this, "EditOutputIniDialog");
}

void EditOutputIniDialog::OnOk(wxCommandEvent& e)
{
    WriteIni();
    EndModal(wxOK);
}

void EditOutputIniDialog::AddKey(wxPGProperty* section, const wxString& key, const wxString& value)
{
    if (key.SubString(0, 8).CmpNoCase("Condition") == 0)
    {
        wxPGProperty* p = m_grid->AppendIn(section, new wxStringProperty(key, wxPG_LABEL, value));
        p->SetHelpString(_("Define which condition should be fulfilled to apply this setting.\nThe variables CameraModel, CameraMaker, Lens, HFOV, LensType, Focallength, Focallength35mm, Filename, Path, ImageCount, PanoHFOV and PanoVFOV can be used.\nFor comparison you can use ==, !=, =~ and !~ for strings and ==, !=, <, <=, > and >= for numbers."));
        return;
    };
    if (key.CmpNoCase("Projection") == 0)
    {
        wxPGProperty* p = m_grid->AppendIn(section, new wxEnumProperty(key, wxPG_LABEL, ProjectionArray));
        if (value.IsEmpty())
        {
            p->SetValue(ProjectionArray[0]);
        }
        else
        {
            p->SetValue(value);
        };
        p->SetHelpString(_("Select which projection should be selected."));
        return;
    };
    if (key.CmpNoCase("FOV") == 0)
    {
        wxPGProperty* p = m_grid->AppendIn(section, new wxStringProperty(key, wxPG_LABEL, value));
        p->SetHelpString(_("Select which field of view should be set.\nCan be Auto for automatic detection, or a fixed horizontal field of view (e.g. 120) or horizontal x vertical field of view (e.g. 140x70)."));
        return;
    };
    if (key.CmpNoCase("Canvas") == 0)
    {
        wxPGProperty* p = m_grid->AppendIn(section, new wxStringProperty(key, wxPG_LABEL, value));
        p->SetHelpString(_("Select to which size the canvas should be set.\nFor automatic detection use Auto or a scale factor in percent (e.g. 70%).\nOr give a width x height (e.g. 8000x4000)."));
        return;
    };
    if (key.CmpNoCase("Crop") == 0)
    {
        wxPGProperty* p = m_grid->AppendIn(section, new wxStringProperty(key, wxPG_LABEL, value));
        p->SetHelpString(_("Select which crop should be used.\nFor automatic detection use Auto or AutoHDR.\nOr give 4 numbers as left, right, top and bottom. You can use absolute pixel values (e.g. 100,7900,1000,3900) or relative values by appending a percentage sign (e.g. 10,90,10,90%)."));
        return;
    };
    if (key.CmpNoCase("OutputExposure") == 0)
    {
        wxPGProperty* p = m_grid->AppendIn(section, new wxStringProperty(key, wxPG_LABEL, value));
        p->SetHelpString(_("Select which output exposure value should be used.\nFor automatic detection use Auto or give a fixed exposure value."));
        return;
    };
    if (key.CmpNoCase("OutputType") == 0)
    {
        // split string into individual items
        wxArrayString values = wxStringTokenize(value, ",");
        for (size_t i = 0; i < values.size(); ++i)
        {
            // remove whitespace from begin and end
            values[i].Trim(true).Trim(false);
        }
        wxPGProperty* p = m_grid->AppendIn(section, new wxMultiChoiceProperty(key, wxPG_LABEL, OutputTypeArray, values));
        p->SetHelpString(_("Select which output types should be activated."));
        return;
    };
    if (key.CmpNoCase("Blender") == 0)
    {
        wxPGProperty* p = m_grid->AppendIn(section, new wxEnumProperty(key, wxPG_LABEL, BlenderArray));
        if (value.IsEmpty())
        {
            p->SetValue(BlenderArray[0]);
        }
        else
        {
            p->SetValue(value);
        };
        p->SetHelpString(_("Select which blender should be used (can be enblend or internal)."));
        return;
    };
    if (key.CmpNoCase("BlenderArgs") == 0)
    {
        wxPGProperty* p = m_grid->AppendIn(section, new wxStringProperty(key, wxPG_LABEL, value));
        p->SetHelpString(_("Select which arguments should be passed to the blender."));
        return;
    };
    if (key.CmpNoCase("LDRFileType") == 0)
    {
        wxPGProperty* p = m_grid->AppendIn(section, new wxEnumProperty(key, wxPG_LABEL, LDRFileTypeArray));
        if (value.IsEmpty())
        {
            p->SetValue(LDRFileTypeArray[0]);
        }
        else
        {
            p->SetValue(value);
        };
        p->SetHelpString(_("Select fileformat for LDR output should be set."));
        return;
    };
    if (key.CmpNoCase("LDRCompression") == 0)
    {
        wxPGProperty* p = m_grid->AppendIn(section, new wxStringProperty(key, wxPG_LABEL, value));
        p->SetHelpString(_("Select which compressions should be used for LDR images.\nFor TIFF images NONE, PACKBITS, LZW and DEFLATE are supported.\nFor JPEG image give a number as quality (0-100)."));
        return;
    };
    if (key.CmpNoCase("HDRFileType") == 0)
    {
        wxPGProperty* p = m_grid->AppendIn(section, new wxEnumProperty(key, wxPG_LABEL, HDRFileTypeArray));
        if (value.IsEmpty())
        {
            p->SetValue(HDRFileTypeArray[0]);
        }
        else
        {
            p->SetValue(value);
        };
        p->SetHelpString(_("Select fileformat for HDR output should be set."));
        return;
    };
    if (key.CmpNoCase("HDRCompression") == 0)
    {
        wxPGProperty* p = m_grid->AppendIn(section, new wxStringProperty(key, wxPG_LABEL, value));
        p->SetHelpString(_("Select which compressions should be used for HDR images.\nFor TIFF images NONE, PACKBITS, LZW and DEFLATE are supported."));
        return;
    }
    // fall-through, should not be called
    wxPGProperty* newProperty = m_grid->AppendIn(section, new wxStringProperty(key, wxPG_LABEL, value));
};

void EditOutputIniDialog::ReadIni()
{
    wxFileName iniFilename = GetIniFileName();
    wxInputStream* iniStream{ nullptr };

    if (iniFilename.FileExists())
    {
        iniStream = new wxFileInputStream(iniFilename.GetFullPath());
        if (!iniStream->IsOk())
        {
            delete iniStream;
            iniStream = nullptr;
        }
    };
    if (iniStream == nullptr)
    {
        iniStream = new wxStringInputStream(defaultIni);
    };
    // now read from stream
    wxFileConfig iniFile(*iniStream);
    m_grid->Freeze();
    wxString section;
    long index;
    bool hasSections = iniFile.GetFirstGroup(section, index);
    while (hasSections)
    {
        wxPGProperty* pgSection = new wxPropertyCategory(section);
        m_grid->Append(pgSection);
        iniFile.SetPath(section);
        wxString key;
        long indexKey;
        bool hasKeys = iniFile.GetFirstEntry(key, indexKey);
        while (hasKeys)
        {
            AddKey(pgSection, key, iniFile.Read(key, wxEmptyString));
            hasKeys = iniFile.GetNextEntry(key, indexKey);
        };
        iniFile.SetPath("/");
        hasSections = iniFile.GetNextGroup(section, index);
    };
    m_grid->Thaw();
    delete iniStream;
};

void EditOutputIniDialog::WriteIni()
{
    wxFileConfig iniFile;
    // iterate all entries
    for (wxPropertyGridIterator it = m_grid->GetIterator(wxPG_ITERATE_ALL); !it.AtEnd(); it++)
    {
        wxPGProperty* p = it.GetProperty();
        if (!p)
        {
            continue;
        };
        if (p->IsCategory())
        {
            iniFile.SetPath("/" + p->GetLabel());
        }
        else
        {
            wxVariant value = p->GetValue();
            if (!value.IsNull())
            {
                if (value.GetType().CmpNoCase("string") == 0)
                {
                    const wxString s(value.GetString());
                    iniFile.Write(p->GetLabel(), s);
                }
                else
                {
                    if (value.GetType().CmpNoCase("long") == 0)
                    {
                        const long i = value.GetLong();
                        wxPGChoices choices = p->GetChoices();
                        iniFile.Write(p->GetLabel(), choices[i].GetText());
                    }
                    else
                    {
                        if (value.GetType().CmpNoCase("arrstring") == 0)
                        {
                            wxArrayString values = value.GetArrayString();
                            wxString s;
                            for (size_t i = 0; i < values.size(); ++i)
                            {
                                s << values[i] << ",";
                            };
                            if (!s.IsEmpty())
                            {
                                // remove the last separator
                                s.RemoveLast();
                            };
                            iniFile.Write(p->GetLabel(), s);
                        };
                    };
                };
            };
        };
    };
    // finally write to disc
    wxFileOutputStream iniStream(GetIniFileName().GetFullPath());
    bool success = true;
    if (iniStream.IsOk())
    {
        success = iniFile.Save(iniStream);
    };
    if (!success)
    {
        wxMessageBox(wxString::Format(_("Could not save ini file \"%s\"."), GetIniFileName().GetFullPath()), _("Error"), wxOK | wxICON_ERROR, this);
    };
}

wxFileName EditOutputIniDialog::GetIniFileName()
{
    return wxFileName(hugin_utils::GetUserAppDataDir(), "output.ini");
}

void EditOutputIniDialog::BuildAddContextMenu(wxMenu& menu, wxPGProperty* category, const bool addSeparator)
{
    // first get list of known values
    wxArrayString knownValues=GetChildren(category);
    m_currentSection = category;
    wxMenuItem* item = menu.Append(wxID_ANY, _("Add condition"));
    Bind(wxEVT_MENU, &EditOutputIniDialog::OnAddCondition, this, item->GetId());
    wxString val;
#define ADDKEYITEM(s) val=s;\
    if (!contains(knownValues, val))\
    {\
        item = menu.Append(wxID_ANY, wxString::Format(_("Add %s"), val.c_str()));\
        Bind(wxEVT_MENU, [this, category, val](wxCommandEvent&) { AddKey(category, val, wxEmptyString); m_grid->RefreshGrid(); }, item->GetId());\
    };
    ADDKEYITEM("Projection")
    ADDKEYITEM("FOV")
    ADDKEYITEM("Canvas")
    ADDKEYITEM("Crop")
    ADDKEYITEM("OutputExposure")
    ADDKEYITEM("OutputType")
    ADDKEYITEM("Blender")
    ADDKEYITEM("BlenderArgs")
    ADDKEYITEM("LDRFileType")
    ADDKEYITEM("LDRCompression")
    ADDKEYITEM("HDRFileType")
    ADDKEYITEM("HDRCompression")
#undef ADDKEYITEM
    // and finally a separator
    if (addSeparator)
    {
        menu.AppendSeparator();
    };
}

void EditOutputIniDialog::OnContextMenu(wxContextMenuEvent& e)
{
    wxPropertyGridHitTestResult hitTest = m_grid->GetGrid()->HitTest(m_grid->GetGrid()->CalcScrolledPosition(m_grid->ScreenToClient(e.GetPosition())));
    wxMenu contextMenu;
    wxMenuItem* menuItem = contextMenu.Append(wxID_ANY, _("Create new section"));
    Bind(wxEVT_MENU, &EditOutputIniDialog::OnAddSection, this, menuItem->GetId());
    contextMenu.AppendSeparator();
    wxPGProperty* prop = hitTest.GetProperty();
    if (prop)
    {
        if (prop->IsCategory())
        {
            BuildAddContextMenu(contextMenu, prop, true);
            // rename section menu item
            menuItem = contextMenu.Append(wxID_ANY, wxString::Format(_("Rename section %s"), prop->GetLabel().c_str()));
            Bind(wxEVT_MENU, &EditOutputIniDialog::OnRenameSection, this, menuItem->GetId());
        }
        else
        {
            BuildAddContextMenu(contextMenu, prop->GetParent(), true);
        };
        // remove section/item menu item
        if (prop->IsCategory())
        {
            menuItem = contextMenu.Append(wxID_ANY, wxString::Format(_("Remove section %s"), prop->GetLabel().c_str()));
        }
        else
        {
            menuItem = contextMenu.Append(wxID_ANY, wxString::Format(_("Remove item %s"), prop->GetLabel().c_str()));
        };
        Bind(wxEVT_MENU, [this, prop](wxCommandEvent&) { m_grid->DeleteProperty(prop); m_grid->RefreshGrid(); }, menuItem->GetId());
    }
    else
    {
        // user clicked below last entry, take last category as target
        wxPGProperty* lastCategory{ nullptr };
        for (wxPropertyGridIterator it = m_grid->GetGrid()->GetIterator(wxPG_ITERATE_CATEGORIES); !it.AtEnd(); it++)
        {
            lastCategory = it.GetProperty();
        }
        if(lastCategory)
        {
            BuildAddContextMenu(contextMenu, lastCategory, false);
        };
    };
    // show popup menu
    PopupMenu(&contextMenu);
}

wxArrayString EditOutputIniDialog::GetSections() const
{
    wxArrayString sections;
    for (wxPropertyGridIterator it = m_grid->GetIterator(wxPG_ITERATE_CATEGORIES); !it.AtEnd(); it++)
    {
        sections.Add(it.GetProperty()->GetLabel());
    };
    return sections;
}

wxArrayString EditOutputIniDialog::GetChildren(wxPGProperty* prop) const
{
    wxArrayString values;
    size_t counter = 0;
    for (wxPropertyGridIterator it = m_grid->GetIterator(wxPG_ITERATE_DEFAULT, prop); !it.AtEnd() && counter < prop->GetChildCount(); it++, ++counter)
    {
        values.Add(it.GetProperty()->GetLabel());
    };
    return values;
}

void EditOutputIniDialog::OnAddSection(wxCommandEvent& e)
{
    // get list of known sections
    const wxArrayString knownSections = GetSections();
    // ask user for new name
    wxTextEntryDialog dialog(this, _("Name of new section:"), _("Create new section"));
    if (dialog.ShowModal() == wxID_OK)
    {
        const wxString newSection = dialog.GetValue();
        // check if new name is unique
        if (contains(knownSections, newSection))
        {
            wxMessageBox(wxString::Format(_("Section \"%s\" is already defined.\nPlease use another name."), newSection.c_str()), _("Duplicate value."), wxOK | wxICON_ERROR);
        }
        else
        {
            // all ok, finally add new section to grid
            wxPGProperty* pgSection = new wxPropertyCategory(newSection);
            m_grid->AppendIn(m_grid->GetPageRoot(0), pgSection);
            m_grid->RefreshGrid();
        };
    };
}

void EditOutputIniDialog::OnRenameSection(wxCommandEvent& e)
{
    wxPGProperty* currentSection = m_grid->GetSelection();
    if (!currentSection->IsCategory())
    {
        return;
    };
    // get list of known sections, ignore current selected section
    wxArrayString knownSections=GetSections();
    knownSections.Remove(currentSection->GetLabel());
    // ask user for new name
    wxTextEntryDialog dialog(this, _("New name of section:"), _("Create new section"), currentSection->GetLabel());
    if (dialog.ShowModal() == wxID_OK)
    {
        const wxString newSection = dialog.GetValue();
        // check if new name is unique
        if (contains(knownSections, newSection))
        {
            wxMessageBox(wxString::Format(_("Section \"%s\" is already defined.\nPlease use another name."), newSection.c_str()), _("Duplicate value."), wxOK | wxICON_ERROR);
        }
        else
        {
            // all ok, finally add new section to grid
            currentSection->SetLabel(newSection);
            m_grid->RefreshGrid();
        };
    };
}

void EditOutputIniDialog::OnAddCondition(wxCommandEvent& e)
{
    wxArrayString children = GetChildren(m_currentSection);
    // get currently highest used number on condition
    size_t index = 0;
    for (size_t i = 0; i < children.GetCount(); ++i)
    {
        if (children[i].SubString(0, 8).CmpNoCase("Condition") == 0)
        {
            long x;
            if (children[i].Mid(9).ToLong(&x))
            {
                index = std::max<long>(index, x);
            };
        };
    };
    ++index;
    const wxString val = wxString::Format("Condition%d", index);
    AddKey(m_currentSection, val, wxEmptyString); 
    m_grid->RefreshGrid();
}
