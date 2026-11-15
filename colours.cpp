#include "colours.h"
#include <cstring>
#ifdef _WIN32
  #include <conio.h>
#else
  #include <termios.h>
  #include <sys/select.h>
  #include <sys/ioctl.h>
  #include <sys/time.h>
  #include <unistd.h>
#endif

bool g_ansiEnabled    = true;
bool g_unicodeEnabled = true;
bool g_scrollClearOk  = true;
bool g_stdoutIsTty    = true;

/*
============================================================
        Runtime ANSI / Unicode detection (initTerminal)
============================================================
*/
#ifdef _WIN32
struct EchoOsVersion { DWORD size, major, minor, build, platform; WCHAR csd[128]; };
typedef LONG (WINAPI *RtlGetVersionFn)(EchoOsVersion*);

// ESC[3J clears the scrollback in Windows Terminal and in the console host from Windows 10 1809 on.
static bool windowsClearsScrollback() {
    if (getenv("WT_SESSION") != NULL) return true;
    HMODULE nt = GetModuleHandleA("ntdll.dll");
    if (nt == NULL) return false;
    RtlGetVersionFn fn = (RtlGetVersionFn)GetProcAddress(nt, "RtlGetVersion");
    if (fn == NULL) return false;
    EchoOsVersion v;
    memset(&v, 0, sizeof(v));
    v.size = sizeof(v);
    if (fn(&v) != 0) return false;
    return v.build >= 17763;
}
#endif

void initTerminal() {
    ios::sync_with_stdio(false);                 // each player frame is built in one string and written once

    bool ansi = true, uni = true;
#ifdef ECHO_ASCII
    uni = false;
#endif
#ifdef _WIN32
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    g_stdoutIsTty = (h != INVALID_HANDLE_VALUE && GetConsoleMode(h, &mode) != 0);
    if (!g_stdoutIsTty) ansi = false;
    else if (SetConsoleMode(h, mode | 0x0004) == 0) ansi = false;    // ENABLE_VIRTUAL_TERMINAL_PROCESSING
    if (!SetConsoleOutputCP(65001)) uni = false;
    SetConsoleCP(65001);                                              // typed text arrives as UTF-8 too
    g_scrollClearOk = ansi && windowsClearsScrollback();
#else
    g_stdoutIsTty = (isatty(STDOUT_FILENO) != 0);
    const char* term = getenv("TERM");
    if (!g_stdoutIsTty || (term != NULL && strcmp(term, "dumb") == 0)) ansi = false;
    g_scrollClearOk = ansi;
#endif
    g_ansiEnabled = ansi;
    g_unicodeEnabled = uni && ansi;              // no ANSI means plain ASCII too
    srand((unsigned)time(NULL));
}

/*
============================================================
                  Small terminal helpers
============================================================
*/
const char* clearLineSeq() {
    static string plain = "\r";
    static string wipe;
    if (g_ansiEnabled) return "\r\033[K";
    if (!g_stdoutIsTty) return plain.c_str();
    if (wipe.empty()) wipe = "\r" + string(78, ' ') + "\r";
    return wipe.c_str();
}

const char* spinFrame(int i) {
    static const char* uni[10] = {"\xE2\xA0\x8B","\xE2\xA0\x99","\xE2\xA0\xB9","\xE2\xA0\xB8","\xE2\xA0\xBC",
                                  "\xE2\xA0\xB4","\xE2\xA0\xA6","\xE2\xA0\xA7","\xE2\xA0\x87","\xE2\xA0\x8F"};
    static const char* asc[4] = {"|", "/", "-", "\\"};
    return g_unicodeEnabled ? uni[i % 10] : asc[i % 4];
}

int spinCount() { return g_unicodeEnabled ? 10 : 4; }

string repeatStr(const string& s, int n) {
    string out;
    for (int i = 0; i < n; i++) out += s;
    return out;
}

// Current terminal size in characters (falls back to 80x24).
void getTermSize(int& cols, int& rows) {
    cols = 80; rows = 24;
#ifdef _WIN32
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi)) {
        cols = csbi.srWindow.Right  - csbi.srWindow.Left + 1;
        rows = csbi.srWindow.Bottom - csbi.srWindow.Top  + 1;
    }
#else
    struct winsize w;
    if (ioctl(1, TIOCGWINSZ, &w) == 0 && w.ws_col > 0 && w.ws_row > 0) { cols = w.ws_col; rows = w.ws_row; }
#endif
}

// Banner width: one column less than the window (so a full line never wraps), a multiple of 4, capped.
int uiWidth() {
    int cols, rows;
    getTermSize(cols, rows);
    int w = ((cols - 1) / UI_WIDTH_STEP) * UI_WIDTH_STEP;
    if (w > UI_WIDTH) w = UI_WIDTH;
    if (w < UI_MIN_WIDTH) w = UI_MIN_WIDTH;
    return w;
}

