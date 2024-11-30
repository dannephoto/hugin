// -*- c-basic-offset: 4 -*-

/** @file Batch.cpp
 *
 *  @brief Batch processor for Hugin
 *
 *  @author Marko Kuder <marko.kuder@gmail.com>
 *
 *  $Id: Batch.cpp 3322 2008-08-18 1:10:07Z mkuder $
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
#include "Batch.h"
#include <wx/stdpaths.h>
#ifdef __WXMSW__
#include <powrprof.h>
#ifdef _MSC_VER
#pragma comment(lib, "PowrProf.lib")
#endif
#endif
#include <wx/dir.h>
#ifndef __WXMSW__
#include <sys/wait.h>
#endif

BEGIN_EVENT_TABLE(Batch, wxFrame)
    EVT_END_PROCESS(-1, Batch::OnProcessTerminate)
END_EVENT_TABLE()

#if defined _WIN32 && defined Hugin_shared
DEFINE_LOCAL_EVENT_TYPE(EVT_BATCH_FAILED)
DEFINE_LOCAL_EVENT_TYPE(EVT_INFORMATION)
DEFINE_LOCAL_EVENT_TYPE(EVT_UPDATE_PARENT)
#else
DEFINE_EVENT_TYPE(EVT_BATCH_FAILED)
DEFINE_EVENT_TYPE(EVT_INFORMATION)
DEFINE_EVENT_TYPE(EVT_UPDATE_PARENT)
#endif

Batch::Batch(wxFrame* parent) : wxFrame(parent, wxID_ANY, _T("Batch"))
{
    //default flag settings
    deleteFiles = false;
    atEnd = DO_NOTHING;
#if wxCHECK_VERSION(3,1,0)
    m_resBlocker = NULL;
#endif
    overwrite = true;
    verbose = false;
    autoremove = false;
    autostitch = false;
    saveLog = false;
    m_cancelled = false;
    m_paused = false;
    m_running = false;
    m_clearedInProgress = false;
}

Batch::~Batch()
{
#if wxCHECK_VERSION(3,1,0)
    if (m_resBlocker != NULL)
    {
        delete m_resBlocker;
    };
#endif
}

void Batch::AddAppToBatch(wxString app)
{
    Project* newApp = new Project(app);
    m_projList.Add(newApp);
}

void Batch::AddProjectToBatch(wxString projectFile, wxString outputFile, wxString userDefinedSequence, Project::Target target)
{
    wxFileName projectName(projectFile);
    wxFileName outName(outputFile);
    projectName.Normalize(wxPATH_NORM_ABSOLUTE | wxPATH_NORM_DOTS | wxPATH_NORM_TILDE | wxPATH_NORM_SHORTCUT);
    outName.Normalize(wxPATH_NORM_ABSOLUTE | wxPATH_NORM_DOTS | wxPATH_NORM_TILDE | wxPATH_NORM_SHORTCUT);

    if(!outputFile.IsEmpty() || target==Project::DETECTING)
    {
        Project* proj = new Project(projectName.GetFullPath(), outName.GetFullPath(), userDefinedSequence, target);
        m_projList.Add(proj);
    }
    else
    {
        //on output set as "", it defaults to same path and name as project file
        Project* proj = new Project(projectName.GetFullPath(), wxEmptyString, userDefinedSequence);
        m_projList.Add(proj);
    }
}

bool Batch::AllDone()
{
    for(unsigned int i=0; i<m_projList.GetCount(); i++)
    {
        if(m_projList.Item(i).status==Project::WAITING ||
                m_projList.Item(i).status==Project::RUNNING ||
                m_projList.Item(i).status==Project::PAUSED)
        {
            return false;
        }
    }
    return true;
}

void Batch::AppendBatchFile(wxString file)
{
    if (wxFileName::FileExists(file))
    {
        wxFileInputStream fileStream(file);
        wxFileConfig batchFile(fileStream);
        //first line in file is idGenerator, we save it a temp variable, cause it gets set when adding projects
        const long idGenTemp = batchFile.ReadLong("/Main/CurrentID", 1l);
        const long projCount = batchFile.ReadLong("/Main/Count", 0l);
        //then for each project: project path, prefix, id, status, skip
        if (projCount > 0)
        {
            // we can not use GetFirstGroup/GetNextGroup because these function return the groups in alphabetical order
            // and not in the order in the file
            for (long i = 1; i <= projCount; ++i)
            {
                const wxString group = wxString::Format("Project_%ld", i);
                if (batchFile.HasGroup(group))
                {
                    batchFile.SetPath(group);
                    const wxString projectName = batchFile.Read("Project", wxEmptyString);
                    const wxString type = batchFile.Read("Type", "Stitching");
                    const long id = batchFile.ReadLong("Id", 1);
                    long status = batchFile.ReadLong("Status", (long)Project::WAITING);
                    const bool skip = batchFile.ReadBool("Skip", false);

                    //we add project to internal list
                    if (id < 0)
                    {
                        // negative ID means command
                        AddAppToBatch(projectName);
                    }
                    else
                    {
                        const wxString userDefinedSequence = batchFile.Read("UserDefinedSequence", wxEmptyString);
                        if (type.CmpNoCase("Stitching") == 0)
                        {
                            const wxString prefix = batchFile.Read("Prefix", wxEmptyString);
                            AddProjectToBatch(projectName, prefix, userDefinedSequence);
                        }
                        else
                        {
                            if (type.CmpNoCase("Detecting") == 0)
                            {
                                AddProjectToBatch(projectName, wxEmptyString, userDefinedSequence, Project::DETECTING);
                            }
                            else
                            {
                                //unknow type, skipping
                                continue;
                            };
                        };
                    };
                    //if status was RUNNING or PAUSED, we set it to FAILED
                    if (status == (long)Project::RUNNING || status == (long)Project::PAUSED)
                    {
                        status = (long)Project::FAILED;
                    }
                    m_projList.Last().id = id;
                    m_projList.Last().status = (Project::Status)status;
                    m_projList.Last().skip = batchFile.ReadBool("Skip", false);
                    batchFile.SetPath("/");
                };
            };
        };
        //we set the id generator we got from file
        Project::idGenerator = idGenTemp;
    };
}

void Batch::CancelBatch()
{
    m_cancelled = true;
    for(int i=0; i<GetRunningCount(); i++)
    {
        CancelProject(i);
    }
#if wxCHECK_VERSION(3,1,0)
    if (m_resBlocker != NULL)
    {
        delete m_resBlocker;
        m_resBlocker = NULL;
    };
#endif
    m_running = false;
}
void Batch::CancelProject(int index)
{
    wxCommandEvent event;
    if(GetRunningCount()==1)
    {
        m_paused = false;
    }
    m_stitchFrames.Item(index)->OnCancel(event);
    if(GetRunningCount()==0)
    {
        m_running = false;
    }
}
void Batch::ChangePrefix(int index, wxString newPrefix)
{
    m_projList.Item(index).prefix = newPrefix;
}

void Batch::ChangeUserDefined(int index, wxString newUserDefined)
{
    m_projList.Item(index).userDefindSequence = newUserDefined;
}

int Batch::ClearBatch()
{
    if(m_stitchFrames.GetCount()!=0)
    {
        wxMessageDialog message(this, _("Cannot clear batch in progress.\nDo you want to cancel it?"),
#ifdef _WIN32
                                _("PTBatcherGUI"),
#else
                                wxT(""),
#endif
                                wxYES_NO | wxICON_INFORMATION);
        if(message.ShowModal()==wxID_YES)
        {
            CancelBatch();

            //we set a flag so we don't process terminating events
            m_clearedInProgress = true;
            Project::idGenerator=1;
            m_projList.Clear();
            ((wxFrame*)GetParent())->SetStatusText(_("Cleared batch."));
            return 2;
        }
        return 1;
        //TO-DO: return
    }
    else
    {
        Project::idGenerator=1;
        m_projList.Clear();
        ((wxFrame*)GetParent())->SetStatusText(_("Cleared batch."));
        return 0;
    }
}

bool Batch::CompareProjectsInLists(int stitchListIndex, int batchListIndex)
{
    return m_stitchFrames.Item(stitchListIndex)->GetProjectId() == m_projList.Item(batchListIndex).id;
}

int Batch::GetFirstAvailable()
{
    unsigned int i = 0;
    while(i<m_projList.Count())
    {
        if(m_projList.Item(i).skip || m_projList.Item(i).status!=Project::WAITING)
        {
            i++;
        }
        else
        {
            break;
        }
    }
    if((m_projList.Count() == 0) || (i == m_projList.Count()))
    {
        //no projects are available anymore
        return -1;
    }
    else
    {
        return i;
    }
}

int Batch::GetIndex(int id)
{
    for(unsigned int i=0; i<m_projList.GetCount(); i++)
    {
        if(m_projList.Item(i).id==id)
        {
            return i;
        }
    }
    return -1;
}

Project* Batch::GetProject(int index)
{
    return (Project*)&m_projList.Item(index);
}

int Batch::GetProjectCount()
{
    return m_projList.GetCount();
}

int Batch::GetProjectCountByPath(wxString path)
{
    int count = 0;
    for(unsigned int i=0; i<m_projList.GetCount(); i++)
    {
        if(!m_projList.Item(i).skip && (path.Cmp(m_projList.Item(i).path)==0))
        {
            count++;
        }
    }
    return count;
}

int Batch::GetRunningCount()
{
    return m_stitchFrames.GetCount();
}

Project::Status Batch::GetStatus(int index)
{
    if((unsigned int)index<m_projList.GetCount())
    {
        return m_projList.Item(index).status;
    }
    else
    {
        wxMessageBox(wxString::Format(_("Error: Could not get status, project with index %d is not in list."),index),_("Error!"),wxOK | wxICON_INFORMATION );
    }
    return Project::MISSING;
}

bool Batch::IsRunning()
{
    return m_running;
};

bool Batch::IsPaused()
{
    return m_paused;
}

int Batch::LoadBatchFile(wxString file)
{
    int clearCode = ClearBatch();
    if(clearCode==0)
    {
        AppendBatchFile(file);
        return 0;
    }
    else if(clearCode==2)
    {
        AppendBatchFile(file);
        return 2;
    }
    else
    {
        wxMessageBox(_("Error: Could not load batch file."));
    };
    return 1;
}

int Batch::LoadTemp()
{
    wxString batchQueue = GetBatchFilename();
    //we load the data from the temp file
    if (wxFileName::FileExists(batchQueue))
    {
        AppendBatchFile(batchQueue);
    };
    return 0;
}

bool Batch::NoErrors()
{
    for(unsigned int i=0; i<m_projList.GetCount(); i++)
    {
        if(m_projList.Item(i).status==Project::FAILED)
        {
            return false;
        }
    }
    return true;
}

void Batch::OnProcessTerminate(wxProcessEvent& event)
{
    //we find the right pointer to remove
    unsigned int i = 0;
    while(i < m_stitchFrames.GetCount() &&
            m_stitchFrames.Item(i)->GetProjectId()!=event.GetId())
    {
        i++;
    }
    m_stitchFrames.RemoveAt(i);
    if(m_clearedInProgress)
    {
        if(m_stitchFrames.GetCount()==0)
        {
            m_paused = false;
            m_running = false;
            m_cancelled = false;
            m_clearedInProgress = false;
        }
    }
    else
    {
        if(m_stitchFrames.GetCount()==0)
        {
            m_paused = false;
        }
        i = GetIndex(event.GetId());
        wxString savedLogfile=wxEmptyString;
        if(saveLog || event.GetExitCode() != 0 || event.GetTimestamp()==-1)
        {
            //get filename for automatic saving of log file
            wxFileName logFile(m_projList.Item(i).path);
            logFile.MakeAbsolute();
            logFile.SetExt(wxT("log"));
            wxString name=logFile.GetName();
            unsigned int i=1;
            while(logFile.FileExists() && i<1000)
            {
                logFile.SetName(wxString::Format(wxT("%s_%d"),name.c_str(),i));
                i++;
            };
            if(i<1000)
            {
                //now save log file
                if((static_cast<RunStitchFrame*>(event.GetEventObject()))->SaveLog(logFile.GetFullPath()))
                {
                    savedLogfile=logFile.GetFullPath();
                }
            };
        };
        if (event.GetExitCode() != 0 || event.GetTimestamp()==-1) //timestamp is used as a fake exit code because it cannot be set manually
        {
            m_projList.Item(i).status=Project::FAILED;
            struct FailedProject failedProject;
            failedProject.project=m_projList.Item(i).path;
            failedProject.logfile=savedLogfile;
            //remember failed project
            m_failedProjects.push_back(failedProject);
        }
        else
        {
            m_projList.Item(i).status=Project::FINISHED;
            // don't sent event for command app
            if(m_projList.Item(i).id>=0)
            {
                bool notifyParent=false;
                if(autostitch && m_projList.Item(i).target==Project::DETECTING)
                {
                    wxFileName name(m_projList.Item(i).path);
                    AddProjectToBatch(m_projList.Item(i).path,name.GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR) + name.GetName(), wxEmptyString, Project::STITCHING);
                    notifyParent=true;
                };
                if(autoremove)
                {
                    RemoveProjectAtIndex(i);
                    SaveTemp();
                    notifyParent=true;
                };
                if(notifyParent)
                {
                    wxCommandEvent e(EVT_UPDATE_PARENT,wxID_ANY);
                    GetParent()->GetEventHandler()->AddPendingEvent(e);
                };
            };
        }
        if(!m_cancelled && !m_paused)
        {
            if(AllDone())
            {
                SaveTemp();
                m_running = false;
#if wxCHECK_VERSION(3,1,0)
                if (m_resBlocker != NULL)
                {
                    delete m_resBlocker;
                    m_resBlocker = NULL;
                };
#endif
                if(NoErrors())
                {
                    wxCommandEvent e(EVT_INFORMATION,wxID_ANY);
                    e.SetString(_("Batch successfully completed."));
                    // setting int to 1 to indicate we are finished
                    e.SetInt(1);
                    GetParent()->GetEventHandler()->AddPendingEvent(e);
                }
                else
                {
                    ((wxFrame*)GetParent())->SetStatusText(_("Batch completed with errors."));
                    if(atEnd==DO_NOTHING)
                    {
                        //notify parent, that at least one project failed
                        // show dialog only if we don't shutdown the computer or end PTBatcherGUI
                        wxCommandEvent e(EVT_BATCH_FAILED,wxID_ANY);
                        GetParent()->GetEventHandler()->AddPendingEvent(e);
                    };
                };
                switch (atEnd)
                {
                    case DO_NOTHING:
                        // no action needed
                        break;
                    case CLOSE_PTBATCHERGUI:
                        GetParent()->Close();
                        break;
                    case SHUTDOWN:
                        {
                            wxProgressDialog progress(_("Initializing shutdown..."), _("Shutting down..."), 49, this,
                                wxPD_AUTO_HIDE | wxPD_SMOOTH | wxPD_APP_MODAL | wxPD_CAN_ABORT | wxPD_CAN_SKIP);
                            progress.Fit();
                            int i = 0;
                            bool skip = false;
                            while (progress.Update(i, _("Shutting down..."), &skip))
                            {
                                if (skip || i == 50)
                                {
                                    wxShutdown(wxSHUTDOWN_POWEROFF);
                                }
                                i++;
#if defined __WXMSW__
                                Sleep(200);
#else
                                sleep(200);
#endif
                            }
                            progress.Close();
                        }
                        break;
                    case SUSPEND:
                    case HIBERNATE:
#ifdef __WXMSW__
                        {
                            wxString progressCaption(_("Prepare to hibernate..."));
                            wxString progressLabel(_("Initializing hibernating..."));
                            if (atEnd == SUSPEND)
                            {
                                progressCaption = wxString(_("Prepare to suspend..."));
                                progressLabel = wxString(_("Initializing suspend mode..."));
                            };
                            wxProgressDialog progress(progressLabel, progressCaption, 49, this,
                                wxPD_AUTO_HIDE | wxPD_SMOOTH | wxPD_APP_MODAL | wxPD_CAN_ABORT | wxPD_CAN_SKIP);
                            progress.Fit();
                            int i = 0;
                            bool skip = false;
                            while (progress.Update(i, progressCaption, &skip))
                            {
                                if (skip || i == 50)
                                {
                                    SetSuspendState(atEnd==HIBERNATE, false, false);
                                    break;
                                }
                                i++;
                                Sleep(200);
                            }
                            progress.Close();
                        };
#endif
                        break;
                };
            }
            else
            {
                RunNextInBatch();
            }
        }
        else
        {
            //after all processes have ended on a cancel, we reset the boolean back to false
            //if(stitchFrames.GetCount()==0)
            if(GetRunningCount()==0)
            {
                m_cancelled=false;
            }
        }
    }
}

bool Batch::OnStitch(wxString scriptFile, wxString outname, wxString userDefinedOutput, int id)
{
    // delete the existing wxConfig to force reloading of settings from file/registy
    delete wxConfigBase::Set((wxConfigBase*)NULL);
    wxConfigBase* config = wxConfigBase::Get();
    if(wxIsEmpty(scriptFile))
    {
        wxString defaultdir = config->Read(wxT("/actualPath"),wxT(""));
        wxFileDialog dlg(0,
                         _("Specify project file"),
                         defaultdir, wxT(""),
                         _("Project files (*.pto)|*.pto|All files (*)|*"),
                         wxFD_OPEN, wxDefaultPosition);

        dlg.SetDirectory(wxConfigBase::Get()->Read(wxT("/actualPath"),wxT("")));
        if (dlg.ShowModal() == wxID_OK)
        {
            config->Write(wxT("/actualPath"), dlg.GetDirectory());  // remember for later
            config->Flush();
            wxFileDialog dlg2(0,_("Specify output prefix"),
                              wxConfigBase::Get()->Read(wxT("/actualPath"),wxT("")),
                              wxT(""), wxT(""),
                              wxFD_SAVE, wxDefaultPosition);
            dlg2.SetDirectory(wxConfigBase::Get()->Read(wxT("/actualPath"),wxT("")));
            if (dlg2.ShowModal() == wxID_OK)
            {
                outname = dlg2.GetPath();
            }
            else     // bail
            {
                wxLogError( _("No output prefix specified"));
                return false;
            }
            scriptFile = dlg.GetPath();
        }
        else     // bail
        {
            wxLogError(_("No project files specified"));
            return false;
        }
    }

    // check output filename
    wxFileName outfn(outname);
    wxString ext = outfn.GetExt();
    // remove extension if it indicates an image file
    if (ext.CmpNoCase(wxT("jpg")) == 0 || ext.CmpNoCase(wxT("jpeg")) == 0 ||
            ext.CmpNoCase(wxT("tif")) == 0 || ext.CmpNoCase(wxT("tiff")) == 0 ||
            ext.CmpNoCase(wxT("png")) == 0 || ext.CmpNoCase(wxT("exr")) == 0 ||
            ext.CmpNoCase(wxT("pnm")) == 0 || ext.CmpNoCase(wxT("hdr")) == 0)
    {
        outfn.ClearExt();
        outname = outfn.GetFullPath();
    }

    RunStitchFrame* stitchFrame = new RunStitchFrame(this, wxT("Hugin Stitcher"), wxDefaultPosition, wxSize(640,600));
    stitchFrame->SetProjectId(id);
    if(verbose)
    {
        stitchFrame->Show( true );
        wxTheApp->SetTopWindow( stitchFrame );
    }

    wxFileName basename(scriptFile);
    stitchFrame->SetTitle(wxString::Format(_("%s - Stitching"), basename.GetName().c_str()));
    if(overwrite)
    {
        stitchFrame->m_stitchPanel->SetOverwrite(true);
    }

    bool n = stitchFrame->StitchProject(scriptFile, outname, userDefinedOutput);
    if(n)
    {
        m_stitchFrames.Add(stitchFrame);
    }
    else
    {
        stitchFrame->Close();
    }
    return n;

}

bool Batch::OnDetect(wxString scriptFile, wxString userDefinedAssistant, int id)
{
    // delete the existing wxConfig to force reloading of settings from file/registy
    delete wxConfigBase::Set((wxConfigBase*)NULL);
    RunStitchFrame* stitchFrame = new RunStitchFrame(this, wxT("Hugin Assistant"), wxDefaultPosition, wxSize(640, 600));
    stitchFrame->SetProjectId(id);
    if(verbose)
    {
        stitchFrame->Show( true );
        wxTheApp->SetTopWindow( stitchFrame );
    }

    wxFileName basename(scriptFile);
    stitchFrame->SetTitle(wxString::Format(_("%s - Assistant"), basename.GetName().c_str()));

    bool n = stitchFrame->DetectProject(scriptFile, userDefinedAssistant);
    if(n)
    {
        m_stitchFrames.Add(stitchFrame);
    }
    else
    {
        stitchFrame->Close();
    }
    return n;

}

void Batch::PauseBatch()
{
    if(!m_paused)
    {
        m_paused = true;
        for(int i=0; i<GetRunningCount(); i++)
        {
            m_stitchFrames.Item(i)->m_stitchPanel->PauseStitch();
        }
        for(unsigned int i=0; i<m_projList.GetCount(); i++)
        {
            if(m_projList.Item(i).status==Project::RUNNING)
            {
                m_projList.Item(i).status=Project::PAUSED;
            }
        }
    }
    else
    {
        m_paused = false;
        for(int i=0; i<GetRunningCount(); i++)
        {
            m_stitchFrames.Item(i)->m_stitchPanel->ContinueStitch();
        }
        for(unsigned int i=0; i<m_projList.GetCount(); i++)
        {
            if(m_projList.Item(i).status==Project::PAUSED)
            {
                m_projList.Item(i).status=Project::RUNNING;
            }
        }
    }
}

void Batch::RemoveProject(int id)
{
    if(GetIndex(id) != -1)
    {
        RemoveProjectAtIndex(GetIndex(id));
    }
    else
    {
        wxMessageBox(wxString::Format(_("Error removing, project with id %d is not in list."),id),_("Error!"),wxOK | wxICON_INFORMATION );
    }
}

void Batch::RemoveProjectAtIndex(int selIndex)
{
    //we delete only successful project files and no applications
    if(deleteFiles
            && m_projList.Item(selIndex).id>=0
            && m_projList.Item(selIndex).status==Project::FINISHED)
    {
        wxFileName file(m_projList.Item(selIndex).path);
        if(file.FileExists())
        {
            if(!wxRemoveFile(file.GetFullPath()))
            {
                wxMessageBox(wxString::Format(_("Error: Could not delete project file %s"), file.GetFullPath()),_("Error!"),wxOK | wxICON_INFORMATION );
            }
        }
    }
    m_projList.RemoveAt(selIndex);
    if(m_projList.GetCount()==0) //reset the id generator on empty list
    {
        Project::idGenerator=1;
    }
}

void Batch::RunBatch()
{
    if(!m_running)
    {
        m_failedProjects.clear();
        ((wxFrame*)GetParent())->SetStatusText(_("Running batch..."));
        m_running = true;
#if wxCHECK_VERSION(3,1,0)
        m_resBlocker = new wxPowerResourceBlocker(wxPOWER_RESOURCE_SYSTEM, _("PTBatcherGUI is stitching"));
#endif
        RunNextInBatch();
    }
    else
    {
        ((wxFrame*)GetParent())->SetStatusText(_("Batch already in progress."));
    }
}

void Batch::RunNextInBatch()
{
    bool value;
    bool repeat = true;
    int i;
    while(((i=GetFirstAvailable())!=-1) && repeat)
    {
        //execute command line instructions
        if(m_projList.Item(i).id<0)
        {
            SetStatusText(wxString::Format(_("Running command \"%s\""), m_projList.Item(i).path.c_str()));
            m_projList.Item(i).status=Project::RUNNING;
            //we create a fake stitchFrame, so program waits for app to complete
            if(wxExecute(m_projList.Item(i).path, wxEXEC_SYNC)==0)
            {
                m_projList.Item(i).status=Project::FINISHED;
            }
            else
            {
                m_projList.Item(i).status=Project::FAILED;
            }
        }
        else
        {
            if (m_projList.Item(i).status == Project::MISSING)
            {
                // skip non-existing project files
                continue;
            };
            if (!m_projList.Item(i).userDefindSequence.empty())
            {
                // skip non-existing user defined sequence files
                if (!wxFileName::FileExists(m_projList.Item(i).userDefindSequence))
                {
                    m_projList.Item(i).status = Project::MISSING;
                    m_projList.Item(i).skip = true;
                    continue;
                };
            }
            m_projList.Item(i).status=Project::RUNNING;
            m_running = true;
            if(m_projList.Item(i).target==Project::STITCHING)
            {
                wxCommandEvent e(EVT_INFORMATION,wxID_ANY);
                e.SetString(wxString::Format(_("Now stitching: %s"),m_projList.Item(i).path.c_str()));
                GetParent()->GetEventHandler()->AddPendingEvent(e);
                value = OnStitch(m_projList.Item(i).path, m_projList.Item(i).prefix, m_projList.Item(i).userDefindSequence, m_projList.Item(i).id);
            }
            else
            {
                wxCommandEvent e(EVT_INFORMATION,wxID_ANY);
                e.SetString(wxString::Format(_("Now detecting: %s"),m_projList.Item(i).path.c_str()));
                GetParent()->GetEventHandler()->AddPendingEvent(e);
                value = OnDetect(m_projList.Item(i).path, m_projList.Item(i).userDefindSequence, m_projList.Item(i).id);
            };
            if(!value)
            {
                m_projList.Item(i).status=Project::FAILED;
            }
            else
            {
                repeat = false;
            }
        }
    }
    if(AllDone())
    {
        m_running = false;
    }
}

void Batch::SaveBatchFile(wxString file)
{
    wxFileConfig batchFile;
    //we write current idGenerator to file
    batchFile.Write("/Main/CurrentID", Project::idGenerator);
    batchFile.Write("/Main/Count", m_projList.GetCount());
    //then for each project: project path, prefix, id, status, skip
    for (unsigned int i = 0; i < m_projList.GetCount(); i++)
    {
        batchFile.SetPath(wxString::Format("/Project_%u", i + 1));
        batchFile.Write("Project", m_projList.Item(i).path);
        switch (m_projList.Item(i).target)
        {
            case Project::STITCHING:
                batchFile.Write("Type", "Stitching");
                batchFile.Write("Prefix", m_projList.Item(i).prefix);
                break;
            case Project::DETECTING:
                batchFile.Write("Type", "Detecting");
                break;
        };
        if (!m_projList.Item(i).userDefindSequence.IsEmpty())
        {
            batchFile.Write("UserDefinedSequence", m_projList.Item(i).userDefindSequence);
        };
        batchFile.Write("Id", m_projList.Item(i).id);
        batchFile.Write("Status", (long)m_projList.Item(i).status);
        if (m_projList.Item(i).skip)
        {
            batchFile.Write("Skip", m_projList.Item(i).skip);
        };
    };
    wxFileOutputStream fileStream(file);
    batchFile.Save(fileStream);
    fileStream.Close();
}

wxString Batch::GetBatchFilename()
{
    wxString userDataDir = wxString(hugin_utils::GetUserAppDataDir().c_str(), wxConvLocal);
    if (userDataDir.IsEmpty())
    {
        // could not find user data directory, or this directory could not be create
        // fall back to system user data dir
        userDataDir = wxStandardPaths::Get().GetUserConfigDir();
    }
    return wxFileName(userDataDir, _T("PTBatcherQueue.ptq")).GetFullPath();
}

void Batch::SaveTemp()
{
    SaveBatchFile(GetBatchFilename());
}

void Batch::SetStatus(int index,Project::Status status)
{
    if((unsigned int)index<m_projList.GetCount())
    {
        m_projList.Item(index).status = status;
    }
    else
    {
        wxMessageBox(wxString::Format(_("Error: Could not set status, project with index %d is not in list."),index),_("Error!"),wxOK | wxICON_INFORMATION );
    }
}

void Batch::SwapProject(int index)
{
    Project* proj = m_projList.Detach(index+1);
    m_projList.Insert(proj,index);
}

void Batch::ShowOutput(bool isVisible)
{
    for(unsigned int i=0; i<m_stitchFrames.Count(); i++)
    {
        m_stitchFrames.Item(i)->Show(isVisible);
    };
};

wxString Batch::GetFailedProjectName(unsigned int i)
{
    if(i<m_failedProjects.size())
    {
        return m_failedProjects[i].project;
    }
    else
    {
        return wxEmptyString;
    }
};

wxString Batch::GetFailedProjectLog(unsigned int i)
{
    if(i<m_failedProjects.size())
    {
        return m_failedProjects[i].logfile;
    }
    else
    {
        return wxEmptyString;
    }
};
