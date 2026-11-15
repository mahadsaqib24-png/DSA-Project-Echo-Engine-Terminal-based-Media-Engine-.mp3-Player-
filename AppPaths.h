#ifndef APP_PATHS_H
#define APP_PATHS_H

/*
==========================================================================
                         APP PATHS
  Makes Echo Engine work from ANY folder. (Saftey Check) 

  Problem: data/ and music/ were relative to wherever the program
  was launched from (IDE, double-click and command prompt all differ),
  and protected folders (Documents, OneDrive, lab PCs) may refuse writes.
  
  Fix: at startup, change the working directory to
    1) the folder containing the .exe, if it is writable; else
    2) a per-user writable folder  <LOCALAPPDATA>\EchoEngine
       (or APPDATA / USERPROFILE / HOME as fallbacks).
  Every other module keeps using plain relative paths.
===========================================================================
*/

#include <string>
#include <fstream>
#include <cstdio>
#include <cstdlib>
#include "FileUtils.h"
#ifdef _WIN32
  #include <windows.h>
  #include <direct.h>
  #define CHANGE_DIR(p) _chdir(p)
#else
  #include <unistd.h>
  #include <sys/types.h>
  #define CHANGE_DIR(p) chdir(p)
#endif

// Folder that contains the running executable ("" if unknown).
inline string exeDirectory() {
#ifdef _WIN32
    char buf[MAX_PATH + 1];
    DWORD n = GetModuleFileNameA(NULL, buf, MAX_PATH);
    if (n == 0 || n >= MAX_PATH) return "";
    string p(buf, n);
#else
    char buf[4096];
    ssize_t n = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
    if (n <= 0) return "";
    string p(buf, (size_t)n);
#endif
    size_t sep = p.find_last_of("/\\");
    return sep == string::npos ? string("") : p.substr(0, sep);
}

// Real write test (creates then deletes a tiny file): also catches
// Windows "Controlled folder access" and read-only network/USB folders.
inline bool dirIsWritable(const string& dir) {
    string t = dir + "/echo_write_test.tmp";
    {
        ofstream f(t.c_str());
        if (!f) return false;
        f << "x";
        f.flush();
        if (!f.good()) return false;
    }
    bool ok = fileExists(t);
    remove(t.c_str());
    return ok;
}

// Picks and enters the working folder. 'chosen' receives the path;
// 'usedFallback' is true when the exe's own folder could not be used.
// Returns false only if no writable folder could be found at all.
inline bool setupWorkingDirectory(string& chosen, bool& usedFallback) {
    usedFallback = false;

    string dir = exeDirectory();
    if (!dir.empty() && dirIsWritable(dir) && CHANGE_DIR(dir.c_str()) == 0) {
        chosen = dir;
        return true;
    }

    const char* envs[4] = { "LOCALAPPDATA", "APPDATA", "USERPROFILE", "HOME" };
    for (int i = 0; i < 4; i++) {
        const char* v = getenv(envs[i]);
        if (v == NULL || *v == '\0') continue;
        string base = string(v) + "/EchoEngine";
        if (makeDir(base) && dirIsWritable(base) && CHANGE_DIR(base.c_str()) == 0) {
            chosen = base;
            usedFallback = true;
            return true;
        }
    }

    chosen = ".";                                   // last resort: current folder
    usedFallback = true;
    return dirIsWritable(".");
}

#endif
