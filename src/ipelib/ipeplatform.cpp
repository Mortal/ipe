// --------------------------------------------------------------------
// Platform dependent methods
// --------------------------------------------------------------------
/*

    This file is part of the extensible drawing editor Ipe.
    Copyright (C) 1993-2010  Otfried Cheong

    Ipe is free software; you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    As a special exception, you have permission to link Ipe with the
    CGAL library and distribute executables, as long as you follow the
    requirements of the Gnu General Public License in regard to all of
    the software in the executable aside from CGAL.

    Ipe is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
    or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
    License for more details.

    You should have received a copy of the GNU General Public License
    along with Ipe; if not, you can find it at
    "http://www.gnu.org/copyleft/gpl.html", or write to the Free
    Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/

#include "ipebase.h"

#ifdef WIN32
#include <windows.h>
#include <shlobj.h>
#else
#include <sys/wait.h>
#endif
#include <cstdlib>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdarg.h>
#include <unistd.h>
#include <clocale>

using namespace ipe;

// --------------------------------------------------------------------

/*! \class ipe::Platform
  \ingroup base
  \brief Platform dependent methods.
*/

//! Return the Ipelib version.
/*! This is available as a function so that one can verify what
  version of Ipelib one has actually linked with (as opposed to the
  header files used during compilation).
*/
int Platform::libVersion()
{
  return IPELIB_VERSION;
}

// --------------------------------------------------------------------

static bool showDebug = false;
static Platform::DebugHandler debugHandler = 0;

static void debugHandlerImpl(const char *msg)
{
  if (showDebug) {
#ifdef WIN32
    OutputDebugStringA(msg);
#endif
    fprintf(stderr, "%s\n", msg);
  }
}

//! Initialize Ipelib.
/*! This method must be called before Ipelib is used.

  It sets the LC_NUMERIC locale to 'C', which is necessary for correct
  loading and saving of Ipe objects.  The method also checks that the
  correct version of Ipelib is loaded, and aborts with an error
  message if the version is not correct.  Also enables ipeDebug
  messages if environment variable IPEDEBUG is defined.  (You can
  override this using setDebug).
*/
void Platform::initLib(int version)
{
  // Ensure numbers are output with a period rather than a comma
  // as the decimal point.
  setlocale(LC_NUMERIC, "C");
  showDebug = false;
  if (getenv("IPEDEBUG") != 0)
    showDebug = true;
  debugHandler = debugHandlerImpl;
  if (version == IPELIB_VERSION)
    return;
  fprintf(stderr,
	  "Ipetoipe has been compiled with header files for Ipelib %d\n"
	  "but is dynamically linked against libipe %d.\n"
#ifndef WIN32
	  "Check with 'ldd' which libipe is being loaded, and "
	  "replace it by the correct version or set LD_LIBRARY_PATH.\n"
#endif
	  , version, IPELIB_VERSION);
  exit(99);
}

//! Enable or disable display of ipeDebug messages.
void Platform::setDebug(bool debug)
{
  showDebug = debug;
}

// --------------------------------------------------------------------

void ipeDebug(const char *msg, ...)
{
  if (debugHandler) {
    char buf[8196];
    va_list ap;
    va_start(ap, msg);
    std::vsprintf(buf, msg, ap);
    va_end(ap);
    debugHandler(buf);
  }
}

// --------------------------------------------------------------------

//! Return correct path separator for this platform.
char Platform::pathSeparator()
{
#ifdef WIN32
  return '\\';
#else
  return '/';
#endif
}

//! Returns current working directory.
/*! Returns empty string if something fails. */
String Platform::currentDirectory()
{
#ifdef __MINGW32__
  char *buffer;
  if ((buffer = _getcwd(0, 0)) == 0)
    return String();
  return String(buffer);
#else
  char buffer[1024];
  if (getcwd(buffer, 1024) != buffer)
    return String();
  return String(buffer);
#endif
}

#ifdef WIN32
String Platform::ipeDir(const char *suffix)
{
  char exename[OFS_MAXPATHNAME];
  GetModuleFileNameA(0, exename, OFS_MAXPATHNAME);
  String exe(exename);
  int i = exe.rfind('\\');
  if (i >= 0) {
    i = exe.left(i).rfind('\\');
    if (i >= 0)
      return exe.left(i + 1) + suffix;
  }
  return String("\\") + suffix;
}
#else
static String dotIpe()
{
  const char *home = getenv("HOME");
  if (!home)
    return String();
  String res = String(home) + "/.ipe";
  if (!Platform::fileExists(res) && mkdir(res.z(), 0700) != 0)
    return String();
  return res + "/";
}
#endif

