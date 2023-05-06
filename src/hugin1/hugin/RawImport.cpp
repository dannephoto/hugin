// -*- c-basic-offset: 4 -*-

/** @file RawImport.cpp
 *
 *  @brief implementation of dialog and functions to import RAW images to project file
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

#include "hugin/RawImport.h"
#include "panoinc.h"
#include "base_wx/platform.h"
#include "base_wx/MyExternalCmdExecDialog.h"
#include "hugin/huginApp.h"
#include "base_wx/CommandHistory.h"
#include "base_wx/wxPanoCommand.h"
#ifdef _WIN32
// workaround for a conflict between exiv2 and wxWidgets/CMake built
#define HAVE_PID_T 1
#endif
#include <exiv2/exiv2.hpp>
#ifdef __WXMSW__
#include <wx/msw/registry.h>
#endif

/** base class for implementation of Raw import functions */
class RawImport
{
public:
    RawImport(wxString ConfigExePath) { m_exe = wxConfig::Get()->Read(ConfigExePath, ""); };
    /** image extension of converted files */
    virtual wxString GetImageExtension() { return wxString("tif"); }
    /** reads additional parameters from dialog into class */
    virtual bool ProcessAdditionalParameters(wxDialog* dlg) { return true; }
    /** return true if program supports overwritting output, otherwise false */
    virtual bool SupportsOverwrite() { return true; }
    /** checks if valid executable was given in dialog 
     *  either absolute path or when relative path is given check PATH */
    bool CheckExe(wxDialog* dlg)
    {
        if (m_exe.IsEmpty())
        {
            m_exe = GetDefaultExe();
        };
        const wxFileName exePath(m_exe);
        if (exePath.IsAbsolute())
        {
            if (exePath.FileExists())
            {
                m_exe = exePath.GetFullPath();
                return true;
            }
            else
            {
                wxMessageBox(wxString::Format(_("Executable \"%s\" not found.\nPlease specify a valid executable in preferences."), exePath.GetFullPath()),
#ifdef _WIN32
                    _("Hugin"),
#else
                    wxT(""),
#endif
                    wxOK | wxICON_INFORMATION, dlg);
                return false;
            };
        };
        wxPathList pathlist;
#ifdef __WXMSW__
        pathlist.Add(huginApp::Get()->GetUtilsBinDir());
#endif
        pathlist.AddEnvList(wxT("PATH"));
        m_exe = pathlist.FindAbsoluteValidPath(exePath.GetFullPath());
        if (m_exe.IsEmpty())
        {
            wxMessageBox(wxString::Format(_("Executable \"%s\" not found in PATH.\nPlease specify a valid executable in preferences."), exePath.GetFullPath()),
#ifdef _WIN32
                _("Hugin"),
#else
                wxT(""),
#endif
                wxOK | wxICON_INFORMATION, dlg);
            return false;
        };
        return true;
    };
    /** return command for processing of reference image */
    virtual HuginQueue::CommandQueue* GetCmdQueueForReference(const wxString& rawFilename, const wxString& imageFilename) = 0;
    /** read output of processing of reference image to read in white balance of reference image */
    virtual bool ProcessReferenceOutput(const wxArrayString& output) = 0;
    /** return commands for processing of all other images with white balance read by RawImport::ProcessReferenceOutput */
    virtual HuginQueue::CommandQueue* GetCmdQueueForImport(const wxArrayString& rawFilenames, const wxArrayString& imageFilenames) = 0;
    /** add additional PanoCommand::PanoCommand to vector if needed 
     *  hint: we need to pass old and new image number because we can't guarantee that the image order
     *  remains unchanged by PanoCommand::wxAddImagesCmd */
    virtual void AddAdditionalPanoramaCommand(std::vector<PanoCommand::PanoCommand*>& cmds, HuginBase::Panorama* pano, const int oldImageCount, const int addedImageCount) {};
protected:
    /** returns the default name of the executable */
    virtual wxString GetDefaultExe() { return wxEmptyString; };
    wxString m_exe;
};

    /** special class for raw import with dcraw */