void moveUp(int n) {
    if (n <= 0) return;
    if (g_ansiEnabled) { cout << "\033[" << n << "A"; return; }
#ifdef _WIN32
    cout.flush();
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (GetConsoleScreenBufferInfo(h, &csbi)) {
        COORD c = csbi.dwCursorPosition;
        c.Y = (SHORT)(c.Y >= n ? c.Y - n : 0);
        SetConsoleCursorPosition(h, c);
    }
#endif
}

void consoleCursorHome() {
#ifdef _WIN32
    cout.flush();
    COORD c; c.X = 0; c.Y = 0;
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    if (GetConsoleScreenBufferInfo(h, &csbi)) c.Y = csbi.srWindow.Top;
    SetConsoleCursorPosition(h, c);
#endif
}

double nowSec() {
#ifdef _WIN32
    static LARGE_INTEGER freq;
    static bool have = false;
    if (!have) { QueryPerformanceFrequency(&freq); have = true; }
    LARGE_INTEGER t;
    QueryPerformanceCounter(&t);
    return (double)t.QuadPart / (double)freq.QuadPart;
#elif defined(CLOCK_MONOTONIC)
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + ts.tv_nsec / 1e9;
#else
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (double)tv.tv_sec + tv.tv_usec / 1e6;
#endif
}

/*
============================================================
       KEYBOARD INPUT (non-blocking, Windows + Linux)
============================================================
*/
#ifdef _WIN32
void rawModeOn()      { cout.flush(); }
void rawModeOff()     {}
bool keyAvailable()   { return _kbhit() != 0; }
int  readKey() {
    int c = _getch();
    if (c == 0 || c == 224) { _getch(); return -1; }   // ignore arrow / function keys
    return c;
}
#else
static struct termios& savedTerm() { static struct termios t; return t; }
static bool& rawActive()           { static bool a = false; return a; }
void rawModeOn() {
    cout.flush();
    struct termios t;
    if (tcgetattr(0, &t) == 0) {
        savedTerm() = t;
        t.c_lflag &= ~(ICANON | ECHO);
        tcsetattr(0, TCSANOW, &t);
        rawActive() = true;
    }
}
void rawModeOff() {
    if (rawActive()) { tcsetattr(0, TCSANOW, &savedTerm()); rawActive() = false; }
}
bool keyAvailable() {
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(0, &fds);
    struct timeval tv;
    tv.tv_sec = 0; tv.tv_usec = 0;
    return select(1, &fds, NULL, NULL, &tv) > 0;
}
int readKey() {
    unsigned char c;
    if (read(0, &c, 1) == 1) return c;
    return -1;
}
#endif

/*
============================================================
                Global Function Definitions
============================================================
*/
void clearLine() {
    cout << CLEAR_LINE << flush;
}

void printDivider(char c, int width) {
    if (width <= 0) width = uiWidth();
    cout << BOLD_BLUE << string(width, c) << RESET << "\n";
}

void printSectionHeader(const string& title) {
    const int W = uiWidth();
    cout << "\n" << BOLD_BLUE << string(W, '-') << RESET << "\n";
    int pad = (W - (int)title.size() - 2) / 2;
    cout << BOLD_CYAN
         << string(pad > 0 ? pad : 0, ' ')
         << "  " << title << "\n"
         << RESET;
    cout << BOLD_BLUE << string(W, '-') << RESET << "\n";
}

void printBanner(const string& title, const string& subtitle) {
    const int W = uiWidth();
    cout << "\n";
    cout << BOLD_YELLOW << string(W, '=') << RESET << "\n";
    int pad = (W - (int)title.size()) / 2;
    cout << BOLD_WHITE
         << string(pad > 0 ? pad : 0, ' ')
         << title << "\n"
         << RESET;
    if (!subtitle.empty()) {
        int pad2 = (W - (int)subtitle.size()) / 2;
        cout << DIM << CYAN
             << string(pad2 > 0 ? pad2 : 0, ' ')
             << subtitle << "\n"
             << RESET;
    }
    cout << BOLD_YELLOW << string(W, '=') << RESET << "\n\n";
}

void printSuccess(const string& msg) { cout << BOLD_GREEN << "  [+] " << msg << RESET << "\n"; }
void printError(const string& msg)   { cout << BOLD_RED   << "  [!] " << msg << RESET << "\n"; }
void printWarn(const string& msg)    { cout << BOLD_YELLOW << "  [~] " << msg << RESET << "\n"; }
void printInfo(const string& msg)    { cout << BOLD_CYAN  << "  [i] " << msg << RESET << "\n"; }