//! Returns directory for running Latex.
/*! The directory is created if it does not exist.  Returns an empty
  string if the directory cannot be found or cannot be created.
  The directory returned ends in the path separator.
 */
String Platform::latexDirectory()
{
#ifdef __MINGW32__
  String latexDir;
  const char *p = getenv("IPELATEXDIR");
  if (p) {
    latexDir = p;
  } else {
    TCHAR szPath[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPath(NULL, CSIDL_LOCAL_APPDATA,
				  NULL, 0, szPath))) {
      latexDir = String(szPath) + "\\ipe";
    } else {
      p = getenv("LOCALAPPDATA");
      if (p)
	latexDir = String(p) + "\\ipe";
      else
	latexDir = ipeDir("latexrun");
    }
  }
  if (latexDir.right(1) == "\\")
    latexDir = latexDir.left(latexDir.size() - 1);
  if (!fileExists(latexDir)) {
    if (_mkdir(latexDir.z()) != 0)
      return String();
  }
  latexDir += "\\";
  return latexDir;
#else
  const char *p = getenv("IPELATEXDIR");
  String latexDir;
  if (p) {
    latexDir = p;
    if (latexDir.right(1) == "/")
      latexDir = latexDir.left(latexDir.size() - 1);
  } else {
    latexDir = dotIpe() + "latexrun";
  }
  if (!fileExists(latexDir) && mkdir(latexDir.z(), 0700) != 0)
    return String();
  latexDir += "/";
  return latexDir;
#endif
}

//! Returns filename of fontmap.
String Platform::fontmapFile()
{
  const char *p = getenv("IPEFONTMAP");
  if (p)
    return String(p);
#ifdef WIN32
  return ipeDir("data\\fontmap.xml");
#else
  return IPEFONTMAP;
#endif
}

//! Determine whether file exists.
bool Platform::fileExists(String fname)
{
#ifdef __MINGW32__
  struct _stat statBuffer;
  return (_stat(fname.z(), &statBuffer) == 0);
#else
  return (access(fname.z(), F_OK) == 0);
#endif
}

//! Read entire file into string.
/*! Returns an empty string if file cannot be found or read.
  There is no way to distinguish an empty file from this. */
String Platform::readFile(String fname)
{
  std::FILE *file = std::fopen(fname.z(), "rb");
  if (!file)
    return String();
  String s;
  int ch;
  while ((ch = std::fgetc(file)) != EOF)
    s.append(ch);
  std::fclose(file);
  return s;
}

//! Runs pdflatex on file text.tex in given directory.
int Platform::runPdfLatex(String dir)
{
#ifdef WIN32
  String s = dir + "runlatex.bat";
  std::FILE *f = std::fopen(s.z(), "wb");
  if (!f)
    return -1;

  if (dir.size() > 2 && dir[1] == ':')
    fprintf(f, "%s\r\n", dir.substr(0, 2).z());

  // CMD.EXE input needs to be encoded in "OEM codepage",
  // which can be different from "Windows codepage"
  Buffer oemDir(2 * dir.size() + 1);
  CharToOemA(dir.z(), oemDir.data());

  fprintf(f, "cd \"%s\"\r\n", oemDir.data());
  fprintf(f, "pdflatex text.tex\r\n");
  std::fclose(f);

  s = String("call \"") + dir + String("runlatex.bat\"");
  std::system(s.z());
  return 0;
#else
  String s("cd ");
  s += dir;
  s += "; rm -f text.log; pdflatex text.tex > /dev/null";
  int result = std::system(s.z());
  if (result != -1)
    result = WEXITSTATUS(result);
  return result;
#endif
}

// --------------------------------------------------------------------

void ipeAssertionFailed(const char *file, int line, const char *assertion)
{
  fprintf(stderr, "Assertion failed on line #%d (%s): '%s'\n",
	  line, file, assertion);
  abort();
  // exit(-1);
}

// --------------------------------------------------------------------
