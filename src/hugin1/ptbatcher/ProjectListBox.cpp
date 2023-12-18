// -*- c-basic-offset: 4 -*-

/** @file ProjectListBox.cpp
 *
 *  @brief Batch processor for Hugin with GUI
 *
 *  @author Marko Kuder <marko.kuder@gmail.com>
 *
 *  $Id: ProjectListBox.cpp 3322 2008-08-16 5:00:07Z mkuder $
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This software is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public
 *  License along with this software. If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#ifdef _WIN32
#include "wx/msw/wrapwin.h"
#endif
#include "ProjectListBox.h"

enum
{
    ID_CHANGE_PREFIX=wxID_HIGHEST+200,
    ID_RESET_PROJECT=wxID_HIGHEST+201,
    ID_EDIT_PROJECT=wxID_HIGHEST+202,
    ID_REMOVE_PROJECT=wxID_HIGHEST+203,
    ID_CHANGE_USER_DEFINED=wxID_HIGHEST+204
};

BEGIN_EVENT_TABLE(ProjectListBox, wxListCtrl)
    EVT_LIST_COL_END_DRAG(wxID_ANY, ProjectListBox::OnColumnWidthChange)
    EVT_CONTEXT_MENU(ProjectListBox::OnContextMenu)
    EVT_MENU(ID_CHANGE_PREFIX, ProjectListBox::OnChangePrefix)
    EVT_MENU(ID_RESET_PROJECT, ProjectListBox::OnResetProject)
    EVT_MENU(ID_EDIT_PROJECT, ProjectListBox::OnEditProject)
    EVT_MENU(ID_REMOVE_PROJECT, ProjectListBox::OnRemoveProject)
    EVT_MENU(ID_CHANGE_USER_DEFINED, ProjectListBox::OnChangeUserDefined)
    EVT_CHAR(ProjectListBox::OnChar)
END_EVENT_TABLE()

bool ProjectListBox::Create(wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name)
{
    if (! wxListCtrl::Create(parent, id, pos, size, wxLC_REPORT | style) )
    {
        return false;
    };
    columns.Add(ID);
    columns.Add(PROJECT);
    columns.Add(PREFIX);
    columns.Add(STATUS);
    columns.Add(MODDATE);
    columns.Add(FORMAT);
    columns.Add(PROJECTION);
    columns.Add(SIZE);
    columns.Add(USERDEFINEDSEQUENCE);

    this->InsertColumn(0,_("ID"));
    this->InsertColumn(1,_("Project"));
    this->InsertColumn(2,_("Output prefix"));
    this->InsertColumn(3,_("Status"));
    this->InsertColumn(4,_("Last modified"));
    this->InsertColumn(5,_("Output format"));
    this->InsertColumn(6,_("Projection"));
    this->InsertColumn(7,_("Size"));
    this->InsertColumn(8, _("User defined sequence"));

    //get saved width
    for( int i=0; i < GetColumnCount() ; i++ )
    {
        int width = wxConfigBase::Get()->Read(wxString::Format(wxT("/BatchList/ColumnWidth%d"), columns[i] ), -1);
        if(width != -1)
        {
            SetColumnWidth(i, width);
        }
    }
    return true;
}

//public methods:

void ProjectListBox::AppendProject(Project* project)
{
    //if we have a command-line application
    if(project->id < 0) 
    {
        int i=columns.Index(PROJECT);	//we find the project column
        if(i != wxNOT_FOUND)
        {
            if(i==0)
            {
                this->InsertItem(this->GetItemCount(),project->path);
            }
            else
            {
                this->InsertItem(this->GetItemCount(),_T(""));
                this->SetItem(this->GetItemCount()-1,i,project->path);
            }
        }
        else	//we insert an empty line
        {
            this->InsertItem(this->GetItemCount(),_T(""));
        }
    }
    else
    {
        if(columns.GetCount()>0)
        {
            this->InsertItem(this->GetItemCount(),this->GetAttributeString(columns[0],project));
            for(unsigned int i=1; i<columns.GetCount(); i++)
            {
                this->SetItem(this->GetItemCount()-1,i,this->GetAttributeString(columns[i],project));
            }
        }
        else	//we have no columns?
        {
            this->InsertItem(this->GetItemCount(),_T(""));
        }
    }
}

void ProjectListBox::ChangePrefix(int index, wxString newPrefix)
{
    int colIndex=columns.Index(PREFIX);
    if(colIndex!=wxNOT_FOUND)
    {
        Project* project=m_batch->GetProject(index);
        this->SetItem(index,colIndex,this->GetAttributeString(colIndex,project));
    }
}

void ProjectListBox::ChangeUserDefined(int index, wxString newUserDefined)
{
    int colIndex = columns.Index(USERDEFINEDSEQUENCE);
    if (colIndex != wxNOT_FOUND)
    {
        Project* project = m_batch->GetProject(index);
        this->SetItem(index, colIndex, this->GetAttributeString(colIndex, project));
    }
}

void ProjectListBox::Deselect(int index)
{
    SetItemState(index, 0, wxLIST_STATE_SELECTED|wxLIST_STATE_FOCUSED);
}

void ProjectListBox::Fill(Batch* batch)
{
    m_batch=batch;
    for(int i=0; i<m_batch->GetProjectCount(); i++)
    {
        AppendProject(m_batch->GetProject(i));
    }
}

int ProjectListBox::GetIndex(int id)
{
    int index=0;
    while(index<this->GetItemCount())
    {
        if(GetText(index,0).Cmp(wxString::Format(_("%d"),id))==0)
        {
            return index;
        }
        index++;
    }
    return -1;
}

int ProjectListBox::GetProjectCountByPath(wxString path)
{
    int count = 0;
    for(int i=0; i<this->GetItemCount(); i++)
    {
        if(path.Cmp(GetText(i,1))==0)
        {
            count++;
        }
    }
    return count;
}

int ProjectListBox::GetProjectId(int index)
{
    long id=-1;
    if(!GetText(index,0).ToLong(&id))
    {
        wxMessageBox(_("Error, cannot convert id"),_("Error"));
    }
    return (int)id;
}

HuginBase::UIntSet ProjectListBox::GetSelectedProjects()
{
    HuginBase::UIntSet selected;
    for (int i = 0; i<GetItemCount(); i++)
    {
        if (GetItemState(i, wxLIST_STATE_SELECTED) & wxLIST_STATE_SELECTED)
        {
            selected.insert(i);
        };
    };
    return selected;
}

wxString ProjectListBox::GetText(int row, int column)
{
    wxListItem item;
    item.SetId(row);
    item.SetColumn(column);
    item.SetMask(wxLIST_MASK_TEXT);
    this->GetItem(item);
    return item.GetText();
}

void ProjectListBox::ReloadProject(int index, Project* project)
{
    for(unsigned int i=0; i<columns.GetCount(); i++)
    {
        this->SetItem(index,i,this->GetAttributeString(columns[i],project));
    }
}

void ProjectListBox::Select(int index)
{
    if(index>=0 && index<this->GetItemCount())
    {
        SetItemState(index,wxLIST_STATE_SELECTED,wxLIST_STATE_SELECTED);
    };
}

void ProjectListBox::SetMissing(int index)
{
    for(int i=0; i< this->GetColumnCount(); i++)
    {
        if(columns[i]==STATUS)
        {
            this->SetItem(index,i,_("File missing"));
        }
        if(columns[i]!=ID && columns[i]!=PROJECT && columns[i]!=PREFIX && columns[i]!=USERDEFINEDSEQUENCE)
        {
            this->SetItem(index,i,_T(""));
        }
    }
}

void ProjectListBox::SwapProject(int index)
{
    wxString temp;
    for(int i=0; i<GetColumnCount(); i++)
    {
        temp = GetText(index,i);
        SetItem(index,i,GetText(index+1,i));
        SetItem(index+1,i,temp);
    }
}

bool ProjectListBox::UpdateStatus(int index, Project* project)
{
    bool change = false;
    wxString newStatus;
    for(int i=0; i< this->GetColumnCount(); i++)
    {
        if(columns[i]==STATUS)
        {
            newStatus = project->GetStatusText();
            if(newStatus.Cmp(GetText(index,i))!=0)
            {
                change = true;
                this->SetItem(index,i,newStatus);
            }
        }
    }
    return change;
}

//private methods:

wxString ProjectListBox::GetAttributeString(int i, Project* project)
{
    wxString str;
    switch(i)
    {
        case 0:
            return wxString::Format(_T("%ld"),project->id);
        case 1:
            return project->path;
        case 2:
            if(project->target==Project::STITCHING)
            {
                //make prefix relative to project path
                wxFileName prefix(project->prefix);
                wxFileName projectFile(project->path);
                prefix.MakeRelativeTo(projectFile.GetPath());
                return prefix.GetFullPath();
            }
            else
            {
                return _("Assistant");
            };
        case 7:
            return project->GetStatusText();	
        //all following cases default to an empty string if file is missing
        case 3:
            if(project->status!=Project::MISSING)
            {
                if (project->modDate.IsValid())
                {
                    return project->modDate.Format();
                };
            };
            return wxEmptyString;
        case 4:
            if(project->status!=Project::MISSING)
            {
                str = GetLongerFormatName(project->options.outputImageType);
                str = str+wxT(" (.")+wxString::FromAscii(project->options.outputImageType.c_str())+wxT(")");
                return str;
            };
            return wxEmptyString;
        case 5:
            if(project->status!=Project::MISSING)
            {
                pano_projection_features proj;
                if (panoProjectionFeaturesQuery(project->options.getProjection(), &proj))
                {
                    wxString str2(proj.name, wxConvLocal);
                    return wxGetTranslation(str2);
                }
                else
                {
                    return _T("");
                }
            };
            return wxEmptyString;
        case 6:
            if(project->status!=Project::MISSING)
            {
                str = wxString() << project->options.getWidth();
                str = str+_T("x");
                str = str << project->options.getHeight();
                return str;
            }
            return wxEmptyString;
        case 8:
            if (!project->userDefindSequence.IsEmpty())
            {
                return project->userDefindSequence;
            };
            return wxEmptyString;
        default:
            return _T("");
    }
}

wxString ProjectListBox::GetLongerFormatName(std::string str)
{
    if(str=="tif")
    {
        return _T("TIFF");
    }
    else if(str=="jpg")
    {
        return _T("JPEG");
    }
    else if(str=="png")
    {
        return _T("PNG");
    }
    else if(str=="exr")
    {
        return _T("EXR");
    }
    else
    {
        return _T("");
    }
}

void ProjectListBox::OnColumnWidthChange(wxListEvent& event)
{
    int col = event.GetColumn();
    wxConfigBase::Get()->Write(wxString::Format(wxT("/BatchList/ColumnWidth%d"),columns[col]), GetColumnWidth(col));
}

// functions for context menu
void ProjectListBox::OnContextMenu(wxContextMenuEvent& e)
{
    const HuginBase::UIntSet selected(GetSelectedProjects());
    wxPoint point = e.GetPosition();
    // if from keyboard
    if ((point.x == -1) && (point.y == -1))
    {
        wxSize size = GetSize();
        point.x = size.x / 2;
        point.y = size.y / 2;
    }
    else
    {
        point = ScreenToClient(point);
    };
    if (selected.size() == 1)
    {
        wxMenu menu;
        menu.Append(ID_CHANGE_PREFIX, _("Change prefix"));
        menu.Append(ID_CHANGE_USER_DEFINED, _("Change user defined sequence"));
        menu.Append(ID_RESET_PROJECT, _("Reset project"));
        menu.Append(ID_EDIT_PROJECT, _("Edit with Hugin"));
        menu.Append(ID_REMOVE_PROJECT, _("Remove"));
        PopupMenu(&menu, point.x, point.y);
    }
    else
    {
        if (selected.size() > 1)
        {
            wxMenu menu;
            // check that all selected project have the same target
            const Project::Target target = m_batch->GetProject(*(selected.begin()))->target;
            bool isSameTarget = true;
            for (const auto& i : selected)
            {
                if (m_batch->GetProject(i)->target != target)
                {
                    isSameTarget = false;
                    break;
                };
            };
            if (isSameTarget)
            {
                menu.Append(ID_CHANGE_USER_DEFINED, _("Change user defined sequence"));
            };
            menu.Append(ID_RESET_PROJECT, _("Reset project"));
            menu.Append(ID_REMOVE_PROJECT, _("Remove"));
            PopupMenu(&menu, point.x, point.y);
        };
    };
};

void ProjectListBox::OnChar(wxKeyEvent& e)
{
    switch (e.GetKeyCode())
    {
        case WXK_DELETE:
        case WXK_NUMPAD_DELETE:
            {
                wxCommandEvent ev(wxEVT_TOOL, XRCID("tool_remove"));
                GetParent()->GetEventHandler()->AddPendingEvent(ev);
            };
            break;
        case WXK_INSERT:
        case WXK_NUMPAD_INSERT:
            {
                wxCommandEvent ev(wxEVT_MENU, XRCID("menu_add"));
                GetParent()->GetEventHandler()->AddPendingEvent(ev);
            };
            break;
        case WXK_ESCAPE:
            {
                wxCommandEvent ev(wxEVT_TOOL, XRCID("tool_cancel"));
                GetParent()->GetEventHandler()->AddPendingEvent(ev);
            };
            break;
        default:
            e.Skip();
    };
}
void ProjectListBox::OnChangePrefix(wxCommandEvent& e)
{
    wxCommandEvent ev(wxEVT_COMMAND_BUTTON_CLICKED, XRCID("button_prefix"));
    GetParent()->GetEventHandler()->AddPendingEvent(ev);
};

void ProjectListBox::OnResetProject(wxCommandEvent& e)
{
    wxCommandEvent ev(wxEVT_COMMAND_BUTTON_CLICKED, XRCID("button_reset"));
    GetParent()->GetEventHandler()->AddPendingEvent(ev);
};

void ProjectListBox::OnEditProject(wxCommandEvent& e)
{
    wxCommandEvent ev(wxEVT_COMMAND_BUTTON_CLICKED, XRCID("button_edit"));
    GetParent()->GetEventHandler()->AddPendingEvent(ev);
};

void ProjectListBox::OnRemoveProject(wxCommandEvent& e)
{
    wxCommandEvent ev(wxEVT_COMMAND_TOOL_CLICKED, XRCID("tool_remove"));
    GetParent()->GetEventHandler()->AddPendingEvent(ev);
};

void ProjectListBox::OnChangeUserDefined(wxCommandEvent& e)
{
    wxCommandEvent ev(wxEVT_COMMAND_BUTTON_CLICKED, XRCID("button_user_defined"));
    GetParent()->GetEventHandler()->AddPendingEvent(ev);
}

const wxString ProjectListBox::fileFormat[] = {_T("JPEG"),
        _T("JPEG_m"),
        _T("PNG"),
        _T("PNG_m"),
        _T("TIFF"),
        _T("TIFF_m"),
        _T("TIFF_mask"),
        _T("TIFF_multilayer"),
        _T("TIFF_multilayer_mask"),
        _T("PICT"),
        _T("PSD"),
        _T("PSD_m"),
        _T("PSD_mask"),
        _T("PAN"),
        _T("IVR"),
        _T("IVR_java"),
        _T("VRML"),
        _T("QTVR"),
        _T("HDR"),
        _T("HDR_m"),
        _T("EXR"),
        _T("EXR_m"),
        _T("FILEFORMAT_NULL")
                                              };

const wxString ProjectListBox::outputMode[] =
{
    _T("OUTPUT_LDR"),
    _T("OUTPUT_HDR")
};

const wxString ProjectListBox::HDRMergeType[] =
{
    _T("HDRMERGE_AVERAGE"),
    _T("HDRMERGE_DEGHOST")
};

const wxString ProjectListBox::blendingMechanism[] =
{
    _T("NO_BLEND"),
    _T("PTBLENDER_BLEND"),
    _T("ENBLEND_BLEND"),
    _T("SMARTBLEND_BLEND"),
    _T("PTMASKER_BLEND")
};

IMPLEMENT_DYNAMIC_CLASS(ProjectListBox, wxListCtrl)

ProjectListBoxXmlHandler::ProjectListBoxXmlHandler()
    : wxListCtrlXmlHandler()
{
    AddWindowStyles();
}

wxObject* ProjectListBoxXmlHandler::DoCreateResource()
{
    XRC_MAKE_INSTANCE(cp, ProjectListBox)

    cp->Create(m_parentAsWindow,
               GetID(),
               GetPosition(), GetSize(),
               GetStyle(wxT("style")),
               GetName());

    SetupWindow( cp);

    return cp;
}

bool ProjectListBoxXmlHandler::CanHandle(wxXmlNode* node)
{
    return IsOfClass(node, wxT("ProjectListBox"));
}

IMPLEMENT_DYNAMIC_CLASS(ProjectListBoxXmlHandler, wxListCtrlXmlHandler)

