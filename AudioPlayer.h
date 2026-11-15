#ifndef AUDIO_PLAYER_H
#define AUDIO_PLAYER_H

/*
===============================================================================
          AUDIO BACKEND  (Windows + Linux, C++98)

  Windows : asynchronous playback through MCI (mciSendString).
            Dev-C++ / MinGW: link with  -lwinmm
  
  Linux   : no audio library is linked. The track is played by a
            small command-line player started with fork()/execvp()
            (first one found on PATH):   mpv  >  ffplay  >  mpg123
            Install one with:   sudo apt install mpv
                           or:  sudo apt install ffmpeg   (ffplay + ffprobe)
            pause/resume = SIGSTOP/SIGCONT, stop = SIGTERM.
  
  Tracks: music/<Title>.mp3  (.wav / .wma, plus .ogg .flac .m4a on Linux)
================================================================================
*/

#include <string>
#include <fstream>
#include <cstdlib>
#include <cstdio>
#include <cwchar>
#include "FileUtils.h"
#include "colours.h"
#ifdef _WIN32
  #include <windows.h>
  #include <mmsystem.h>
  #ifdef _MSC_VER
    #pragma comment(lib, "winmm.lib")
  #endif
#else
  #include <unistd.h>
  #include <signal.h>
  #include <fcntl.h>
  #include <dirent.h>
  #include <sys/types.h>
  #include <sys/stat.h>
  #include <sys/time.h>
  #include <sys/wait.h>
  #ifdef __linux__
    #include <sys/prctl.h>
  #endif
#endif

#define MUSIC_DIR "music/"

/*
============================================================
                    Small shared helpers
============================================================
*/

// Returns the extension (with dot, lower-case) or "".
inline string fileExtension(const string& path) {
    size_t dot = path.find_last_of('.');
    size_t sep = path.find_last_of("/\\");
    if (dot == string::npos || (sep != string::npos && dot < sep)) return "";
    return toLower(path.substr(dot));
}

// File name without its extension ("AC DC - Ballbreaker.mp3" -> "AC DC - Ballbreaker").
inline string fileStem(const string& name) {
    size_t dot = name.find_last_of('.');
    size_t sep = name.find_last_of("/\\");
    if (dot == string::npos || (sep != string::npos && dot < sep)) return name;
    return name.substr(0, dot);
}

inline bool isAudioExt(const string& ext) {
    return ext == ".mp3" || ext == ".wav" || ext == ".wma"
#ifndef _WIN32
        || ext == ".ogg" || ext == ".flac" || ext == ".m4a"
#endif
        ;
}

/*
==================================================================================================
  Song length read straight from the file headers (no audio library, no MCI).
  Windows MCI reports a wrong (often doubled) length for some MP3s, so this is tried first.
  MP3: Xing / Info / VBRI frame count, otherwise constant bitrate.  WAV: byte rate.  0 = unknown.
==================================================================================================
*/
inline int readAt(FILE* f, long pos, unsigned char* buf, int n) {
    if (fseek(f, pos, SEEK_SET) != 0) return 0;
    return (int)fread(buf, 1, n, f);
}

inline unsigned long beU32(const unsigned char* p) {
    return ((unsigned long)p[0] << 24) | ((unsigned long)p[1] << 16) | ((unsigned long)p[2] << 8) | p[3];
}

inline unsigned long leU32(const unsigned char* p) {
    return ((unsigned long)p[3] << 24) | ((unsigned long)p[2] << 16) | ((unsigned long)p[1] << 8) | p[0];
}

inline bool tagAt(const unsigned char* p, const char* tag) {
    for (int i = 0; tag[i]; i++) if (p[i] != (unsigned char)tag[i]) return false;
    return true;
}

