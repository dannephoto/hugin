// -*- c-basic-offset: 4 -*-

/** @file hugin_utils/utils.cpp
 *
 *  @author Pablo d'Angelo <pablo.dangelo@web.de>
 *
 *  $Id$
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

#include "utils.h"
#include "stl_utils.h"
#include "hugin_version.h"
#include "hugin_config.h"

#ifdef _WIN32
    #define NOMINMAX
    #include <sys/utime.h>
    #include <shlobj.h>
#else
    #include <sys/time.h>
    #include <cstdlib>
    #include <unistd.h>
    #include <sys/types.h>
    #include <pwd.h>
#endif
#include <time.h>
#include <fstream>
#include <stdio.h>
#include <cstdio>
#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#else
#include <hugin_config.h>
#endif
#include <algorithm>
#include <hugin_utils/filesystem.h>
#include <lcms2.h>

#ifdef __APPLE__
#include <mach-o/dyld.h>  /* _NSGetExecutablePath */
#include <limits.h>       /* PATH_MAX */
#include <libgen.h>       /* dirname */
#endif

#if defined HAVE_EPOXY && HAVE_EPOXY
#include <epoxy/gl.h>
#ifdef _WIN32
#include <epoxy/wgl.h>
#endif
#else
#include <GL/glew.h>
#ifdef _WIN32
#include <GL/wglew.h>
#endif
#endif
#if defined __APPLE__
  #include <GLUT/glut.h>
#endif

