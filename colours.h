#ifndef COLOURS_H
#define COLOURS_H

#include <iostream>
#include <string>
#include <cstdio>
#include <cstdlib>
#include <ctime>
// For Windows and Linux
#ifdef _WIN32
  #include <windows.h>
  #define SLEEP_MS(ms) Sleep(ms)
#else
  #include <unistd.h>
  #define SLEEP_MS(ms) usleep((ms) * 1000)
#endif

using namespace std;

#define UI_WIDTH 72                     // width of banners, headers and dividers

// ============================================================
//  ColourFonts, BackGrounds and TextStyles
// ============================================================
// Reset
#define RESET           "\033[0m"
// Standard colours 
#define BLACK           "\033[0;30m"
#define RED             "\033[0;31m"
#define GREEN           "\033[0;32m"
#define YELLOW          "\033[0;33m"
#define BLUE            "\033[0;34m"
#define MAGENTA         "\033[0;35m"
#define CYAN            "\033[0;36m"
#define WHITE           "\033[0;37m"
// Bold / Bright colours 
#define BOLD_RED        "\033[1;31m"
#define BOLD_GREEN      "\033[1;32m"
#define BOLD_YELLOW     "\033[1;33m"
#define BOLD_BLUE       "\033[1;34m"
#define BOLD_MAGENTA    "\033[1;35m"
#define BOLD_CYAN       "\033[1;36m"
#define BOLD_WHITE      "\033[1;37m"
// Text styles 
#define UNDERLINE       "\033[4m"
#define BLINK           "\033[5m"
#define DIM             "\033[2m"
// Background colours 
#define BG_BLACK        "\033[40m"
#define BG_RED          "\033[41m"
#define BG_GREEN        "\033[42m"
#define BG_YELLOW       "\033[43m"
#define BG_BLUE         "\033[44m"
#define BG_MAGENTA      "\033[45m"
#define BG_CYAN         "\033[46m"
#define BG_WHITE        "\033[47m"
// Cursor helpers 
#define CLEAR_LINE      "\r\033[K"
#define CURSOR_UP       "\033[1A"

// ============================================================
//  Glyphs, cursor control and terminal setup (shared)
//  Define ECHO_ASCII before including this file if your
//  terminal cannot display Unicode characters.
// ============================================================
#ifdef ECHO_ASCII
  #define GLYPH_FULL    "#"
  #define GLYPH_EMPTY   "-"
  #define GLYPH_CHECK   "[+]"
  #define GLYPH_CROSS   "[!]"
  static const char* SPIN_FRAMES[4] = {"|", "/", "-", "\\"};
  static const int   SPIN_COUNT = 4;
#else
  #define GLYPH_FULL    "\xE2\x96\x88"          // block
  #define GLYPH_EMPTY   "\xE2\x96\x91"          // light shade
  #define GLYPH_CHECK   "\xE2\x9C\x94"          // check mark
  #define GLYPH_CROSS   "\xE2\x9C\x98"          // cross mark
  static const char* SPIN_FRAMES[10] = {"\xE2\xA0\x8B","\xE2\xA0\x99","\xE2\xA0\xB9","\xE2\xA0\xB8","\xE2\xA0\xBC",
                                        "\xE2\xA0\xB4","\xE2\xA0\xA6","\xE2\xA0\xA7","\xE2\xA0\x87","\xE2\xA0\x8F"};
  static const int   SPIN_COUNT = 10;
#endif

#define HIDE_CURSOR "\033[?25l"
#define SHOW_CURSOR "\033[?25h"
#define ERASE_EOL   "\033[K"

// Call once at the start of main(). Enables ANSI + UTF-8 on Windows.
inline void initTerminal() {
#ifdef _WIN32
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);                                          // typed text arrives as UTF-8 too
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    if (GetConsoleMode(h, &mode)) SetConsoleMode(h, mode | 0x0004); // VIRTUAL_TERMINAL_PROCESSING
#endif
    srand((unsigned)time(NULL));
}

inline string repeatStr(const string& s, int n) {
    string out;
    for (int i = 0; i < n; i++) out += s;
    return out;
}

// ============================================================
//  Global Animations Declarations
// ============================================================

void clearLine();
// cycles  = roughly how many 1/4-second beats the animation lasts (same meaning as before)
// subject = optional text, e.g. "songs.txt"      success = false ends with a failure message
void processingAnimation(int cycles, const string& subject = "", bool success = true);
void updatingAnimation(int cycles, const string& subject = "", bool success = true);
void savingAnimation(int cycles, const string& subject = "", bool success = true);
void loadingAnimation(int cycles, const string& subject = "", bool success = true);

// ============================================================
//  Global Function Declarations
// ============================================================

void printBanner(const string& title, const string& subtitle = "");
void printSuccess(const string& msg);
void printError(const string& msg);
void printWarn(const string& msg);
void printInfo(const string& msg);
void printSectionHeader(const string& title);
void printDivider(char c = '-', int width = UI_WIDTH);

// ============================================================
//  Inline Animation definitions
// ============================================================

// Clear current terminal line 
inline void clearLine() {
    cout << CLEAR_LINE << flush;
}

// ---- helpers used by the four animations below ----

// Fixed-width dots: "   ", ".  ", ".. ", "..." (no jitter when the width changes)
inline string dotsText(int beat) {
    int d = beat % 4;
    return string(d, '.') + string(3 - d, ' ');
}

inline string simpleBar(int value, int total, int width, const char* colour) {
    int filled = (total > 0) ? (value * width) / total : 0;
    if (filled > width) filled = width;
    if (filled < 0) filled = 0;
    return string(colour) + repeatStr(GLYPH_FULL, filled) + RESET + DIM +
           repeatStr(GLYPH_EMPTY, width - filled) + RESET;
}

