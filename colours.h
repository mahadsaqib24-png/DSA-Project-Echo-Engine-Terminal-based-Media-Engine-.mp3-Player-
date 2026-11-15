#ifndef COLOURS_H
#define COLOURS_H

#include <iostream>
#include <string>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include "Constants.h"
#ifdef _WIN32
  #include <windows.h>
  #define SLEEP_MS(ms) (std::cout.flush(), Sleep(ms))
#else
  #include <unistd.h>
  #define SLEEP_MS(ms) (std::cout.flush(), usleep((ms) * 1000))
#endif

using namespace std;

/*
=================================================================
        Runtime terminal capabilities (set by initTerminal)
  g_ansiEnabled    : colours and cursor codes are sent
  g_unicodeEnabled : Unicode glyphs are used (else plain ASCII)
  g_scrollClearOk  : ESC[3J really wipes the scrollback
  g_stdoutIsTty    : stdout is an interactive console
=================================================================
*/
extern bool g_ansiEnabled;
extern bool g_unicodeEnabled;
extern bool g_scrollClearOk;
extern bool g_stdoutIsTty;

/*
============================================================
          ColourFonts, BackGrounds and TextStyles
  Each macro resolves to "" when ANSI is not available.
============================================================
*/
#define ECHO_SEQ(code)  (g_ansiEnabled ? code : "")

// Reset
#define RESET           ECHO_SEQ("\033[0m")
// Standard colours
#define BLACK           ECHO_SEQ("\033[0;30m")
#define RED             ECHO_SEQ("\033[0;31m")
#define GREEN           ECHO_SEQ("\033[0;32m")
#define YELLOW          ECHO_SEQ("\033[0;33m")
#define BLUE            ECHO_SEQ("\033[0;34m")
#define MAGENTA         ECHO_SEQ("\033[0;35m")
#define CYAN            ECHO_SEQ("\033[0;36m")
#define WHITE           ECHO_SEQ("\033[0;37m")
// Bold / Bright colours
#define BOLD_RED        ECHO_SEQ("\033[1;31m")
#define BOLD_GREEN      ECHO_SEQ("\033[1;32m")
#define BOLD_YELLOW     ECHO_SEQ("\033[1;33m")
#define BOLD_BLUE       ECHO_SEQ("\033[1;34m")
#define BOLD_MAGENTA    ECHO_SEQ("\033[1;35m")
#define BOLD_CYAN       ECHO_SEQ("\033[1;36m")
#define BOLD_WHITE      ECHO_SEQ("\033[1;37m")
// Text styles
#define UNDERLINE       ECHO_SEQ("\033[4m")
#define BLINK           ECHO_SEQ("\033[5m")
#define DIM             ECHO_SEQ("\033[2m")
// Background colours
#define BG_BLACK        ECHO_SEQ("\033[40m")
#define BG_RED          ECHO_SEQ("\033[41m")
#define BG_GREEN        ECHO_SEQ("\033[42m")
#define BG_YELLOW       ECHO_SEQ("\033[43m")
#define BG_BLUE         ECHO_SEQ("\033[44m")
#define BG_MAGENTA      ECHO_SEQ("\033[45m")
#define BG_CYAN         ECHO_SEQ("\033[46m")
#define BG_WHITE        ECHO_SEQ("\033[47m")
// Cursor helpers
#define CLEAR_LINE      clearLineSeq()                  // return to column 0 and wipe the line
#define HIDE_CURSOR     ECHO_SEQ("\033[?25l")
#define SHOW_CURSOR     ECHO_SEQ("\033[?25h")
#define ERASE_EOL       ECHO_SEQ("\033[K")              // erase from cursor to end of line
#define ERASE_BELOW     ECHO_SEQ("\033[J")              // erase from cursor to end of screen
#define CLEAR_SCREEN    ECHO_SEQ("\033[2J")             // erase the whole screen
#define CURSOR_HOME     ECHO_SEQ("\033[H")              // move cursor to top-left
#define ALT_SCREEN_ON   ECHO_SEQ("\033[?1049h")        // switch to the alternate screen
#define ALT_SCREEN_OFF  ECHO_SEQ("\033[?1049l")        // back to the normal screen

/*
============================================================
     Glyphs: Unicode when g_unicodeEnabled, else plain ASCII
============================================================
*/
#define ECHO_GLYPH(uni, ascii)  (g_unicodeEnabled ? uni : ascii)

#define GLYPH_FULL    ECHO_GLYPH("\xE2\x96\x88", "#")          // block
#define GLYPH_EMPTY   ECHO_GLYPH("\xE2\x96\x91", "-")          // light shade
#define GLYPH_CHECK   ECHO_GLYPH("\xE2\x9C\x94", "[+]")        // check mark
#define GLYPH_CROSS   ECHO_GLYPH("\xE2\x9C\x98", "[!]")        // cross mark
#define GLYPH_PLAY    ECHO_GLYPH("\xE2\x96\xB6", ">")          // play triangle
#define GLYPH_NOTE    ECHO_GLYPH("\xE2\x99\xAA", "*")          // music note
#define GLYPH_PTR     ECHO_GLYPH("\xE2\x96\xBC", "v")          // down triangle
#define GLYPH_ARROW   ECHO_GLYPH("\xE2\x86\x92", "->")         // right arrow
#define GLYPH_BIARROW ECHO_GLYPH("\xE2\x87\x84", "<->")        // two-way arrows

const char* clearLineSeq();
const char* spinFrame(int i);   // braille spinner, or | / - \ in ASCII mode
int         spinCount();

/*
============================================================
              Terminal setup, size and keyboard
============================================================
*/
// Call once at the start of main(). Detects ANSI / UTF-8 support at runtime.
void initTerminal();
void getTermSize(int& cols, int& rows);     // falls back to 80x24
int  uiWidth();                             // banner width: terminal width, multiple of 4, capped at UI_WIDTH
void moveUp(int n);
void consoleCursorHome();                   // top-left without ANSI (legacy Windows consoles)
double nowSec();                            // sub-second monotonic clock

void rawModeOn();
void rawModeOff();
bool keyAvailable();
int  readKey();                             // -1 for arrow / function keys or end of input

string repeatStr(const string& s, int n);

/*
============================================================
               Global Animations Declarations
============================================================
*/

void clearLine();
// cycles  = roughly how many 1/4-second beats the animation lasts (same meaning as before)
// subject = optional text, e.g. "songs.txt"      success = false ends with a failure message
void processingAnimation(int cycles, const string& subject = "", bool success = true);
void updatingAnimation(int cycles, const string& subject = "", bool success = true);
void savingAnimation(int cycles, const string& subject = "", bool success = true);
void loadingAnimation(int cycles, const string& subject = "", bool success = true);

/*
============================================================
                Global Function Declarations
============================================================
*/

void printBanner(const string& title, const string& subtitle = "");
void printSuccess(const string& msg);
void printError(const string& msg);
void printWarn(const string& msg);
void printInfo(const string& msg);
void printSectionHeader(const string& title);
void printDivider(char c = '-', int width = 0);   // width 0 = uiWidth()

#endif