inline int mp3LengthSeconds(FILE* f, long fileSize) {
    static const int BR_V1L1[15]  = {0,32,64,96,128,160,192,224,256,288,320,352,384,416,448};
    static const int BR_V1L2[15]  = {0,32,48,56,64,80,96,112,128,160,192,224,256,320,384};
    static const int BR_V1L3[15]  = {0,32,40,48,56,64,80,96,112,128,160,192,224,256,320};
    static const int BR_V2L1[15]  = {0,32,48,56,64,80,96,112,128,144,160,176,192,224,256};
    static const int BR_V2L23[15] = {0,8,16,24,32,40,48,56,64,80,96,112,128,144,160};
    static const int SR_V1[3] = {44100, 48000, 32000}, SR_V2[3] = {22050, 24000, 16000}, SR_V25[3] = {11025, 12000, 8000};

    unsigned char h[10];
    long start = 0, end = fileSize;
    if (readAt(f, 0, h, 10) < 10) return 0;
    if (tagAt(h, "ID3")) {                                       // skip the ID3v2 tag
        start = 10 + (((long)(h[6] & 0x7F) << 21) | ((long)(h[7] & 0x7F) << 14) | ((long)(h[8] & 0x7F) << 7) | (h[9] & 0x7F));
        if (h[5] & 0x10) start += 10;
    }
    unsigned char t[3];
    if (fileSize > 128 && readAt(f, fileSize - 128, t, 3) == 3 && tagAt(t, "TAG")) end -= 128;   // ID3v1 tag

    const int N = 65536;
    unsigned char* buf = new unsigned char[N];
    int got = readAt(f, start, buf, N);
    int result = 0;
    for (int i = 0; i + 4 <= got && result == 0; i++) {
        if (buf[i] != 0xFF || (buf[i + 1] & 0xE0) != 0xE0) continue;
        int ver = (buf[i + 1] >> 3) & 3, layer = (buf[i + 1] >> 1) & 3;     // layer: 3 = I, 2 = II, 1 = III
        int bi = (buf[i + 2] >> 4) & 15, si = (buf[i + 2] >> 2) & 3, pad = (buf[i + 2] >> 1) & 1;
        if (ver == 1 || layer == 0 || bi == 0 || bi == 15 || si == 3) continue;
        bool v1 = (ver == 3);
        int kbps = v1 ? (layer == 3 ? BR_V1L1[bi] : layer == 2 ? BR_V1L2[bi] : BR_V1L3[bi])
                      : (layer == 3 ? BR_V2L1[bi] : BR_V2L23[bi]);
        int sr = v1 ? SR_V1[si] : (ver == 2 ? SR_V2[si] : SR_V25[si]);
        int spf = (layer == 3) ? 384 : ((layer == 2 || v1) ? 1152 : 576);   // MPEG-2/2.5 layer III: 576
        int frameLen = (layer == 3) ? (12 * kbps * 1000 / sr + pad) * 4
                     : (layer == 2 || v1) ? 144 * kbps * 1000 / sr + pad : 72 * kbps * 1000 / sr + pad;
        if (i + frameLen + 2 <= got && (buf[i + frameLen] != 0xFF || (buf[i + frameLen + 1] & 0xE0) != 0xE0)) continue;

        unsigned long frames = 0;                                // VBR header: exact frame count
        if (layer == 1) {
            bool mono = ((buf[i + 3] >> 6) == 3);
            int x = i + 4 + (v1 ? (mono ? 17 : 32) : (mono ? 9 : 17));
            if (x + 12 <= got && (tagAt(buf + x, "Xing") || tagAt(buf + x, "Info")) && (buf[x + 7] & 1)) frames = beU32(buf + x + 8);
            int vb = i + 4 + 32;
            if (frames == 0 && vb + 18 <= got && tagAt(buf + vb, "VBRI")) frames = beU32(buf + vb + 14);
        }
        if (frames > 0) result = (int)((double)frames * spf / sr + 0.5);
        else            result = (int)((double)(end - (start + i)) * 8.0 / (kbps * 1000.0) + 0.5);   // constant bitrate
    }
    delete[] buf;
    return result;
}

inline int wavLengthSeconds(FILE* f, long fileSize) {
    unsigned char h[12], c[8], fmt[16];
    if (readAt(f, 0, h, 12) < 12 || !tagAt(h, "RIFF") || !tagAt(h + 8, "WAVE")) return 0;
    long pos = 12;
    unsigned long byteRate = 0, dataSize = 0;
    while (pos + 8 <= fileSize) {
        if (readAt(f, pos, c, 8) < 8) break;
        unsigned long sz = leU32(c + 4);
        if (tagAt(c, "fmt ")) {
            if (readAt(f, pos + 8, fmt, 16) < 16) break;
            byteRate = leU32(fmt + 8);
        } else if (tagAt(c, "data")) {
            dataSize = sz;
            if (dataSize > (unsigned long)(fileSize - pos - 8)) dataSize = (unsigned long)(fileSize - pos - 8);
            break;
        }
        pos += 8 + (long)sz + (long)(sz & 1);
    }
    return (byteRate > 0 && dataSize > 0) ? (int)((double)dataSize / byteRate + 0.5) : 0;
}