namespace hugin_utils {
    
#ifdef UNIX_LIKE
std::string GetCurrentTimeString()
{
  char tmp[100];
  struct tm t;
  struct timeval tv;
  gettimeofday(&tv,NULL);
  localtime_r((time_t*)&tv.tv_sec, &t); // is the casting safe?
  strftime(tmp,99,"%H:%M:%S",&t);
  sprintf(tmp+8,".%06ld", (long)tv.tv_usec);
  return tmp;
}
#else
std::string GetCurrentTimeString()
{
    // FIXME implement for Win
    return "";
}
#endif


std::string getExtension(const std::string & basename2)
{
	std::string::size_type idx = basename2.rfind('.');
    // check if the dot is not followed by a \ or a /
    // to avoid cutting pathes.
    if (idx == std::string::npos) {
        // no dot found
		return std::string("");
    }
#ifdef UNIX_LIKE
    // check for slashes after dot
    std::string::size_type slashidx = basename2.find('/', idx);
    if ( slashidx == std::string::npos)
    {
        return basename2.substr(idx+1);
    } else {
        return std::string("");
    }
#else
    // check for slashes after dot
    std::string::size_type slashidx = basename2.find('/', idx);
    std::string::size_type backslashidx = basename2.find('\\', idx);
    if ( slashidx == std::string::npos &&  backslashidx == std::string::npos)
    {
        return basename2.substr(idx+1);
    } else {
		return std::string("");
    }
#endif
}

std::string stripExtension(const std::string & basename2)
{
    std::string::size_type idx = basename2.rfind('.');
    // check if the dot is not followed by a \ or a /
    // to avoid cutting pathes.
    if (idx == std::string::npos) {
        // no dot found
        return basename2;
    }
#ifdef UNIX_LIKE
    std::string::size_type slashidx = basename2.find('/', idx);
    if ( slashidx == std::string::npos)
    {
        return basename2.substr(0, idx);
    } else {
        return basename2;
    }
#else
    // check for slashes after dot
    std::string::size_type slashidx = basename2.find('/', idx);
    std::string::size_type backslashidx = basename2.find('\\', idx);
    if ( slashidx == std::string::npos &&  backslashidx == std::string::npos)
    {
        return basename2.substr(0, idx);
    } else {
        return basename2;
    }
#endif
}

std::string stripPath(const std::string & filename)
{
#ifdef UNIX_LIKE
    std::string::size_type idx = filename.rfind('/');
#else
    std::string::size_type idx1 = filename.rfind('\\');
    std::string::size_type idx2 = filename.rfind('/');
    std::string::size_type idx;
    if (idx1 == std::string::npos) {
        idx = idx2;
    } else if (idx2 == std::string::npos) {
        idx = idx1;
    } else {
        idx = std::max(idx1, idx2);
    }
#endif
    if (idx != std::string::npos) {
//        DEBUG_DEBUG("returning substring: " << filename.substr(idx + 1));
        return filename.substr(idx + 1);
    } else {
        return filename;
    }
}

std::string getPathPrefix(const std::string & filename)
{
#ifdef UNIX_LIKE
    std::string::size_type idx = filename.rfind('/');
#else
    std::string::size_type idx1 = filename.rfind('\\');
    std::string::size_type idx2 = filename.rfind('/');
    std::string::size_type idx;
    if (idx1 == std::string::npos) {
        idx = idx2;
    } else if (idx2 == std::string::npos) {
        idx = idx1;
    } else {
        idx = std::max(idx1, idx2);
    }
#endif
    if (idx != std::string::npos) {
//        DEBUG_DEBUG("returning substring: " << filename.substr(idx + 1));
        return filename.substr(0, idx+1);
    } else {
        return "";
    }
}

std::string StrTrim(const std::string& str)
{
    std::string s(str);
    std::string::size_type pos = s.find_last_not_of(" \t");
    if (pos != std::string::npos)
    {
        s.erase(pos + 1);
        pos = s.find_first_not_of(" \t");
        if (pos != std::string::npos)
        {
            s.erase(0, pos);
        };
    }
    else
    {
        s.erase(s.begin(), s.end());
    };
    return s;
}

std::string doubleToString(double d, int digits)
{
    char fmt[10];
    if (digits < 0) {
        strcpy(fmt,"%f");
    } else {
        // max. 16 digits to prevent overflow
        std::sprintf(fmt, "%%.%df", std::min(digits, 16));
    }
    char c[1024];
    c[1023] = 0;
#ifdef _MSC_VER
    _snprintf (c, 1023, fmt, d);
#else
    snprintf (c, 1023, fmt, d);
#endif

    std::string number (c);

    int l = (int)number.length()-1;

    while ( l != 0 && number[l] == '0' ) {
      number.erase (l);
      l--;
    }
    if ( number[l] == ',' ) {
      number.erase (l);
      l--;
    }
    if ( number[l] == '.' ) {
      number.erase (l);
      l--;
    }
    return number;
}

bool stringToInt(const std::string& s, int& val)
{
    if (StrTrim(s) == "0")
    {
        val = 0;
        return true;
    };
    int x = atoi(s.c_str());
    if (x != 0)
    {
        val = x;
        return true;
    };
    return false;
};

bool stringToUInt(const std::string&s, unsigned int& val)
{
    int x;
    if (stringToInt(s, x))
    {
        if (x >= 0)
        {
            val = static_cast<unsigned int>(x);
            return true;
        };
    };
    return false;
};

std::vector<std::string> SplitString(const std::string& s, const std::string& sep)
{
    std::vector<std::string> result;
    std::size_t pos = s.find_first_of(sep, 0);
    std::size_t pos2 = 0;
    while (pos != std::string::npos)
    {
        if (pos - pos2 > 0)
        {
            std::string t(s.substr(pos2, pos - pos2));
            t=StrTrim(t);
            if (!t.empty())
            {
                result.push_back(t);
            };
        };
        pos2 = pos + 1;
        pos = s.find_first_of(sep, pos2);
    }
    if (pos2 < s.length())
    {
        std::string t(s.substr(pos2));
        t = StrTrim(t);
        if (!t.empty())
        {
            result.push_back(t);
        };
    };
    return result;
};

void ReplaceAll(std::string& s, const std::string& oldChar, char newChar)
{
    std::size_t found = s.find_first_of(oldChar);
    while (found != std::string::npos)
    {
        s[found] = newChar;
        found = s.find_first_of(oldChar, found + 1);
    };
};

bool StringContainsCaseInsensitive(const std::string& s1, const std::string& s2)
{
    const auto it = std::search(s1.begin(), s1.end(), s2.begin(), s2.end(), [](const char a, const char b)->bool { return std::tolower(a) == std::tolower(b); });
    return it != s1.end();
}