class DCRawImport :public RawImport
{
public:
    DCRawImport() : RawImport("/RawImportDialog/dcrawExe") {};
    wxString GetImageExtension() override { return wxString("tiff"); }
    bool ProcessAdditionalParameters(wxDialog* dlg) override 
    {
        m_additionalParameters = XRCCTRL(*dlg, "raw_dcraw_parameter", wxTextCtrl)->GetValue().Trim(true).Trim(false);
        return true;
    };
    HuginQueue::CommandQueue* GetCmdQueueForReference(const wxString& rawFilename, const wxString& imageFilename) override
    {
        HuginQueue::CommandQueue* queue = new HuginQueue::CommandQueue();
        wxString args(" -w -v -4 -T ");
        if (!m_additionalParameters.IsEmpty())
        {
            args.Append(m_additionalParameters);
            args.Append(" ");
        };
        args.Append(HuginQueue::wxEscapeFilename(rawFilename));
        queue->push_back(new HuginQueue::NormalCommand(m_exe, args, wxString::Format(_("Executing: %s %s"), m_exe, args)));
        args.Empty();
        args.Append("-overwrite_original -tagsfromfile ");
        args.Append(HuginQueue::wxEscapeFilename(rawFilename));
        args.Append(" -all:all ");
        args.Append(HuginQueue::wxEscapeFilename(imageFilename));
        queue->push_back(new HuginQueue::OptionalCommand(HuginQueue::GetExternalProgram(wxConfig::Get(), huginApp::Get()->GetUtilsBinDir(), "exiftool"), args, wxString::Format(_("Updating EXIF data for %s"), imageFilename)));
        return queue;
    };
    bool ProcessReferenceOutput(const wxArrayString& output) override
    {
        for (auto& s : output)
        {
            int pos = s.Find("multipliers");
            if (pos >= 0)
            {
                m_wb = "-r " + s.Mid(pos + 12);
                return true;
            };
        };
        return false;
    };
    HuginQueue::CommandQueue* GetCmdQueueForImport(const wxArrayString& rawFilenames, const wxArrayString& imageFilenames) override
    {
        HuginQueue::CommandQueue* queue = new HuginQueue::CommandQueue();
        wxConfigBase* config = wxConfig::Get();
        const wxString binDir = huginApp::Get()->GetUtilsBinDir();
        for (size_t i = 0; i < rawFilenames.size(); ++i)
        {
            wxString args(m_wb);
            args.Append(" -v -4 -T ");
            if (!m_additionalParameters.IsEmpty())
            {
                args.Append(m_additionalParameters);
                args.Append(" ");
            };
            args.Append(HuginQueue::wxEscapeFilename(rawFilenames[i]));
            queue->push_back(new HuginQueue::NormalCommand(m_exe, args,wxString::Format(_("Executing: %s %s"), m_exe, args)));
            args.Empty();
            args.Append("-overwrite_original -tagsfromfile ");
            args.Append(HuginQueue::wxEscapeFilename(rawFilenames[i]));
            args.Append(" -all:all ");
            args.Append(HuginQueue::wxEscapeFilename(imageFilenames[i]));
            queue->push_back(new HuginQueue::OptionalCommand(HuginQueue::GetExternalProgram(config, binDir, "exiftool"), args, wxString::Format(_("Updating EXIF data for %s"), imageFilenames[i])));
        }
        return queue;
    };
    void AddAdditionalPanoramaCommand(std::vector<PanoCommand::PanoCommand*>& cmds, HuginBase::Panorama* pano, const int oldImageCount, const int addedImageCount) override
    {
        // set response to linear for newly added images
        HuginBase::UIntSet imgs;
        fill_set(imgs, oldImageCount, oldImageCount + addedImageCount - 1);
        cmds.push_back(new PanoCommand::ChangeImageResponseTypeCmd(*pano, imgs, HuginBase::SrcPanoImage::RESPONSE_LINEAR));
    };
protected:
    virtual wxString GetDefaultExe()
    {
#ifdef _WIN32
        return "dcraw.exe";
#else
        return "dcraw";
#endif
    };
private:
    wxString m_additionalParameters;
    wxString m_wb;
};