inline int nativeLengthSeconds(const string& path) {
    string ext = fileExtension(path);
    if (ext != ".mp3" && ext != ".wav") return 0;
    FILE* f = openBinary(path);
    if (f == NULL) return 0;
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    int r = 0;
    if (size > 0) r = (ext == ".mp3") ? mp3LengthSeconds(f, size) : wavLengthSeconds(f, size);
    fclose(f);
    return r;
}
/*
=============================================================================
  Folder listing: names of the audio files inside 'dir' (must end with '/').
  Returns a new[] array (caller does delete[]); n receives the count.
=============================================================================
*/
inline void pushName(string*& arr, int& n, int& cap, const string& s) {
    if (n == cap) {                                   // manual growth: double the array
        string* bigger = new string[cap * 2];
        for (int i = 0; i < n; i++) bigger[i] = arr[i];
        delete[] arr;
        arr = bigger;
        cap *= 2;
    }
    arr[n++] = s;
}

inline string* listMusicFiles(const string& dir, int& n) {
    int cap = 16;
    n = 0;
    string* out = new string[cap];
#ifdef _WIN32
    WIN32_FIND_DATAW fd;
    HANDLE h = FindFirstFileW((utf8ToWide(dir) + L"*").c_str(), &fd);
    if (h != INVALID_HANDLE_VALUE) {
        do {
            if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;
            string name = wideToUtf8(fd.cFileName);
            if (isAudioExt(fileExtension(name))) pushName(out, n, cap, name);
        } while (FindNextFileW(h, &fd));
        FindClose(h);
    }
#else
    DIR* d = opendir(dir.c_str());
    if (d != NULL) {
        struct dirent* e;
        while ((e = readdir(d)) != NULL) {
            string name = e->d_name;
            if (name == "." || name == "..") continue;
            struct stat st;
            if (stat((dir + name).c_str(), &st) != 0 || !S_ISREG(st.st_mode)) continue;
            if (isAudioExt(fileExtension(name))) pushName(out, n, cap, name);
        }
        closedir(d);
    }
#endif
    return out;
}

// Finds the audio file that belongs to a song. Returns "" if none exists.
// The file is named "Artist - Title.<ext>", or just "Title.<ext>" when no other artist uses that title.
// titleShared = another song in the library has the same title (only the "Artist - Title" name matches then).
inline string locateTrackFile(const Song& song, bool titleShared) {
    const char* ext[9] = { ".mp3", ".wav", ".wma", ".MP3", ".WAV", ".WMA", ".ogg", ".flac", ".m4a" };
    string comp = songFileStem(song), safe = safeFileName(song.title);
    for (int i = 0; i < 9; i++) {
        string p = string(MUSIC_DIR) + comp + ext[i];
        if (fileExistsU(p)) return p;
        if (titleShared) continue;
        p = string(MUSIC_DIR) + song.title + ext[i];
        if (fileExistsU(p)) return p;
        if (safe != song.title) {
            p = string(MUSIC_DIR) + safe + ext[i];
            if (fileExistsU(p)) return p;
        }
    }
    int n = 0;
    string* files = listMusicFiles(MUSIC_DIR, n);
    string keyComp = toLower(comp), keyTitle = toLower(trim(song.title)), keySafe = toLower(safe), found;
    for (int i = 0; i < n && found.empty(); i++) {
        string stem = toLower(trim(fileStem(files[i])));
        if (stem == keyComp || (!titleShared && (stem == keyTitle || stem == keySafe))) found = string(MUSIC_DIR) + files[i];
    }
    delete[] files;
    return found;
}

/*
============================================================
                        WINDOWS: MCI
============================================================
*/
#ifdef _WIN32

