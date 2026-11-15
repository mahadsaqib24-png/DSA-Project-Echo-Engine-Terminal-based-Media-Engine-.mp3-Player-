#ifndef FILE_UTILS_H
#define FILE_UTILS_H

/*
============================================================
                SHARED FILE / STRING HELPERS
============================================================
*/

#include <string>
#include <fstream>
#include <iostream>
#include <cstdlib>
#include <cctype>
#include <cstdio>
#include <sys/stat.h>
#include "Structs.h"
#ifdef _WIN32
  #include <windows.h>
  #include <direct.h>
#else
  #include <unistd.h>
#endif

using namespace std;

/*
============================================================
                 Paths and format constants
============================================================
*/
#define DATA_DIR "data/"
const char FIELD_DELIM = '|';
const int  MAX_FIELDS  = 8;           // size to use for parts[] arrays

inline string dataPath(const string& name) { return string(DATA_DIR) + name; }

/*
============================================================
                       String helpers
============================================================
*/

inline string intStr(int v) {
    if (v == 0) return "0";
    bool neg = v < 0;
    unsigned int u = neg ? (unsigned int)(-(v + 1)) + 1u : (unsigned int)v;
    string s;
    while (u > 0) { s = string(1, (char)('0' + u % 10)) + s; u /= 10; }
    return neg ? "-" + s : s;
}

inline string stripCR(const string& s) {
    if (!s.empty() && s[s.size() - 1] == '\r') return s.substr(0, s.size() - 1);
    return s;
}

inline string trim(const string& s) {
    size_t b = 0, e = s.size();
    while (b < e && isspace((unsigned char)s[b])) b++;
    while (e > b && isspace((unsigned char)s[e - 1])) e--;
    return s.substr(b, e - b);
}

inline string toLower(const string& s) {
    string r = s;
    for (size_t i = 0; i < r.size(); i++) r[i] = (char)tolower((unsigned char)r[i]);
    return r;
}

inline bool containsDelim(const string& s) { return s.find(FIELD_DELIM) != string::npos; }

// True for lines the loaders must ignore: blank lines and lines starting with '#'. O(n)
inline bool isBlankOrComment(const string& line) {
    string t = trim(line);
    return t.empty() || t[0] == '#';
}
/* 
	Split 'line' on 'delim'. Each field is trimmed and stored in parts[] (first maxParts only).
	Returns the TOTAL number of fields found, which can be larger than maxParts,
	so callers can check:   if (splitLine(line, FIELD_DELIM, p, MAX_FIELDS) != 4) -> malformed.
	An empty line returns 1 (one empty field). "a||b" returns 3 (middle field empty). O(n)
*/
inline int splitLine(const string& raw, char delim, string parts[], int maxParts) {
    string line = stripCR(raw);
    int count = 0;
    string current;
    for (size_t i = 0; i <= line.size(); i++) {
        if (i == line.size() || line[i] == delim) {
            if (count < maxParts) parts[count] = trim(current);
            count++;
            current = "";
        } else {
            current += line[i];
        }
    }
    return count;
}

// Parse a whole decimal integer (optional leading '-'). Returns false for empty text,
// letters, decimals, or numbers longer than 9 digits (avoids overflow). O(n)
inline bool toInt(const string& text, int& out) {
    string s = trim(text);
    if (s.empty()) return false;
    size_t i = 0;
    if (s[0] == '-') i = 1;
    if (i >= s.size() || s.size() - i > 9) return false;
    for (size_t k = i; k < s.size(); k++)
        if (!isdigit((unsigned char)s[k])) return false;
    out = atoi(s.c_str());
    return true;
}

inline int min3(int a, int b, int c) { int m = a < b ? a : b; return m < c ? m : c; }

