#ifndef ANIMATIONS_H
#define ANIMATIONS_H

/*
============================================================
      ECHO ENGINE: ANIMATIONS, THEMES AND DRAWING HELPERS
  (C++98 compatible, Windows + Linux)

  Declarations only: the code lives in animations.cpp.
  Requires colours.h (colour macros, glyphs, SLEEP_MS).
  Unused demo animations are kept in demos.h.
============================================================
*/

#include "colours.h"
#include <cstdlib>
#include <cstdio>
#include <cmath>
#include <ctime>

const double ANIM_PI = 3.14159265358979;

/*
============================================================
                       COLOUR THEMES
============================================================
*/
struct Theme {
    const char* name;
    const char* primary;     // headings, "NOW PLAYING", banners
    const char* secondary;   // artist line, spinners, messages
    const char* label;       // disc label
    const char* low;         // progress bars, equalizer bottom
    const char* mid;         // equalizer middle, highlights
    const char* high;        // equalizer peaks
};

const int THEME_COUNT = 4;

const Theme* themeTable();
int& themeIndex();
inline const Theme& T() { return themeTable()[themeIndex()]; }

/*
============================================================
                  SETUP AND SMALL HELPERS
============================================================
*/
const char* eqLevel(int i);                      // equalizer glyph 0..7

string formatTime(int sec);
string padNum(int n, int width);                 // right-aligned number, e.g. padNum(7, 3) = "  7"

// UTF-8 aware text helpers: columns are counted in characters, never in bytes
int    utf8Len(const string& s);
size_t utf8Off(const string& s, int cp);         // byte offset of character number cp
string utf8Sub(const string& s, int start, int len);
string padTo(const string& s, int w);
string makeBar(int value, int total, int width);
string scrollText(const string& text, int width, int frame);
string clipStr(const string& s, int w);

// Column widths for song lists: the title gets whatever room the window leaves.
void listWidths(int& tw, int& aw);

/*
============================================================
                     BASIC ANIMATIONS
============================================================
*/
void typewriter(const string& text, const char* colour = NULL, int delayMs = 35);
void splashAnimation();
void spinnerAnimation(const string& msg, int cycles, const char* colour = NULL);
void progressBarAnimation(const string& label, int steps = 40);
void bootSequence(int songCount = 3, int userCount = 2, int bucketCount = 101);

/*
============================================================
     SHARED DRAWING HELPERS: DISC + VERTICAL EQUALIZER
============================================================
*/
string discRow(int ry, int R, double theta);                   // one text row of the spinning disc
string eqRow(const int level[], int bars, int eqh, int k);     // row k (0 = top) of the equalizer
void spinningDiscAnimation(const string& title, const string& artist, const string& album,
                           int durationSeconds, int realSeconds = 8);

/*
============================================================
              STACK + QUEUE ANIMATIONS
  Used by the History stack and the Up Next queue screens.
============================================================
*/
void drawStack(const string stk[], int k, int cap, const string& label,
               const string& flying, int flyRow, int flyIndent, int w = 14);
string queueCell(const string& s, int w = 10);
void drawQueue(const string q[], int c, int headShrink, int tailGap,
               const string& incoming, const string& label, int w = 10);

string cellName(const string& title, int w, bool dropArtist);  // short text for a cell
int queueMaxVisible();                                         // queue cells that fit on one line

// items[0] = oldest ... items[n-1] = newest. Builds the stack by pushing the newest 5.
void stackBuildAnimation(const string items[], int n, int w = 36);
void stackPopAnimation(const string items[], int n, int w = 36);

// Shows the front of the queue; items[0] plays first. total = real queue length.
void queueBuildAnimation(const string items[], int n, int total);
void queueEnqueueAnimation(const string items[], int n, int total, const string& newItem);
void queueDequeueAnimation(const string items[], int n, int total);

#endif