    void ControlPointErrorColour(const double cperr, 
        double &r,double &g, double &b)
    {
        //Colour change points
#define XP1 5.0f
#define XP2 10.0f

        if ( cperr<= XP1) 
        {
            //low error
            r = cperr / XP1;
            g = 0.75;
        }
        else
        {
            r = 1.0;
            g = 0.75 * ((1.0 - std::min<double>(cperr - XP1, XP2 - XP1) / (XP2 - XP1)));
        } 
        b = 0.0;
    }

bool FileExists(const std::string& filename)
{
    std::ifstream ifile(filename.c_str());
    return !ifile.fail();
}

std::string GetAbsoluteFilename(const std::string& filename)
{
#ifdef _WIN32
    char fullpath[_MAX_PATH];
    _fullpath(fullpath,filename.c_str(),_MAX_PATH);
    return std::string(fullpath);
#else
    //realpath works only with existing files
    //so as work around we create the file first, call then realpath 
    //and delete the temp file
    /** @TODO replace realpath with function with works without this hack */
    bool tempFileCreated=false;
    if(!FileExists(filename))
    {
        tempFileCreated=true;
        std::ofstream os(filename.c_str());
        os.close();
    };
    char *real_path = realpath(filename.c_str(), NULL);
    std::string absPath;
    if(real_path!=NULL)
    {
        absPath=std::string(real_path);
        free(real_path);
    }
    else
    {
        absPath=filename;
    };
    if(tempFileCreated)
    {
        remove(filename.c_str());
    };
    return absPath;
#endif
};

bool IsFileTypeSupported(const std::string& filename)
{
    const std::string extension = getExtension(filename);
    return (vigra::impexListExtensions().find(extension) != std::string::npos);
};

void EnforceExtension(std::string& filename, const std::string& defaultExtension)
{
    const std::string extension = getExtension(filename);
    if (extension.empty())
    {
        filename = stripExtension(filename) + "." + defaultExtension;
    };
}

std::string GetOutputFilename(const std::string& out, const std::string& in, const std::string& suffix)
{
    if (out.empty())
    {
        const std::string extension = getExtension(in);
        if (extension.empty())
        {
            return in + "_" + suffix;
        }
        else
        {
            return in.substr(0, in.length() - extension.length() - 1).append("_" + suffix + "." + extension);
        };
    }
    else
    {
        return out;
    };
}


std::string GetDataDir()
{
#ifdef _WIN32
    char buffer[MAX_PATH];//always use MAX_PATH for filepaths
    GetModuleFileName(NULL,buffer,sizeof(buffer));
    fs::path data_path(buffer);
    data_path.remove_filename();
    if (data_path.has_parent_path())
    {
        return (data_path.parent_path() / "share/hugin/data").string() + "\\";
    };
    return std::string();
#elif defined MAC_SELF_CONTAINED_BUNDLE
    char path[PATH_MAX + 1];
    uint32_t size = sizeof(path);
    std::string data_path("");
    if (_NSGetExecutablePath(path, &size) == 0)
    {
        data_path=dirname(path);
        data_path.append("/../Resources/xrc/");
    }
    return data_path;
#elif defined UNIX_SELF_CONTAINED_BUNDLE
    fs::path data_path = fs::read_symlink("/proc/self/exe");
    data_path.remove_filename();
    if (data_path.has_parent_path())
    {
        return (data_path.parent_path() / "share/hugin/data").string() + "/";
    };
    return std::string();
#else
    return std::string(INSTALL_DATA_DIR);
#endif
};

#ifndef _WIN32
std::string GetHomeDir()
{
    char *homedir = getenv("HOME");
    struct passwd *pw;
    if (homedir == NULL)
    {
        pw = getpwuid(getuid());
        if (pw != NULL)
        {
            homedir = pw->pw_dir;
        };
    };
    if (homedir == NULL)
    {
        return std::string();
    };
    return std::string(homedir);
}
#endif

std::string GetUserAppDataDir()
{
    fs::path path;
#ifdef _WIN32
    char fullpath[_MAX_PATH];
    if(SHGetFolderPath(NULL, CSIDL_APPDATA | CSIDL_FLAG_CREATE, NULL, 0, fullpath)!=S_OK)
    {
        return std::string();
    };
    path = fs::path(fullpath);
    path /= "hugin";
#else
#ifdef USE_XDG_DIRS
    char *xdgDataDir = getenv("XDG_DATA_HOME");
    if (xdgDataDir == NULL || strlen(xdgDataDir) == 0)
    {
        // no XDG_DATA_HOME enviroment variable set or empty variable
        // use $HOME/.local/share instead
        const  std::string homeDir = GetHomeDir();
        if (homeDir.empty())
        {
            return std::string();
        };
        path = fs::path(homeDir);
        path /= ".local/share/hugin";
    }
    else
    {
        // XDG_DATA_HOME set, use hugindata sub directory
        path = fs::path(xdgDataDir);
        path /= "hugin";
    };
#else
    // old behaviour, save in users home directory, sub-directory .hugindata
    const std::string homeDir = GetHomeDir();
    if (homeDir.empty())
    {
        return std::string();
    };
    path = fs::path(homeDir);
    // we have already a file with name ".hugin" for our wxWidgets settings
    // therefore we use directory ".hugindata" in homedir
    path /= ".hugindata";
#endif
#endif
    if (!fs::exists(path))
    {
        if (!fs::create_directories(path))
        {
            std::cerr << "ERROR: Could not create destination directory: " << path.string() << std::endl
                << "Maybe you have not sufficient rights to create this directory." << std::endl;
            return std::string();
        };
    };
    return path.string();
};

// initialization and wrapup of GPU for GPU remapping
#ifdef _WIN32
struct ContextSettings
{
    HWND window;
    HDC dc;
    HGLRC renderingContext;
    