// Edit distance, two rolling rows: O(n*m) time, O(m) memory.
inline int levenshtein(const string& a, const string& b) {
    int n = (int)a.size(), m = (int)b.size();
    int* prev = new int[m + 1];
    int* cur  = new int[m + 1];
    for (int j = 0; j <= m; j++) prev[j] = j;
    for (int i = 1; i <= n; i++) {
        cur[0] = i;
        for (int j = 1; j <= m; j++) {
            int cost = (a[i - 1] == b[j - 1]) ? 0 : 1;
            cur[j] = min3(prev[j] + 1, cur[j - 1] + 1, prev[j - 1] + cost);
        }
        int* t = prev; prev = cur; cur = t;
    }
    int r = prev[m];
    delete[] prev;
    delete[] cur;
    return r;
}

// Replaces characters that Windows forbids in file names.
inline string safeFileName(const string& in) {
    string s = in;
    const string bad = "\\/:*?\"<>|";
    for (size_t i = 0; i < s.size(); i++)
        if (bad.find(s[i]) != string::npos) s[i] = '_';
    return s;
}

/*
============================================================
        Song identity + the shared Title|Artist|Album|Secs
        line format (library, playlists, saved files)
============================================================
*/

// Two songs are the same track only when artist AND title match (case-insensitive).
inline string songKey(const string& artist, const string& title) {
    return toLower(artist) + FIELD_DELIM + toLower(title);
}
inline string songKey(const Song& s) { return songKey(s.artist, s.title); }

// Name (without extension) of the audio file that is stored for a song: "Artist - Title".
inline string songFileStem(const Song& s) { return safeFileName(s.artist + " - " + s.title); }

inline string songToLine(const Song& s) {
    return s.title + FIELD_DELIM + s.artist + FIELD_DELIM + s.album + FIELD_DELIM + intStr(s.durationSeconds);
}

// Parses one Title|Artist|Album|DurationSecs line. On failure 'why' holds the reason.
inline bool parseSongLine(const string& line, Song& out, string& why) {
    string p[MAX_FIELDS];
    int dur = 0;
    if (splitLine(line, FIELD_DELIM, p, MAX_FIELDS) != 4) { why = "expected 4 fields"; return false; }
    if (p[0].empty() || p[1].empty())                      { why = "empty title or artist"; return false; }
    if (!toInt(p[3], dur) || dur <= 0)                     { why = "bad duration"; return false; }
    out.title = p[0]; out.artist = p[1]; out.album = p[2]; out.durationSeconds = dur;
    return true;
}

/*
============================================================
   Per-user file names. Upper-case letters become "-" + the
   lower-case letter, so "Bob" and "bob" never share a file
   (Windows file names ignore case).
============================================================
*/
inline string userFileName(const string& username) {
    string out;
    for (size_t i = 0; i < username.size(); i++) {
        unsigned char c = (unsigned char)username[i];
        if (isupper(c))                    { out += '-'; out += (char)tolower(c); }
        else if (isalnum(c) || c == '_')   out += (char)c;
        else                               out += '_';
    }
    return out;
}

inline string settingsPath(const string& username) { return dataPath(userFileName(username) + "_settings.txt"); }

/*
============================================================
                   File / folder helpers
============================================================
*/

inline bool fileExists(const string& path) {
    ifstream f(path.c_str());
    return f.good();
}

// Create a folder if it does not exist. Returns true if it exists afterwards.
inline bool makeDir(const string& path) {
    struct stat st;
    if (stat(path.c_str(), &st) == 0) return true;
#ifdef _WIN32
    return _mkdir(path.c_str()) == 0;
#else
    return mkdir(path.c_str(), 0755) == 0;
#endif
}

inline bool ensureDataDirs() {
    bool a = makeDir("data");
    bool b = makeDir("data/playlists");
    return a && b;
}

inline void reportBadLine(const string& file, int lineNo, const string& reason) {
    cout << "  [~] " << file << " line " << lineNo << " skipped: " << reason << "\n";
}