class AudioPlayer {
private:
    bool opened;
    bool paused;
    DWORD startTick;            // GetTickCount() when position 0 would have been
    double pausedPos;

    AudioPlayer(const AudioPlayer&);
    AudioPlayer& operator=(const AudioPlayer&);

    // The command text is UTF-8 (file names may contain accents); MCI itself gets UTF-16.
    static bool cmd(const string& c, char* out = NULL, int outLen = 0) {
        wchar_t buf[64] = { 0 };
        bool ok = mciSendStringW(utf8ToWide(c).c_str(), out ? buf : NULL, out ? 63 : 0, NULL) == 0;
        if (ok && out) {
            int i = 0;
            for (; i < outLen - 1 && buf[i]; i++) out[i] = (char)buf[i];
            out[i] = 0;
        }
        return ok;
    }

public:
    AudioPlayer() : opened(false), paused(false), startTick(0), pausedPos(0) {}

    static string findTrackFile(const Song& song, bool titleShared) { return locateTrackFile(song, titleShared); }
    static bool   isSupported() { return true; }
    static string backendName() { return "Windows MCI"; }
    static string installHint() { return "Windows audio (MCI) not available on this build: playback is simulated."; }

    bool open(const string& path) {
        close();
        string ext = fileExtension(path);
        string type = (ext == ".wav") ? "waveaudio" : "mpegvideo";
        if (!cmd("open \"" + path + "\" type " + type + " alias echoaudio")) return false;
        cmd("set echoaudio time format milliseconds");
        opened = true;
        paused = false;
        return true;
    }

    bool play() {
        if (!opened) return false;
        paused = false;
        startTick = GetTickCount();
        return cmd("play echoaudio");           // no "wait": returns immediately
    }

    void pause()  { if (opened && !paused) { pausedPos = positionSeconds(); cmd("pause echoaudio"); paused = true; } }
    void resume() {
        if (opened && paused) {
            cmd("resume echoaudio");
            startTick = GetTickCount() - (DWORD)(pausedPos * 1000.0);
            paused = false;
        }
    }
    void stop()   { close(); }

    void close() {
        if (opened) { cmd("stop echoaudio"); cmd("close echoaudio"); }
        opened = false;
        paused = false;
    }

    void setVolume(int percent) {
        if (!opened) return;
        if (percent < 0) percent = 0;
        if (percent > 100) percent = 100;
        char b[48];
        sprintf(b, "setaudio echoaudio volume to %d", percent * 10);
        cmd(b);
    }

    bool isActive() const { return opened; }

    // Playback position in seconds, or -1 when nothing is open. Taken from the clock (MCI's own
    // position is unreliable for some MP3s); MCI is still asked when the track has ended.
    double positionSeconds() const {
        if (!opened) return -1;
        if (paused) return pausedPos;
        return (double)(DWORD)(GetTickCount() - startTick) / 1000.0;
    }

    // True once the open track has finished by itself.
    bool isStopped() const {
        if (!opened || paused) return false;
        char buf[64] = "";
        if (!cmd("status echoaudio mode", buf, 63)) return false;
        return string(buf) == "stopped";
    }

    // Length of an audio file in whole seconds (0 if it cannot be read).
    static int probeLengthSeconds(const string& path) {
        int nat = nativeLengthSeconds(path);                  // exact: MCI can report a doubled length
        if (nat > 0) return nat;
        string ext = fileExtension(path);
        string type = (ext == ".wav") ? "waveaudio" : "mpegvideo";
        if (!cmd("open \"" + path + "\" type " + type + " alias echoprobe")) return 0;
        cmd("set echoprobe time format milliseconds");
        char buf[64] = "";
        bool ok = cmd("status echoprobe length", buf, 63);
        cmd("close echoprobe");
        return ok ? (int)(atof(buf) / 1000.0 + 0.5) : 0;
    }

    ~AudioPlayer() { close(); }
};

/*
==============================================================
  LINUX / macOS: external command-line player (fork + execvp)
==============================================================
*/
#else

class AudioPlayer {
private:
    enum { B_NONE = -1, B_MPV = 0, B_FFPLAY = 1, B_MPG123 = 2 };