/** class for RawTherapee raw import */
class RTRawImport :public RawImport
{
public:
    RTRawImport() : RawImport("/RawImportDialog/RTExe") {};
    bool ProcessAdditionalParameters(wxDialog* dlg) override
    {
        m_processingProfile = XRCCTRL(*dlg, "raw_rt_processing_profile", wxTextCtrl)->GetValue().Trim(true).Trim(false);
        if (!m_processingProfile.IsEmpty() && !wxFileName::FileExists(m_processingProfile))
        {
            wxMessageBox(wxString::Format(_("Processing profile \"%s\" not found.\nPlease specify a valid file or leave field empty for default settings."), m_processingProfile),
#ifdef _WIN32
                _("Hugin"),
#else
                wxT(""),
#endif
                wxOK | wxICON_INFORMATION, dlg);
            return false;
        }
        return true;
    };
    HuginQueue::CommandQueue* GetCmdQueueForReference(const wxString& rawFilename, const wxString& imageFilename) override
    {
        HuginQueue::CommandQueue* queue = new HuginQueue::CommandQueue();
        wxString args("-O " + HuginQueue::wxEscapeFilename(imageFilename));
        if (m_processingProfile.IsEmpty())
        {
            args.Append(" -d");
        }
        else
        {
            args.Append(" -p " + HuginQueue::wxEscapeFilename(m_processingProfile));
        };
        // apply some special settings, especially disable all crop and rotation settings
        args.Append(" -s -p " + HuginQueue::wxEscapeFilename(
            wxString(std::string(hugin_utils::GetDataDir() + "hugin_rt.pp3").c_str(), HUGIN_CONV_FILENAME)));
        args.Append(" -b16 -tz -Y -c ");
        args.Append(HuginQueue::wxEscapeFilename(rawFilename));
        m_usedProcessingProfile = imageFilename + ".pp3";
        queue->push_back(new HuginQueue::NormalCommand(m_exe, args, wxString::Format(_("Executing: %s %s"), m_exe, args)));
        return queue;
    };
    bool ProcessReferenceOutput(const wxArrayString& output) override
    {
        // we need to change the WB setting in the processing profile so the white balance of the reference image
        // is used and not the stored WB from each individual image
        wxFileConfig config(wxEmptyString, wxEmptyString, m_usedProcessingProfile);
        config.Write("/White Balance/Setting", "Custom");
        config.Flush();
        return true;
    };
    HuginQueue::CommandQueue* GetCmdQueueForImport(const wxArrayString& rawFilenames, const wxArrayString& imageFilenames) override
    {
        HuginQueue::CommandQueue* queue = new HuginQueue::CommandQueue();
        for (size_t i = 0; i < rawFilenames.size(); ++i)
        {
            wxString args("-o " + HuginQueue::wxEscapeFilename(imageFilenames[i]));
            args.Append(" -p "+HuginQueue::wxEscapeFilename(m_usedProcessingProfile));
            args.Append(" -b16 -tz -Y -c ");
            args.Append(HuginQueue::wxEscapeFilename(rawFilenames[i]));
            queue->push_back(new HuginQueue::NormalCommand(m_exe, args, wxString::Format(_("Executing: %s %s"), m_exe, args)));
        }
        return queue;
    };
protected:
    virtual wxString GetDefaultExe()
    {
#ifdef __WXMSW__
        // try reading installed version from registry
        // works only with RT 5.5 and above
        wxRegKey regkey(wxRegKey::HKLM, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\App Paths\\rawtherapee-cli.exe");
        wxString prog;
        if (regkey.HasValue(wxT("")) && regkey.QueryRawValue(wxT(""), prog))
        {
            if (wxFileName::FileExists(prog))
            {
                return prog;
            };
        }
        else
        {
            // now check if installed for current user only
            wxRegKey regkeyUser(wxRegKey::HKCU, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\App Paths\\rawtherapee-cli.exe");
            wxString prog;
            if (regkeyUser.HasValue(wxT("")) && regkeyUser.QueryRawValue(wxT(""), prog))
            {
                if (wxFileName::FileExists(prog))
                {
                    return prog;
                };
            };
        };
        // nothing found in registry, return default name
        return "rawtherapee-cli.exe";
#elif defined __WXMAC__
        return "/Applications/RawTherapee.app/Contents/MacOS/rawtherapee-cli";
#else
        return "rawtherapee-cli";
#endif
    };
private:
    wxString m_processingProfile;
    wxString m_usedProcessingProfile;
};

/** special class for Darktable raw import */
class DarkTableRawImport : public RawImport
{
public:
    DarkTableRawImport() : RawImport("/RawImportDialog/DarktableExe") {}
    ~DarkTableRawImport()
    {
        // destructor, clean up generated files;
        if (!m_usedSettings.IsEmpty())
        {
            wxRemoveFile(m_usedSettings);
        };
    }
    bool SupportsOverwrite() override { return false; }
    HuginQueue::CommandQueue* GetCmdQueueForReference(const wxString& rawFilename, const wxString& imageFilename) override
    {
        HuginQueue::CommandQueue* queue = new HuginQueue::CommandQueue();
        wxString args(" --verbose ");
        args.Append(HuginQueue::wxEscapeFilename(rawFilename));
        args.Append(" ");
#ifdef __WXMSW__
        // darktable has problems with Windows paths as output
        // so change separator before
        wxString file(imageFilename);
        file.Replace("\\", "/", true);
        args.Append(HuginQueue::wxEscapeFilename(file));
#else
        args.Append(HuginQueue::wxEscapeFilename(imageFilename));
#endif
        // switch --bpp does not work, so using this strange workaround
        args.Append(" --core --conf plugins/imageio/format/tiff/bpp=16");
        m_refImage = imageFilename;
        queue->push_back(new HuginQueue::NormalCommand(m_exe, args, wxString::Format(_("Executing: %s %s"), m_exe, args)));
        return queue;
    };
    bool ProcessReferenceOutput(const wxArrayString& output) override
    {
        // extract XMP package for converted reference image and store in temp
#if defined EXIV2_VERSION && EXIV2_TEST_VERSION(0,27,99)
        Exiv2::Image::UniquePtr image;
#else
        Exiv2::Image::AutoPtr image;
#endif
        try
        {
            image = Exiv2::ImageFactory::open(std::string(m_refImage.mb_str(HUGIN_CONV_FILENAME)));
        }
        catch (const Exiv2::Error& e)
        {
            std::cerr << "Exiv2: Error reading metadata (" << e.what() << ")" << std::endl;
            return false;
        }
        try
        {
            image->readMetadata();
        }
        catch (const Exiv2::Error& e)
        {
            std::cerr << "Caught Exiv2 exception " << e.what() << std::endl;
            return false;
        };
        m_usedSettings = wxFileName::CreateTempFileName("hugdt");
#if defined EXIV2_VERSION && EXIV2_TEST_VERSION(0,27,99)
        Exiv2::Image::UniquePtr image2;
#else
        Exiv2::Image::AutoPtr image2;
#endif
        try
        {
            image2 = Exiv2::ImageFactory::create(Exiv2::ImageType::xmp, std::string(m_usedSettings.mb_str(HUGIN_CONV_FILENAME)));
        }
        catch (const Exiv2::Error& e)
        {
            std::cerr << "Exiv2: Error creating temp xmp file (" << e.what() << ")" << std::endl;
            return false;
        }
        image2->setXmpData(image->xmpData());
        image2->writeMetadata();
        return true;
    };
    HuginQueue::CommandQueue* GetCmdQueueForImport(const wxArrayString& rawFilenames, const wxArrayString& imageFilenames) override
    {
        HuginQueue::CommandQueue* queue = new HuginQueue::CommandQueue();
        for (size_t i = 0; i < rawFilenames.size(); ++i)
        {
            wxString args("--verbose ");
            args.Append(HuginQueue::wxEscapeFilename(rawFilenames[i]));
            args.Append(" ");
            args.Append(HuginQueue::wxEscapeFilename(m_usedSettings));
            args.Append(" ");
#ifdef __WXMSW__
            // darktable has problems with Windows paths as output
            // so change separator before
            wxString file(imageFilenames[i]);
            file.Replace("\\", "/", true);
            args.Append(HuginQueue::wxEscapeFilename(file));
#else
            args.Append(HuginQueue::wxEscapeFilename(imageFilenames[i]));
#endif
            // switch --bpp does not work, so using this strange workaround
            args.Append(" --core --conf plugins/imageio/format/tiff/bpp=16");
            queue->push_back(new HuginQueue::NormalCommand(m_exe, args, wxString::Format(_("Executing: %s %s"), m_exe, args)));
        }
        return queue;
    };
protected:
    virtual wxString GetDefaultExe()
    {
#ifdef __WXMSW__
        // try reading installed version from registry
        wxRegKey regkey(wxRegKey::HKLM, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\App Paths\\darktable-cli.exe");
        wxString prog;
        if (regkey.HasValue(wxT("")) && regkey.QueryRawValue(wxT(""), prog))
        {
            if (wxFileName::FileExists(prog))
            {
                return prog;
            }
        }
        else
        {
            // now check if installed for current user only
            wxRegKey regkeyUser(wxRegKey::HKCU, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\App Paths\\darktable-cli.exe");
            wxString prog;
            if (regkeyUser.HasValue(wxT("")) && regkeyUser.QueryRawValue(wxT(""), prog))
            {
                if (wxFileName::FileExists(prog))
                {
                    return prog;
                };
            };
        };
        // nothing found in registry, return default name
        return "darktable-cli.exe";
#elif defined __WXMAC__
        return "/Applications/darktable.app/Contents/MacOS/darktable-cli";
#else
        return "darktable-cli";
#endif
    };
private:
    wxString m_refImage;
    wxString m_usedSettings;
};


/** dialog class for showing progress of raw import */
class RawImportProgress :public wxDialog
{
public:
    RawImportProgress(wxWindow * parent, std::shared_ptr<RawImport>& converter, const wxArrayString& rawImages, const wxArrayString& images, const int refImg) : 
        wxDialog(parent, wxID_ANY, _("Import Raw Images"), wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
        m_converter(converter), m_rawImages(rawImages), m_images(images), m_refImg(refImg), m_isRunning(false)
    {
        DEBUG_ASSERT(m_rawImages.size() > 0);
        DEBUG_ASSERT(m_rawImages.size() == m_images.size());
        if (m_refImg < 0)
        {
            m_refImg = 0;
        };
        if (m_refImg >= m_rawImages.size())
        {
            m_refImg = 0;
        };
        wxBoxSizer * topsizer = new wxBoxSizer(wxVERTICAL);
        m_progressPanel = new MyExecPanel(this);
        topsizer->Add(m_progressPanel, 1, wxEXPAND | wxALL, 2);

        wxBoxSizer* bottomsizer = new wxBoxSizer(wxHORIZONTAL);
#if wxCHECK_VERSION(3,1,0)
        m_progress = new wxGauge(this, wxID_ANY, 100, wxDefaultPosition, wxDefaultSize, wxGA_HORIZONTAL | wxGA_PROGRESS);
#else
        m_progress = new wxGauge(this, wxID_ANY, 100, wxDefaultPosition, wxDefaultSize, wxGA_HORIZONTAL);
#endif
        bottomsizer->Add(m_progress, 1, wxEXPAND | wxALL, 10);
        m_cancelButton = new wxButton(this, wxID_CANCEL, _("Cancel"));
        bottomsizer->Add(m_cancelButton, 0, wxALL | wxALIGN_RIGHT, 10);
        m_cancelButton->Bind(wxEVT_BUTTON, &RawImportProgress::OnCancel, this);
        topsizer->Add(bottomsizer, 0, wxEXPAND);
#ifdef __WXMSW__
        // wxFrame does have a strange background color on Windows..
        this->SetBackgroundColour(m_progressPanel->GetBackgroundColour());
#endif
        SetSizer(topsizer);
        RestoreFramePosition(this, "RawImportProgress");
        Bind(EVT_QUEUE_PROGRESS, &RawImportProgress::OnProgress, this);
        Bind(wxEVT_INIT_DIALOG, &RawImportProgress::OnInitDialog, this);
    };
    virtual ~RawImportProgress()
    {
        StoreFramePosition(this, "RawImportProgress");
    }

protected:
    void OnProcessReferenceTerminate(wxProcessEvent& e) 
    {
        if (e.GetExitCode() == 0)
        {
            if (!m_converter->ProcessReferenceOutput(m_progressPanel->GetLogAsArrayString()))
            {
                wxMessageBox(_("Could not process of output of reference image.\nCan't process further images."),
#ifdef _WIN32
                    _("Hugin"),
#else
                    wxT(""),
#endif
                    wxOK | wxICON_INFORMATION, this);

            };
            Unbind(wxEVT_END_PROCESS, &RawImportProgress::OnProcessReferenceTerminate, this);
            m_progress->SetValue(hugin_utils::roundi(1.0 / m_images.size()));
            wxArrayString rawFiles(m_rawImages);
            rawFiles.RemoveAt(m_refImg);
            if (rawFiles.IsEmpty())
            {
                EndModal(wxID_OK);
            };
            wxArrayString imgFiles(m_images);
            imgFiles.RemoveAt(m_refImg);
            Bind(wxEVT_END_PROCESS, &RawImportProgress::OnProcessTerminate, this);
            m_progressPanel->ExecQueue(m_converter->GetCmdQueueForImport(rawFiles, imgFiles));
        }
        else
        {
            m_isRunning = false;
            m_cancelButton->SetLabel(_("Close"));
        };
    };
    void OnProcessTerminate(wxProcessEvent& e)
    {
        if (e.GetExitCode() == 0)
        {
            EndModal(wxID_OK);
        }
        else
        {
            m_isRunning = false;
            m_cancelButton->SetLabel(_("Close"));
        };
    };
    void OnProgress(wxCommandEvent& event)
    {
        if (event.GetInt() >= 0)
        {
            m_progress->SetValue(hugin_utils::roundi((1 + event.GetInt() / 100.0f * (m_images.size() - 2)) * 100.0f / m_images.size()));
        };
    };
    void OnCancel(wxCommandEvent & event)
    {
        if (m_isRunning)
        {
            m_progressPanel->KillProcess();
            m_isRunning = false;
        }
        else
        {
            EndModal(wxID_CANCEL);
        };
    };
    void OnInitDialog(wxInitDialogEvent& e)
    {
        // start processing reference image
        Bind(wxEVT_END_PROCESS, &RawImportProgress::OnProcessReferenceTerminate, this);
        m_progressPanel->ExecQueue(m_converter->GetCmdQueueForReference(m_rawImages[m_refImg], m_images[m_refImg]));
    }

private:
    std::shared_ptr<RawImport> m_converter;
    wxArrayString m_rawImages, m_images;
    int m_refImg;
    bool m_isRunning;
    wxGauge* m_progress;
    wxButton* m_cancelButton;
    MyExecPanel * m_progressPanel;
};

BEGIN_EVENT_TABLE(RawImportDialog, wxDialog)
    EVT_BUTTON(XRCID("raw_rt_processing_profile_select"), RawImportDialog::OnSelectRTProcessingProfile)
    EVT_BUTTON(wxID_OK, RawImportDialog::OnOk)
    EVT_RADIOBUTTON(XRCID("raw_rb_dcraw"), RawImportDialog::OnRawConverterSelected)
    EVT_RADIOBUTTON(XRCID("raw_rb_rt"), RawImportDialog::OnRawConverterSelected)
    EVT_RADIOBUTTON(XRCID("raw_rb_darktable"), RawImportDialog::OnRawConverterSelected)
END_EVENT_TABLE()

RawImportDialog::RawImportDialog(wxWindow *parent, HuginBase::Panorama* pano, std::vector<std::string>& rawFiles)
{
    // load our children. some children might need special
    // initialization. this will be done later.
    wxXmlResource::Get()->LoadDialog(this, parent, wxT("import_raw_dialog"));

#ifdef __WXMSW__
    wxIconBundle myIcons(huginApp::Get()->GetXRCPath() + wxT("data/hugin.ico"),wxBITMAP_TYPE_ICO);
    SetIcons(myIcons);
#else
    wxIcon myIcon(huginApp::Get()->GetXRCPath() + wxT("data/hugin.png"),wxBITMAP_TYPE_PNG);
    SetIcon(myIcon);
#endif

    RestoreFramePosition(this, "RawImportDialog");
    wxConfigBase* config = wxConfig::Get();
    // dcraw
    wxString s = config->Read("/RawImportDialog/dcrawParameter", "");
    XRCCTRL(*this, "raw_dcraw_parameter", wxTextCtrl)->SetValue(s);

    // RT processing profile
    s = config->Read("/RawImportDialog/RTProcessingProfile", "");
    wxTextCtrl* ctrl = XRCCTRL(*this, "raw_rt_processing_profile", wxTextCtrl);
    ctrl->SetValue(s);
    ctrl->AutoCompleteFileNames();
    // read last used converter
    const long converter = config->Read("/RawImportDialog/Converter", 0l);
    switch (converter)
    {
        default:
        case 0: 
            XRCCTRL(*this, "raw_rb_dcraw", wxRadioButton)->SetValue(true);
            break;
        case 1:
            XRCCTRL(*this, "raw_rb_rt", wxRadioButton)->SetValue(true);
            break;
        case 2:
            XRCCTRL(*this, "raw_rb_darktable", wxRadioButton)->SetValue(true);
            break;
    };
    wxCommandEvent dummy;
    OnRawConverterSelected(dummy);

    m_pano=pano;
    for (auto& file : rawFiles)
    {
        m_rawImages.Add(wxString(file.c_str(), HUGIN_CONV_FILENAME));
    };
};

RawImportDialog::~RawImportDialog()
{
    StoreFramePosition(this, "RawImportDialog");
}

PanoCommand::PanoCommand * RawImportDialog::GetPanoCommand()
{
    return m_cmd;
};

bool RawImportDialog::CheckRawFiles()
{
    wxArrayString errorReadingFile;
    wxArrayString differentCam;
    std::string camera;
    for (auto& file : m_rawImages)
    {
        // check that all images are from the same camera
#if defined EXIV2_VERSION && EXIV2_TEST_VERSION(0,27,99)
        Exiv2::Image::UniquePtr image;
#else
        Exiv2::Image::AutoPtr image;
#endif
        try
        {
            image = Exiv2::ImageFactory::open(std::string(file.mb_str(HUGIN_CONV_FILENAME)));
        }
        catch (const Exiv2::Error& e)
        {
            std::cerr << "Exiv2: Error reading metadata (" << e.what() << ")" << std::endl;
            errorReadingFile.push_back(file);
            continue;
        }

        try
        {
            image->readMetadata();
        }
        catch (const Exiv2::Error& e)
        {
            std::cerr << "Caught Exiv2 exception '" << e.what() << "' for file " << file << std::endl;
            errorReadingFile.push_back(file);
        };
        Exiv2::ExifData &exifData = image->exifData();
        if (exifData.empty())
        {
            errorReadingFile.push_back(file);
            continue;
        };
        std::string cam;
        auto make = Exiv2::make(exifData);
        if (make != exifData.end() && make->count())
        {
            cam = make->toString();
        };
        auto model = Exiv2::model(exifData);
        if (model != exifData.end() && model->count())
        {
            cam += "|" + model->toString();
        };
        if (camera.empty())
        {
            camera = cam;
        }
        else
        {
            if (cam != camera)
            {
                differentCam.push_back(file);
                continue;
            };
        };
    };
    if (!errorReadingFile.IsEmpty())
    {
        wxDialog dlg;
        wxXmlResource::Get()->LoadDialog(&dlg, this, wxT("dlg_warning_filename"));
        dlg.SetLabel(_("Warning: Read error"));
        XRCCTRL(dlg, "dlg_warning_text", wxStaticText)->SetLabel(_("The following files will be skipped because the metadata of these files could not read."));
        XRCCTRL(dlg, "dlg_warning_list", wxListBox)->Append(errorReadingFile);
        dlg.Fit();
        dlg.CenterOnScreen();
        dlg.ShowModal();
        for (auto& file : errorReadingFile)
        {
            m_rawImages.Remove(file);
        };
        if (m_rawImages.IsEmpty())
        {
            return false;
        };
    };
    if (differentCam.IsEmpty())
    {
        FillImageChoice();
        return true;
    }
    else
    {
        wxDialog dlg;
        wxXmlResource::Get()->LoadDialog(&dlg, this, wxT("dlg_warning_filename"));
        dlg.SetLabel(_("Warning: raw images from different cameras"));
        XRCCTRL(dlg, "dlg_warning_text", wxStaticText)->SetLabel(_("The following images were shot with different camera than the other one.\nThe raw import works only for images from the same cam."));
        XRCCTRL(dlg, "dlg_warning_list", wxListBox)->Append(differentCam);
        dlg.Fit();
        dlg.CenterOnScreen();
        dlg.ShowModal();
        return false;
    };
};

void RawImportDialog::OnOk(wxCommandEvent & e)
{
    std::shared_ptr<RawImport> rawConverter;
    long rawConverterInt;
    if (XRCCTRL(*this, "raw_rb_dcraw", wxRadioButton)->GetValue())
    {
        rawConverter = std::make_shared<DCRawImport>();
        rawConverterInt = 0;
    }
    else
    {
        if (XRCCTRL(*this, "raw_rb_rt", wxRadioButton)->GetValue())
        {
            rawConverter = std::make_shared<RTRawImport>();
            rawConverterInt = 1;
        }
        else
        {
            rawConverter = std::make_shared<DarkTableRawImport>();
            rawConverterInt = 2;
        };
    };
    // check if given program is available
    if (!rawConverter->CheckExe(this))
    {
        return;
    }
    // check additional parameters
    if (!rawConverter->ProcessAdditionalParameters(this))
    {
        return;
    };
    {
        // check if image files already exists
        m_images.clear();
        wxArrayString existingImages;
        for (auto& img : m_rawImages)
        {
            wxFileName newImage(img);
            newImage.SetExt(rawConverter->GetImageExtension());
            m_images.push_back(newImage.GetFullPath());
            if (newImage.FileExists())
            {
                existingImages.push_back(newImage.GetFullPath());
            };
        };
        if (!existingImages.IsEmpty())
        {
            wxDialog dlg;
            wxXmlResource::Get()->LoadDialog(&dlg, this, wxT("dlg_warning_overwrite"));
            XRCCTRL(dlg, "dlg_overwrite_list", wxListBox)->Append(existingImages);
            dlg.Fit();
            dlg.CenterOnScreen();
            if (dlg.ShowModal() != wxID_OK)
            {
                return;
            };
        };
        if (!rawConverter->SupportsOverwrite())
        {
            // raw converter does not support overwritting
            // so delete the file explicit before
            for (auto& img : existingImages)
            {
                wxRemoveFile(img);
            };
        };
    };
    RawImportProgress dlg(this, rawConverter, m_rawImages, m_images, XRCCTRL(*this, "raw_choice_wb", wxChoice)->GetSelection());
    if (dlg.ShowModal() == wxID_OK)
    {
        // save settings for next call
        wxConfigBase* config = wxConfig::Get();
        config->Write("/RawImportDialog/Converter", rawConverterInt);
        config->Write("/RawImportDialog/dcrawParameter", XRCCTRL(*this, "raw_dcraw_parameter", wxTextCtrl)->GetValue().Trim(true).Trim(false));
        config->Write("/RawImportDialog/RTProcessingProfile", XRCCTRL(*this, "raw_rt_processing_profile", wxTextCtrl)->GetValue());
        config->Flush();
        // check if all files were generated
        std::vector<std::string> files;
        bool missingFiles = false;
        for (auto& img:m_images)
        {
            if (wxFileName::FileExists(img))
            {
                files.push_back(std::string(img.mb_str(HUGIN_CONV_FILENAME)));
            }
            else
            {
                missingFiles = true;
            };
        };
        if (missingFiles || files.empty())
        {
            wxMessageBox(_("At least one raw images was not successfully converted.\nThis image(s) will be skipped"),
#ifdef _WIN32
                _("Hugin"),
#else
                wxT(""),
#endif
                wxOK | wxICON_INFORMATION, this);
        };
        if (files.empty())
        {
            return;
        };
        // now build PanoCommand and store it
        std::vector<PanoCommand::PanoCommand*> cmds;
        cmds.push_back(new PanoCommand::wxAddImagesCmd(*m_pano, files));
        rawConverter->AddAdditionalPanoramaCommand(cmds, m_pano, m_pano->getNrOfImages(), files.size());
        m_cmd = new PanoCommand::CombinedPanoCommand(*m_pano, cmds);
        m_cmd->setName("import raw images");
        // close dialog
        EndModal(wxID_OK);
        return;
    };
    EndModal(wxID_CANCEL);
}

void RawImportDialog::OnSelectRTProcessingProfile(wxCommandEvent& e)
{
    wxTextCtrl* input = XRCCTRL(*this, "raw_rt_processing_profile", wxTextCtrl);
    wxFileDialog dlg(this, _("Select default RT processing profile"), "", input->GetValue(), _("RT processing profile|*.pp3"), wxFD_OPEN, wxDefaultPosition);
    if (dlg.ShowModal() == wxID_OK)
    {
        input->SetValue(dlg.GetPath());
    }
}

void RawImportDialog::OnRawConverterSelected(wxCommandEvent & e)
{
    enum Converter {
        DCRAW,
        RAWTHERAPEE,
        DARKTABLE
    } rawConverter;
    if (XRCCTRL(*this, "raw_rb_dcraw", wxRadioButton)->GetValue())
    {
        rawConverter = Converter::DCRAW;
    }
    else
    {
        if (XRCCTRL(*this, "raw_rb_rt", wxRadioButton)->GetValue())
        {
            rawConverter = Converter::RAWTHERAPEE;
        }
        else
        {
            rawConverter = Converter::DARKTABLE;
        };
    };
    XRCCTRL(*this, "raw_dcraw_text", wxStaticText)->Enable(rawConverter == Converter::DCRAW);
    XRCCTRL(*this, "raw_dcraw_parameter", wxTextCtrl)->Enable(rawConverter == Converter::DCRAW);
    XRCCTRL(*this, "raw_rt_text", wxStaticText)->Enable(rawConverter == Converter::RAWTHERAPEE);
    XRCCTRL(*this, "raw_rt_processing_profile", wxTextCtrl)->Enable(rawConverter == Converter::RAWTHERAPEE);
    XRCCTRL(*this, "raw_rt_processing_profile_select", wxButton)->Enable(rawConverter == Converter::RAWTHERAPEE);
};

void RawImportDialog::FillImageChoice()
{
    wxChoice* choice = XRCCTRL(*this, "raw_choice_wb", wxChoice);
    for (int i=0; i<m_rawImages.size();++i)
    {
        const wxFileName file(m_rawImages[i]);
        choice->Append(file.GetFullName());
    };
    choice->SetSelection(0);
};