/*
================================================================================
  UTF-8 file names. The program stores and prints everything as UTF-8. Windows
  file APIs use UTF-16, so names with accents (Moneskin, Duhe) need conversion.
================================================================================
*/
#ifdef _WIN32
inline wstring utf8ToWide(const string& s) {
    if (s.empty()) return wstring();
    int n = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), NULL, 0);
    if (n <= 0) return wstring();
    wchar_t* buf = new wchar_t[n + 1];
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), buf, n);
    buf[n] = 0;
    wstring w(buf);
    delete[] buf;
    return w;
}

inline string wideToUtf8(const wchar_t* w) {
    int n = WideCharToMultiByte(CP_UTF8, 0, w, -1, NULL, 0, NULL, NULL);
    if (n <= 1) return string();
    char* buf = new char[n];
    WideCharToMultiByte(CP_UTF8, 0, w, -1, buf, n, NULL, NULL);
    string s(buf);
    delete[] buf;
    return s;
}

inline bool fileExistsU(const string& path) {
    FILE* f = _wfopen(utf8ToWide(path).c_str(), L"rb");
    if (f == NULL) return false;
    fclose(f);
    return true;
}

inline FILE* openBinary(const string& path)      { return _wfopen(utf8ToWide(path).c_str(), L"rb"); }
inline FILE* openBinaryWrite(const string& path) { return _wfopen(utf8ToWide(path).c_str(), L"wb"); }
inline bool  renameFileU(const string& from, const string& to) {
    return MoveFileW(utf8ToWide(from).c_str(), utf8ToWide(to).c_str()) != 0;
}

// True when stdin is a real console (not a pipe or a file).
inline bool stdinIsConsole() {
    static int known = -1;
    if (known < 0) {
        DWORD m = 0;
        known = (GetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), &m) != 0) ? 1 : 0;
    }
    return known == 1;
}

// Reads one line with ReadConsoleW and converts it to UTF-8, so accented input works under code page 65001.
// Returns false at end of input (Ctrl+Z).
inline bool readConsoleLineUtf8(string& out) {
    HANDLE h = GetStdHandle(STD_INPUT_HANDLE);
    wstring acc;
    wchar_t buf[256];
    DWORD got = 0;
    bool done = false;
    while (!done) {
        if (!ReadConsoleW(h, buf, 256, &got, NULL) || got == 0) { if (acc.empty()) return false; break; }
        for (DWORD i = 0; i < got; i++) {
            acc += buf[i];
            if (buf[i] == L'\n') done = true;
        }
    }
    while (!acc.empty() && (acc[acc.size() - 1] == L'\n' || acc[acc.size() - 1] == L'\r')) acc.erase(acc.size() - 1);
    if (!acc.empty() && acc[0] == (wchar_t)0x1A) return false;
    out = wideToUtf8(acc.c_str());
    return true;
}
#else
// Reads one line straight from stdin, one byte at a time, so nothing is read ahead:
// the raw key reads (passwords, player) always see the rest of the input. False at end of input.
inline bool readStdinLine(string& out) {
    out = "";
    char c;
    bool any = false;
    while (true) {
        ssize_t n = read(0, &c, 1);
        if (n <= 0) return any;
        any = true;
        if (c == '\n') return true;
        out += c;
    }
}
inline bool fileExistsU(const string& path)      { return fileExists(path); }
inline FILE* openBinary(const string& path)      { return fopen(path.c_str(), "rb"); }
inline FILE* openBinaryWrite(const string& path) { return fopen(path.c_str(), "wb"); }
inline bool  renameFileU(const string& from, const string& to) { return rename(from.c_str(), to.c_str()) == 0; }
#endif

// Copies a file byte for byte (UTF-8 paths work on Windows too).
inline bool copyFileBinary(const string& from, const string& to) {
    FILE* in = openBinary(from);
    if (in == NULL) return false;
    FILE* out = openBinaryWrite(to);
    if (out == NULL) { fclose(in); return false; }
    char buf[4096];
    bool ok = true;
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), in)) > 0)
        if (fwrite(buf, 1, n, out) != n) { ok = false; break; }
    fclose(in);
    if (fclose(out) != 0) ok = false;
    return ok;
}

#endif