    mutable pid_t pid;          // child process, -1 when none / already reaped
    bool   opened;
    bool   paused;
    int    volume;              // 0..100 (matches the player's default of 7/10)
    double startedAt;           // wall-clock second at which position 0 would have been
    double pausedPos;
    string path;

    AudioPlayer(const AudioPlayer&);
    AudioPlayer& operator=(const AudioPlayer&);

    static bool onPath(const char* prog) {
        const char* p = getenv("PATH");
        if (p == NULL) return false;
        string s(p);
        size_t start = 0;
        while (start <= s.size()) {
            size_t c = s.find(':', start);
            if (c == string::npos) c = s.size();
            string dir = s.substr(start, c - start);
            if (dir.empty()) dir = ".";
            if (access((dir + "/" + prog).c_str(), X_OK) == 0) return true;
            start = c + 1;
        }
        return false;
    }

    static int backend() {                              // detected once, then cached
        static int b = -2;
        if (b == -2) b = onPath("mpv") ? B_MPV : onPath("ffplay") ? B_FFPLAY : onPath("mpg123") ? B_MPG123 : B_NONE;
        return b;
    }

    static string shellQuote(const string& s) {         // 'it'\''s' style quoting
        string q = "'";
        for (size_t i = 0; i < s.size(); i++) {
            if (s[i] == '\'') q += "'\\''";
            else q += s[i];
        }
        return q + "'";
    }

    bool launch(const char* argv[], double startSec) {
        cout.flush();                                   // the child must not inherit unwritten output
        pid_t c = fork();
        if (c < 0) return false;
        if (c == 0) {                                   // child
#ifdef __linux__
            prctl(PR_SET_PDEATHSIG, SIGTERM);           // die with the parent (even on Ctrl+C)
#endif
            int dn = ::open("/dev/null", O_RDWR);         // never touch the terminal
            if (dn >= 0) { dup2(dn, 0); dup2(dn, 1); dup2(dn, 2); if (dn > 2) ::close(dn); }
            execvp(argv[0], (char* const*)argv);
            _exit(127);
        }
        pid = c;                                        // parent
        startedAt = ::nowSec() - startSec;
        usleep(150000);                                 // did exec fail / did the player die at once?
        int status;
        if (waitpid(pid, &status, WNOHANG) == pid) { pid = -1; return false; }
        return true;
    }

    // Starts the player process at 'startSec'. No shell is involved, so any file name is safe.
    bool spawn(double startSec) {
        int b = backend();
        if (b == B_NONE) return false;
        if (startSec < 0) startSec = 0;

        char vol[32], st[32], scale[32], frames[32], vopt[40], sopt[40];
        const char* argv[20];
        int k = 0;
        if (b == B_MPV) {
            sprintf(vopt, "--volume=%d", volume);
            sprintf(sopt, "--start=%.2f", startSec);
            argv[k++] = "mpv";  argv[k++] = "--no-config"; argv[k++] = "--no-video";
            argv[k++] = "--no-terminal"; argv[k++] = "--really-quiet";
            argv[k++] = vopt;   argv[k++] = sopt; argv[k++] = "--"; argv[k++] = path.c_str();
        } else if (b == B_FFPLAY) {
            sprintf(vol, "%d", volume);
            sprintf(st, "%.2f", startSec);
            argv[k++] = "ffplay"; argv[k++] = "-nodisp"; argv[k++] = "-autoexit"; argv[k++] = "-nostats";
            argv[k++] = "-loglevel"; argv[k++] = "quiet";
            argv[k++] = "-volume"; argv[k++] = vol;
            argv[k++] = "-ss"; argv[k++] = st;
            argv[k++] = "-i"; argv[k++] = path.c_str();
        } else {
            sprintf(scale, "%d", volume * 32768 / 100);   // mpg123: MP3 only, scale 0..32768
            argv[k++] = "mpg123"; argv[k++] = "-q"; argv[k++] = "-f"; argv[k++] = scale;
            if (startSec > 0.5) { sprintf(frames, "%d", (int)(startSec * 38.28)); argv[k++] = "-k"; argv[k++] = frames; }
            argv[k++] = path.c_str();
        }
        argv[k] = NULL;
        return launch(argv, startSec);
    }