    ContextSettings()
    {
        window = NULL;
        dc = NULL;
        renderingContext = NULL;
    }
};
static ContextSettings context;

// create context, return false if failed
bool CreateContext(int *argcp, char **argv)
{
    WNDCLASS windowClass;
    /* register window class */
    ZeroMemory(&windowClass, sizeof(WNDCLASS));
    windowClass.hInstance = GetModuleHandle(NULL);
    windowClass.lpfnWndProc = DefWindowProc;
    windowClass.lpszClassName = "Hugin";
    if (RegisterClass(&windowClass) == 0)
    {
        return false;
    };
    /* create window */
    context.window = CreateWindow("Hugin", "Hugin", 0, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        CW_USEDEFAULT, NULL, NULL, GetModuleHandle(NULL), NULL);
    if (context.window == NULL)
    {
        return false;
    };
    /* get the device context */
    context.dc = GetDC(context.window);
    if (context.dc == NULL)
    {
        return false;
    };
    /* find pixel format */
    PIXELFORMATDESCRIPTOR pixelFormatDesc;
    ZeroMemory(&pixelFormatDesc, sizeof(PIXELFORMATDESCRIPTOR));
    pixelFormatDesc.nSize = sizeof(PIXELFORMATDESCRIPTOR);
    pixelFormatDesc.nVersion = 1;
    pixelFormatDesc.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL;
    int pixelFormat = ChoosePixelFormat(context.dc, &pixelFormatDesc);
    if (pixelFormat == 0)
    {
        return false;
    }
    /* set the pixel format for the dc */
    if (SetPixelFormat(context.dc, pixelFormat, &pixelFormatDesc) == FALSE)
    {
        return false;
    };
    /* create rendering context */
    context.renderingContext = wglCreateContext(context.dc);
    if (context.renderingContext == NULL)
    {
        return false;
    };
    if (wglMakeCurrent(context.dc, context.renderingContext) == FALSE)
    {
        return false;
    };
#if defined HAVE_EPOXY && HAVE_EPOXY
    epoxy_handle_external_wglMakeCurrent();
#endif
    return true;
}

void DestroyContext()
{
    if (context.renderingContext != NULL)
    {
        wglMakeCurrent(NULL, NULL);
#if defined HAVE_EPOXY && HAVE_EPOXY
        epoxy_handle_external_wglMakeCurrent();
#endif
        wglDeleteContext(context.renderingContext);
    }
    if (context.window != NULL && context.dc != NULL)
    {
        ReleaseDC(context.window, context.dc);
    };
    if (context.window != NULL)
    {
        DestroyWindow(context.window);
    };
    UnregisterClass("Hugin", GetModuleHandle(NULL));
}

#elif defined __APPLE__
static GLuint GlutWindowHandle;
bool CreateContext(int *argcp, char **argv)
{
    // GLUT changes the working directory to the ressource path of the bundle
    // so we store the old working directory and restore it at the end
    char workingDir[FILENAME_MAX];
    getcwd(workingDir, FILENAME_MAX);
    glutInit(argcp, argv);
    glutInitDisplayMode(GLUT_SINGLE | GLUT_RGBA | GLUT_ALPHA);
    GlutWindowHandle = glutCreateWindow("Hugin");
    // restore working dir, as GLUT changes it the ressource path
    chdir(workingDir);
    return true;
}

void DestroyContext()
{
    glutDestroyWindow(GlutWindowHandle);
}

#else
#if defined HAVE_EGL && HAVE_EGL
#if defined HAVE_EPOXY && HAVE_EPOXY
#include <epoxy/egl.h>
#else
#include <EGL/egl.h>
#endif

struct ContextSettings
{
    EGLDisplay m_display;
    EGLContext m_context;
};

static ContextSettings context;

static const EGLint configAttribs[] = {
    EGL_BLUE_SIZE, 8,
    EGL_GREEN_SIZE, 8,
    EGL_RED_SIZE, 8,
    EGL_ALPHA_SIZE, 8,
    EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT,
    EGL_CONFORMANT, EGL_OPENGL_BIT, 
    EGL_COLOR_BUFFER_TYPE, EGL_RGB_BUFFER,
    EGL_NONE
};

bool CreateContext(int *argcp, char **argv)
{
    // get display connection
    context.m_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (context.m_display == EGL_NO_DISPLAY)
    {
        std::cerr << "Could not connect to EGL_DEFAULT_DISPLAY" << std::endl;
        return false;
    };
    // initialize egl
    EGLint major, minor;
    if (eglInitialize(context.m_display, &major, &minor) != EGL_TRUE)
    {
        std::cerr << "Could not initialize EGL" << std::endl
            << "Error: 0x" << std::hex << eglGetError() << std::endl;
        return false;
    };
    std::cout << "Init OpenGL ES " << major << "." << minor << std::endl;
    std::cout << "Client API: " << eglQueryString(context.m_display, EGL_CLIENT_APIS) << std::endl
        << "Vendor: " << eglQueryString(context.m_display, EGL_VENDOR) << std::endl
        << "Version: " << eglQueryString(context.m_display, EGL_VERSION) << std::endl
        << "EGL Extensions: " << eglQueryString(context.m_display, EGL_EXTENSIONS) << std::endl;
    // bind OpenGL API (not OpenGL ES)
    if (!eglBindAPI(EGL_OPENGL_API))
    {
        std::cerr << "Could not bind OpenGL API" << std::endl
            << "Error: 0x" << std::hex << eglGetError() << std::endl;
        return false;
    };
    // choose config
    EGLint numConfigs;
    EGLConfig egl_config;
    if (eglChooseConfig(context.m_display, configAttribs, &egl_config, 1, &numConfigs) != EGL_TRUE)
    {
        std::cerr << "Cound not set egl config" << std::endl
            << "Error: 0x" << std::hex << eglGetError() << std::endl;
        return false;
    };
    // create surface
    // create context and make it current
    context.m_context = eglCreateContext(context.m_display, egl_config, EGL_NO_CONTEXT, NULL);
    if (context.m_context == EGL_NO_CONTEXT)
    {
        std::cerr << "Cound not create context" << std::endl
            << "Error: 0x" << std::hex << eglGetError() << std::endl;
        return false;
    };
    if (eglMakeCurrent(context.m_display, EGL_NO_SURFACE, EGL_NO_SURFACE, context.m_context) != EGL_TRUE)
    {
        std::cerr << "Could not make current context" << std::endl
            << "Error: 0x" << std::hex << eglGetError() << std::endl;
        return false;
    };
    return true;
}

void DestroyContext()
{
    // terminate egl at end
    eglTerminate(context.m_display);
};

#else
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#if defined HAVE_EPOXY && HAVE_EPOXY
#include <epoxy/glx.h>
#else
#include <GL/glx.h>
#endif

struct ContextSettings
{
    Display* display;
    XVisualInfo* visualInfo;
    GLXContext context;
    Window window;
    Colormap colormap;
    