// Final result line: green tick + elapsed time, or red cross on failure
inline void finishAnimation(bool ok, const string& okText, const string& failText, int frames, int delayMs) {
    char buf[32];
    sprintf(buf, " (%.1fs)", frames * delayMs / 1000.0);
    cout << CLEAR_LINE;
    if (ok) cout << BOLD_GREEN << "  " << GLYPH_CHECK << " " << okText   << RESET << DIM << buf << RESET << "\n";
    else    cout << BOLD_RED   << "  " << GLYPH_CROSS << " " << failText << RESET << DIM << buf << RESET << "\n";
    cout << SHOW_CURSOR;
}

// "Processing" Animation: spinner + fixed-width dots
inline void processingAnimation(int cycles, const string& subject, bool success) {
    int frames = cycles * 4;
    string what = subject.empty() ? string("") : " " + subject;
    cout << HIDE_CURSOR;
    for (int i = 0; i < frames; i++) {
        cout << CLEAR_LINE << BOLD_CYAN << "  " << SPIN_FRAMES[i % SPIN_COUNT]
             << " Processing" << what << dotsText(i / 4) << RESET << flush;
        SLEEP_MS(65);
    }
    finishAnimation(success, "Done!", "Processing failed" + (subject.empty() ? string("") : ": " + subject), frames, 65);
}

// "Updating" Animation: a bright wave travels across the word
inline void updatingAnimation(int cycles, const string& subject, bool success) {
    const string word = "Updating";
    int len = (int)word.size();
    int frames = cycles * 4;
    string what = subject.empty() ? string("") : " " + subject;
    cout << HIDE_CURSOR;
    for (int i = 0; i < frames; i++) {
        int p = i % (len + 3);
        string w;
        for (int j = 0; j < len; j++) {
            if (j == p)          w += string(BOLD_WHITE) + word[j];
            else if (j == p - 1) w += string(BOLD_YELLOW) + word[j];
            else                 w += string(YELLOW) + word[j];
        }
        cout << CLEAR_LINE << "  " << w << RESET << what << flush;
        SLEEP_MS(65);
    }
    finishAnimation(success, "Done!", "Update failed" + (subject.empty() ? string("") : ": " + subject), frames, 65);
}

// "Saving" Animation: progress bar with percentage (slow start, slow end)
inline void savingAnimation(int cycles, const string& subject, bool success) {
    int frames = cycles * 4;
    if (frames < 1) frames = 1;
    int stopAt = success ? frames : (frames * 65) / 100;      // a failed save stops part-way
    cout << HIDE_CURSOR;
    for (int i = 0; i <= stopAt; i++) {
        double p = (double)i / frames;
        int v = (int)(p * p * (3 - 2 * p) * 1000);             // smoothstep easing
        cout << CLEAR_LINE << BOLD_MAGENTA << "  Saving" << RESET << (subject.empty() ? "" : " ") << subject
             << " [" << simpleBar(v, 1000, 24, BOLD_MAGENTA) << "] " << (v / 10) << "%" << flush;
        SLEEP_MS(55);
    }
    finishAnimation(success, "Saved!", "Save failed" + (subject.empty() ? string("") : ": " + subject), stopAt, 55);
}

// "Loading" Animation: spinner + progress bar + percentage
inline void loadingAnimation(int cycles, const string& subject, bool success) {
    int frames = cycles * 4;
    if (frames < 1) frames = 1;
    int stopAt = success ? frames : (frames * 70) / 100;      // a failed load stops part-way
    cout << HIDE_CURSOR;
    for (int i = 0; i <= stopAt; i++) {
        int pct = (i * 100) / frames;
        cout << CLEAR_LINE << BOLD_BLUE << "  " << SPIN_FRAMES[i % SPIN_COUNT] << " Loading" << RESET
             << (subject.empty() ? "" : " ") << subject
             << " [" << simpleBar(i, frames, 20, BOLD_BLUE) << "] " << pct << "%" << flush;
        SLEEP_MS(50);
    }
    finishAnimation(success, "Ready!", "Load failed" + (subject.empty() ? string("") : ": " + subject), stopAt, 50);
}

// ============================================================
//  Inline Function Definitions
// ============================================================

// Full-width divider line 
inline void printDivider(char c, int width) {
    cout << BOLD_BLUE << string(width, c) << RESET << "\n";
}

// Coloured section header 
inline void printSectionHeader(const string& title) {
    const int W = UI_WIDTH;
    cout << "\n" << BOLD_BLUE << string(W, '-') << RESET << "\n";
    int pad = (W - (int)title.size() - 2) / 2;
    cout << BOLD_CYAN
         << string(pad > 0 ? pad : 0, ' ')
         << "  " << title << "\n"
         << RESET;
    cout << BOLD_BLUE << string(W, '-') << RESET << "\n";
}

// Decorative banner (used for main title screens) 
inline void printBanner(const string& title, const string& subtitle) {
    const int W = UI_WIDTH;
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

// Coloured status message helpers 
inline void printSuccess(const string& msg) { cout << BOLD_GREEN << "  [+] " << msg << RESET << "\n"; }
inline void printError(const string& msg) { cout << BOLD_RED   << "  [!] " << msg << RESET << "\n"; }
inline void printWarn(const string& msg) { cout << BOLD_YELLOW << "  [~] " << msg << RESET << "\n"; }
inline void printInfo(const string& msg) { cout << BOLD_CYAN  << "  [i] " << msg << RESET << "\n"; }

#endif