    void killProcess() {
        if (pid <= 0) { pid = -1; return; }
        kill(pid, SIGCONT);                             // a stopped process cannot handle SIGTERM
        kill(pid, SIGTERM);
        int status;
        for (int i = 0; i < 40; i++) {                  // wait up to 400 ms
            if (waitpid(pid, &status, WNOHANG) == pid) { pid = -1; return; }
            usleep(10000);
        }
        kill(pid, SIGKILL);
        waitpid(pid, &status, 0);
        pid = -1;
    }

public:
    AudioPlayer() : pid(-1), opened(false), paused(false), volume(70), startedAt(0), pausedPos(0) {}

    static string findTrackFile(const Song& song, bool titleShared) { return locateTrackFile(song, titleShared); }
    static bool   isSupported() { return backend() != B_NONE; }
    static string backendName() {
        int b = backend();
        return b == B_MPV ? "mpv" : b == B_FFPLAY ? "ffplay" : b == B_MPG123 ? "mpg123 (mp3 only)" : "none";
    }
    static string installHint() {
        return "No audio player found (mpv / ffplay / mpg123): playback is simulated. "
               "Install one:  sudo apt install mpv";
    }

    bool open(const string& file) {
        close();
        if (backend() == B_NONE || !fileExists(file)) return false;
        path = file;
        opened = true;
        paused = false;
        return true;
    }

    bool play() {
        if (!opened) return false;
        paused = false;
        if (!spawn(0)) { opened = false; return false; }
        return true;
    }

    void pause() {
        if (opened && !paused && pid > 0) {
            pausedPos = positionSeconds();
            kill(pid, SIGSTOP);
            paused = true;
        }
    }

    void resume() {
        if (opened && paused && pid > 0) {
            kill(pid, SIGCONT);
            startedAt = ::nowSec() - pausedPos;
            paused = false;
        }
    }

    void stop() { close(); }

    void close() {
        killProcess();
        opened = false;
        paused = false;
    }

    // percent: 0..100. External players cannot change volume while running,
    // so the player is restarted at the current position (a tiny gap).
    void setVolume(int percent) {
        if (percent < 0) percent = 0;
        if (percent > 100) percent = 100;
        if (percent == volume) return;
        volume = percent;
        if (!opened || pid <= 0) return;
        double pos = positionSeconds();
        bool wasPaused = paused;
        killProcess();
        if (!spawn(pos)) { opened = false; return; }
        if (wasPaused) { pausedPos = pos; kill(pid, SIGSTOP); }
    }

    bool isActive() const { return opened; }

    double positionSeconds() const {
        if (!opened) return -1;
        if (paused) return pausedPos;
        double p = ::nowSec() - startedAt;
        return p < 0 ? 0 : p;
    }

    // True once the player process has ended by itself.
    bool isStopped() const {
        if (!opened || paused) return false;
        if (pid <= 0) return true;
        int status;
        if (waitpid(pid, &status, WNOHANG) == pid) { pid = -1; return true; }
        return false;
    }

    // Length in whole seconds: ffprobe if installed, otherwise estimated from the file size.
    static int probeLengthSeconds(const string& file) {
        int nat = nativeLengthSeconds(file);
        if (nat > 0) return nat;
        if (onPath("ffprobe")) {
            string c = "ffprobe -v error -show_entries format=duration -of default=noprint_wrappers=1:nokey=1 "
                       + shellQuote(file) + " 2>/dev/null";
            FILE* f = popen(c.c_str(), "r");
            if (f != NULL) {
                char buf[64] = "";
                double d = 0;
                if (fgets(buf, 63, f) != NULL) d = atof(buf);
                pclose(f);
                if (d > 0) return (int)(d + 0.5);
            }
        }
        ifstream in(file.c_str(), ios::in | ios::binary);
        if (!in) return 0;
        in.seekg(0, ios::end);
        double bytes = (double)in.tellg();
        if (bytes <= 0) return 0;
        double perSec = (fileExtension(file) == ".wav") ? 176400.0 : 16000.0;   // ~128 kbps MP3
        int s = (int)(bytes / perSec + 0.5);
        return s < 1 ? 1 : s;
    }

    ~AudioPlayer() { close(); }
};

#endif

#endif