    ContextSettings()
    {
        display=NULL;
        visualInfo=NULL;
        context=NULL;
        window=0;
        colormap=0;
    };
};

static ContextSettings context;

bool CreateContext(int *argcp, char **argv)
{
    /* open display */
    context.display = XOpenDisplay(NULL);
    if (context.display == NULL)
    {
        return false;
    };
    /* query for glx */
    int erb, evb;
    if (!glXQueryExtension(context.display, &erb, &evb))
    {
        return false;
    };
    /* choose visual */
    int attrib[] = { GLX_RGBA, None };
    context.visualInfo = glXChooseVisual(context.display, DefaultScreen(context.display), attrib);
    if (context.visualInfo == NULL)
    {
        return false;
    };
    /* create context */
    context.context = glXCreateContext(context.display, context.visualInfo, None, True);
    if (context.context == NULL)
    {
        return false;
    };
    /* create window */
    context.colormap = XCreateColormap(context.display, RootWindow(context.display, context.visualInfo->screen), context.visualInfo->visual, AllocNone);
    XSetWindowAttributes swa;
    swa.border_pixel = 0;
    swa.colormap = context.colormap;
    context.window = XCreateWindow(context.display, RootWindow(context.display, context.visualInfo->screen),
        0, 0, 1, 1, 0, context.visualInfo->depth, InputOutput, context.visualInfo->visual,
        CWBorderPixel | CWColormap, &swa);
    /* make context current */
    if (!glXMakeCurrent(context.display, context.window, context.context))
    {
        return false;
    };
    return true;
};

void DestroyContext()
{
    if (context.display != NULL && context.context != NULL)
    {
        glXDestroyContext(context.display, context.context);
    }
    if (context.display != NULL && context.window != 0)
    {
        XDestroyWindow(context.display, context.window);
    };
    if (context.display != NULL && context.colormap != 0)
    {
        XFreeColormap(context.display, context.colormap);
    };
    if (context.visualInfo != NULL)
    {
        XFree(context.visualInfo);
    };
    if (context.display != NULL)
    {
        XCloseDisplay(context.display);
    };
};
#endif
#endif

bool initGPU(int *argcp, char **argv)
{
    if (!CreateContext(argcp, argv))
    {
        return false;
    };
    std::cout << hugin_utils::stripPath(argv[0]) << ": using graphics card: " << glGetString(GL_VENDOR) << " " << glGetString(GL_RENDERER) << std::endl;
#if defined HAVE_EPOXY && HAVE_EPOXY
    const GLboolean has_arb_fragment_shader = epoxy_has_gl_extension("GL_ARB_fragment_shader");
    const GLboolean has_arb_vertex_shader = epoxy_has_gl_extension("GL_ARB_vertex_shader");
    const GLboolean has_arb_shader_objects = epoxy_has_gl_extension("GL_ARB_shader_objects");
    const GLboolean has_arb_shading_language = epoxy_has_gl_extension("GL_ARB_shading_language_100");
    const GLboolean has_ext_framebuffer = epoxy_has_gl_extension("GL_EXT_framebuffer_object");
    const GLboolean has_arb_texture_rectangle = epoxy_has_gl_extension("GL_ARB_texture_rectangle");
    const GLboolean has_arb_texture_border_clamp = epoxy_has_gl_extension("GL_ARB_texture_border_clamp");
    const GLboolean has_arb_texture_float = epoxy_has_gl_extension("GL_ARB_texture_float");
#else
    int err = glewInit();
    if (err != GLEW_OK)
    {
        std::cerr << argv[0] << ": an error occurred while setting up the GPU:" << std::endl;
        std::cerr << glewGetErrorString(err) << std::endl;
        std::cerr << argv[0] << ": Switching to CPU calculation." << std::endl;
        DestroyContext();
        return false;
    }

    const GLboolean has_arb_fragment_shader = glewGetExtension("GL_ARB_fragment_shader");
    const GLboolean has_arb_vertex_shader = glewGetExtension("GL_ARB_vertex_shader");
    const GLboolean has_arb_shader_objects = glewGetExtension("GL_ARB_shader_objects");
    const GLboolean has_arb_shading_language = glewGetExtension("GL_ARB_shading_language_100");
    const GLboolean has_ext_framebuffer = glewGetExtension("GL_EXT_framebuffer_object");
    const GLboolean has_arb_texture_rectangle = glewGetExtension("GL_ARB_texture_rectangle");
    const GLboolean has_arb_texture_border_clamp = glewGetExtension("GL_ARB_texture_border_clamp");
    const GLboolean has_arb_texture_float = glewGetExtension("GL_ARB_texture_float");
#endif

    if (!(has_arb_fragment_shader && has_arb_vertex_shader && has_arb_shader_objects && has_arb_shading_language && has_ext_framebuffer && has_arb_texture_rectangle && has_arb_texture_border_clamp && has_arb_texture_float)) {
        const char * msg[] = {"false", "true"};
        std::cerr << argv[0] << ": extension GL_ARB_fragment_shader = " << msg[has_arb_fragment_shader] << std::endl;
        std::cerr << argv[0] << ": extension GL_ARB_vertex_shader = " << msg[has_arb_vertex_shader] << std::endl;
        std::cerr << argv[0] << ": extension GL_ARB_shader_objects = " << msg[has_arb_shader_objects] << std::endl;
        std::cerr << argv[0] << ": extension GL_ARB_shading_language_100 = " << msg[has_arb_shading_language] << std::endl;
        std::cerr << argv[0] << ": extension GL_EXT_framebuffer_object = " << msg[has_ext_framebuffer] << std::endl;
        std::cerr << argv[0] << ": extension GL_ARB_texture_rectangle = " << msg[has_arb_texture_rectangle] << std::endl;
        std::cerr << argv[0] << ": extension GL_ARB_texture_border_clamp = " << msg[has_arb_texture_border_clamp] << std::endl;
        std::cerr << argv[0] << ": extension GL_ARB_texture_float = " << msg[has_arb_texture_float] << std::endl;
        std::cerr << argv[0] << ": This graphics system lacks the necessary extensions for -g." << std::endl;
        std::cerr << argv[0] << ": Switching to CPU calculation." << std::endl;
        DestroyContext();
        return false;
    }

    return true;
}

bool wrapupGPU()
{
    DestroyContext();
    return true;
}

std::string GetHuginVersion()
{
    return std::string(DISPLAY_VERSION);
};

std::string GetICCDesc(const vigra::ImageImportInfo::ICCProfile& iccProfile)
{
    if (iccProfile.empty())
    {
        // no profile
        return std::string();
    };
    cmsHPROFILE profile = cmsOpenProfileFromMem(iccProfile.data(), iccProfile.size());
    if (profile == NULL)
    {
        // invalid profile
        return std::string();
    };
    const std::string name=GetICCDesc(profile);
    cmsCloseProfile(profile);
    return name;
};

std::string GetICCDesc(const cmsHPROFILE& profile)
{
    const size_t size = cmsGetProfileInfoASCII(profile, cmsInfoDescription, cmsNoLanguage, cmsNoCountry, nullptr, 0);
    std::string information(size, '\000');
    cmsGetProfileInfoASCII(profile, cmsInfoDescription, cmsNoLanguage, cmsNoCountry, &information[0], size);
    StrTrim(information);
    return information;
}

/** return vector of known extensions of raw files, all lower case */
std::vector<std::string> GetRawExtensions()
{
    std::vector<std::string> rawExt{ "dng", "crw", "cr2","cr3","raw","erf","raf","mrw","nef","orf","rw2","pef","srw","arw" };
    return rawExt;
};

bool IsRawExtension(const std::string testExt)
{
    const std::string testExtLower = tolower(testExt);
    const auto rawExts = GetRawExtensions();
    for (const auto& ext : rawExts)
    {
        if (testExtLower.compare(ext) == 0)
        {
            return true;
        };
    };
    return false;
};

} //namespace
