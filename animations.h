#ifndef ANIMATIONS_H
#define ANIMATIONS_H

/*
============================================================
      ECHO ENGINE: ANIMATIONS, THEMES AND LIVE PLAYER
  (C++98 compatible, Windows + Linux)

  Requires colours.h (colour macros + SLEEP_MS).
  Define ECHO_ASCII before including this file if your
  terminal cannot display Unicode block characters.
============================================================
*/

#include "colours.h"
#include <cstdlib>
#include <cstdio>
#include <cmath>
#include <ctime>

#ifdef _WIN32
  #include <conio.h>
#else
  #include <termios.h>
  #include <sys/select.h>
  #include <sys/ioctl.h>
  #include <unistd.h>
#endif

/*
==============================================================
         Glyphs (Unicode, or plain ASCII fallback)
  Shared ones (FULL, EMPTY, CHECK, spinner) live in colours.h
==============================================================
*/
#ifdef ECHO_ASCII
  #define GLYPH_PLAY    ">"
  #define GLYPH_NOTE    "*"
  #define GLYPH_PTR     "v"
  #define GLYPH_ARROW   "->"
  #define GLYPH_BIARROW "<->"
  static const char* EQ_LEVELS[8]   = {"_", ".", "-", "=", "+", "*", "#", "#"};
#else
  #define GLYPH_PLAY    "\xE2\x96\xB6"          // play triangle
  #define GLYPH_NOTE    "\xE2\x99\xAA"          // music note
  #define GLYPH_PTR     "\xE2\x96\xBC"          // down triangle
  #define GLYPH_ARROW   "\xE2\x86\x92"          // right arrow
  #define GLYPH_BIARROW "\xE2\x87\x84"          // two-way arrows
  static const char* EQ_LEVELS[8] = {"\xE2\x96\x81","\xE2\x96\x82","\xE2\x96\x83","\xE2\x96\x84",
                                     "\xE2\x96\x85","\xE2\x96\x86","\xE2\x96\x87","\xE2\x96\x88"};
#endif

static const double ANIM_PI = 3.14159265358979;

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

inline const Theme* themeTable() {
    static const Theme t[THEME_COUNT] = {
        {"Neon",         BOLD_MAGENTA, BOLD_CYAN, MAGENTA, BOLD_GREEN,  BOLD_YELLOW, BOLD_RED},
        {"Ocean",        BOLD_BLUE,    BOLD_CYAN, BLUE,    BOLD_CYAN,   BOLD_BLUE,   BOLD_WHITE},
        {"Retro Green",  BOLD_GREEN,   GREEN,     GREEN,   GREEN,       BOLD_GREEN,  BOLD_WHITE},
        {"Sunset",       BOLD_YELLOW,  BOLD_MAGENTA, RED,  BOLD_YELLOW, BOLD_RED,    BOLD_MAGENTA}
    };
    return t;
}
inline int& themeIndex() { static int i = 0; return i; }
inline const Theme& T() { return themeTable()[themeIndex()]; }

/*
============================================================
                  SETUP AND SMALL HELPERS
============================================================
*/

inline void moveUp(int n) { if (n > 0) cout << ANSI_CSI << n << "A"; }

inline string formatTime(int sec) {
    char buf[32];
    sprintf(buf, "%02d:%02d", sec / 60, sec % 60);
    return buf;
}

// UTF-8 aware text helpers: columns are counted in characters, never in bytes
inline int utf8Len(const string& s) {
    int n = 0;
    for (size_t i = 0; i < s.size(); i++)
        if (((unsigned char)s[i] & 0xC0) != 0x80) n++;
    return n;
}

inline size_t utf8Off(const string& s, int cp) {          // byte offset of character number cp
    int n = 0;
    for (size_t i = 0; i < s.size(); i++)
        if (((unsigned char)s[i] & 0xC0) != 0x80) { if (n == cp) return i; n++; }
    return s.size();
}

inline string utf8Sub(const string& s, int start, int len) {
    size_t a = utf8Off(s, start), b = utf8Off(s, start + len);
    return s.substr(a, b - a);
}

inline string padTo(string s, int w) {
    s = utf8Sub(s, 0, w);
    return s + string(w - utf8Len(s), ' ');
}

inline string makeBar(int value, int total, int width) {
    int filled = (total > 0) ? (value * width) / total : 0;
    if (filled > width) filled = width;
    if (filled < 0) filled = 0;
    return string(T().low) + repeatStr(GLYPH_FULL, filled) + RESET + DIM +
           repeatStr(GLYPH_EMPTY, width - filled) + RESET;
}

/*
============================================================
       KEYBOARD INPUT (non-blocking, Windows + Linux)
============================================================
*/
#ifdef _WIN32
inline void rawModeOn()      {}
inline void rawModeOff()     {}
inline bool keyAvailable()   { return _kbhit() != 0; }
inline int  readKey() {
    int c = _getch();
    if (c == 0 || c == 224) { _getch(); return -1; }   // ignore arrow / function keys
    return c;
}
#else
inline struct termios& savedTerm() { static struct termios t; return t; }
inline bool& rawActive()           { static bool a = false; return a; }
inline void rawModeOn() {
    struct termios t;
    if (tcgetattr(0, &t) == 0) {
        savedTerm() = t;
        t.c_lflag &= ~(ICANON | ECHO);
        tcsetattr(0, TCSANOW, &t);
        rawActive() = true;
    }
}
inline void rawModeOff() {
    if (rawActive()) { tcsetattr(0, TCSANOW, &savedTerm()); rawActive() = false; }
}
inline bool keyAvailable() {
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(0, &fds);
    struct timeval tv;
    tv.tv_sec = 0; tv.tv_usec = 0;
    return select(1, &fds, NULL, NULL, &tv) > 0;
}
inline int readKey() {
    unsigned char c;
    if (read(0, &c, 1) == 1) return c;
    return -1;
}
#endif

// Current terminal size in characters (falls back to 80x24).
inline void getTermSize(int& cols, int& rows) {
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

// Column widths for song lists: the title gets whatever room the window leaves.
inline void listWidths(int& tw, int& aw) {
    int cols, rows;
    getTermSize(cols, rows);
    aw = (cols >= 110) ? 24 : 18;
    tw = cols - 1 - 16 - aw;
    if (tw > 64) tw = 64;
    if (tw < 16) tw = 16;
}

/*
============================================================
                       1. TYPEWRITER
============================================================
*/
inline void typewriter(const string& text, const char* colour = NULL, int delayMs = 35) {
    cout << (colour ? colour : BOLD_WHITE);
    for (size_t i = 0; i < text.size(); i++) { cout << text[i] << flush; SLEEP_MS(delayMs); }
    cout << RESET;
}

/*
============================================================
                      2. SPLASH SCREEN
============================================================
*/
inline void splashAnimation() {
    const int W = UI_WIDTH;
    cout << HIDE_CURSOR << "\n";
    for (int half = 0; half <= W / 2; half += 2) {
        cout << CLEAR_LINE << T().primary << string((W / 2) - half, ' ')
             << string(half * 2, '=') << RESET << flush;
        SLEEP_MS(25);
    }
    cout << "\n" << string((W - 21) / 2, ' ');
    typewriter("E C H O   E N G I N E", BOLD_WHITE, 60);
    cout << "\n" << string((W - 36) / 2, ' ');
    typewriter("A Terminal-Based Media Engine in C++", T().secondary, 20);
    cout << "\n" << T().primary << string(W, '=') << RESET << "\n\n" << SHOW_CURSOR;
}

/*
============================================================
                         3. SPINNER
============================================================
*/
inline void spinnerAnimation(const string& msg, int cycles, const char* colour = NULL) {
    cout << HIDE_CURSOR;
    for (int i = 0; i < cycles; i++) {
        cout << CLEAR_LINE << (colour ? colour : T().secondary) << "  "
             << SPIN_FRAMES[i % SPIN_COUNT] << " " << msg << RESET << flush;
        SLEEP_MS(90);
    }
    cout << CLEAR_LINE << SHOW_CURSOR;
}

/*
============================================================
                      4. LOGIN RESULT
============================================================
*/
inline void loginAnimation(const string& user, bool ok) {
    spinnerAnimation("Authenticating " + user + "...", 18);
    if (!ok) { printError("Invalid username or password."); return; }
    cout << T().low << "  " << GLYPH_CHECK << " Access granted: " << RESET
         << BOLD_WHITE << user << RESET << "\n";
}

/*
============================================================
           5. PROGRESS BAR (loading / uploading)
============================================================
*/
inline void progressBarAnimation(const string& label, int steps = 40) {
    cout << HIDE_CURSOR;
    for (int i = 0; i <= steps; i++) {
        cout << CLEAR_LINE << "  " << label << " [" << makeBar(i, steps, 30) << "] "
             << (i * 100 / steps) << "%" << flush;
        SLEEP_MS(40 + (rand() % 40));
    }
    cout << "\n" << SHOW_CURSOR;
}

/*
============================================================
      6. NOW PLAYING (horizontal equalizer + progress)
============================================================
*/
inline void nowPlayingAnimation(const string& title, const string& artist, const string& album,
                                int durationSeconds, int realSeconds = 6) {
    const int BARS = 16, LINES = 5;
    int level[BARS];
    for (int i = 0; i < BARS; i++) level[i] = rand() % 8;
    int frames = realSeconds * 8;
    cout << HIDE_CURSOR;
    for (int f = 0; f <= frames; f++) {
        if (f > 0) moveUp(LINES);
        int elapsed = (durationSeconds * f) / frames;
        string eq;
        for (int i = 0; i < BARS; i++) {
            level[i] += (rand() % 5) - 2;
            if (level[i] < 0) level[i] = 0;
            if (level[i] > 7) level[i] = 7;
            const char* col = level[i] < 3 ? T().low : (level[i] < 6 ? T().mid : T().high);
            eq += string(col) + EQ_LEVELS[level[i]] + EQ_LEVELS[level[i]] + " ";
        }
        cout << CLEAR_LINE << T().primary << "  " << GLYPH_NOTE << " NOW PLAYING" << RESET << "\n";
        cout << CLEAR_LINE << BOLD_WHITE << "  " << title << RESET << "\n";
        cout << CLEAR_LINE << DIM << T().secondary << "  " << artist << "  |  " << album << RESET << "\n";
        cout << CLEAR_LINE << "  " << eq << RESET << "\n";
        cout << CLEAR_LINE << "  " << T().low << GLYPH_PLAY << RESET << " " << formatTime(elapsed)
             << " " << makeBar(elapsed, durationSeconds, 30) << " " << formatTime(durationSeconds) << "\n" << flush;
        SLEEP_MS(125);
    }
    cout << SHOW_CURSOR;
}

/*
============================================================
                         7. MARQUEE
============================================================
*/
inline void marqueeAnimation(const string& text, int width = 24, int steps = 60) {
    string loop = text + "   *   ";
    string doubled = loop + loop;
    cout << HIDE_CURSOR;
    for (int i = 0; i < steps; i++) {
        int start = i % (int)loop.size();
        cout << CLEAR_LINE << "  " << GLYPH_NOTE << " " << T().secondary << doubled.substr(start, width) << RESET << flush;
        SLEEP_MS(110);
    }
    cout << CLEAR_LINE << SHOW_CURSOR;
}

/*
============================================================
                    8. TRACK TRANSITION
============================================================
*/
inline void trackTransition(const string& from, const string& to, bool forward, int W = 34) {
    string a = padTo(from, W), b = padTo(to, W);
    string strip = forward ? a + b : b + a;
    cout << HIDE_CURSOR;
    for (int step = 0; step <= W; step++) {
        int offset = forward ? step : W - step;
        cout << CLEAR_LINE << "  " << (forward ? "" : GLYPH_BIARROW " ") << BOLD_WHITE
             << strip.substr(offset, W) << RESET << flush;
        SLEEP_MS(18);
    }
    cout << "\n" << SHOW_CURSOR;
}

/*
============================================================
               9. CIRCULAR LIST POINTER WALK
============================================================
*/
inline void circularListAnimation(const string names[], int n, int steps, bool forward = true) {
    int cur = forward ? 0 : n - 1;
    cout << HIDE_CURSOR;
    for (int s = 0; s <= steps; s++) {
        if (s > 0) moveUp(3);
        string ptrLine = "  ", nodeLine = "  ";
        for (int i = 0; i < n; i++) {
            string cell = "[" + names[i] + "]";
            if (i == cur) {
                ptrLine  += string(T().mid) + string(cell.size() / 2, ' ') + GLYPH_PTR +
                            string(cell.size() - cell.size() / 2 - 1, ' ') + RESET;
                nodeLine += string(T().low) + cell + RESET;
            } else {
                ptrLine  += string(cell.size(), ' ');
                nodeLine += string(DIM) + cell + RESET;
            }
            if (i < n - 1) { ptrLine += "    "; nodeLine += string(" ") + GLYPH_BIARROW + " "; }
        }
        cout << CLEAR_LINE << ptrLine << "\n" << CLEAR_LINE << nodeLine << "\n";
        bool wrapped = (s > 0) && ((forward && cur == 0) || (!forward && cur == n - 1));
        cout << CLEAR_LINE << (wrapped ? string(T().primary) + "  wrapped around (tail <-> head)" + RESET : string(""))
             << "\n" << flush;
        SLEEP_MS(550);
        cur = forward ? (cur + 1) % n : (cur - 1 + n) % n;
    }
    cout << SHOW_CURSOR;
}

/*
============================================================
                 10. HASH LOOKUP VISUALISER
============================================================
*/
inline void hashLookupAnimation(const string& title, int tableSize, int chainLength, int foundAt) {
    int h = 0;
    for (size_t k = 0; k < title.size(); k++) h = (h * 31 + title[k]) % tableSize;
    cout << "  " << DIM << "hash(\"" << title << "\") = " << RESET;
    SLEEP_MS(500); cout << T().mid << h << RESET << "\n" << flush;
    SLEEP_MS(400);
    int shown = tableSize > 12 ? 12 : tableSize;
    int target = h % shown;
    cout << "  ";
    for (int i = 0; i < shown; i++) cout << (i == target ? string(T().mid) + " " GLYPH_PTR " " + RESET : string("    "));
    cout << "\n  ";
    for (int i = 0; i < shown; i++) {
        char b[16]; sprintf(b, "[%2d]", i);
        cout << (i == target ? string(T().low) + b + RESET : string(DIM) + b + RESET);
    }
    cout << "\n" << flush;
    SLEEP_MS(600);
    cout << "  " << DIM << "jumped straight to bucket " << target << " (no scanning)" << RESET << "\n";
    SLEEP_MS(500);
    cout << "  chain: ";
    for (int i = 0; i < chainLength; i++) {
        bool hit = (i == foundAt);
        cout << (hit ? T().low : WHITE) << "[" << (hit ? "FOUND" : "next?") << "]" << RESET << flush;
        SLEEP_MS(450);
        if (hit) break;
        cout << " " << GLYPH_ARROW << " ";
    }
    cout << "\n\n";
}

/*
============================================================
     SHARED DRAWING HELPERS: DISC + VERTICAL EQUALIZER
============================================================
*/

// One text row of the spinning disc (always 4*R+1 visible characters).
inline string discRow(int ry, int R, double theta) {
    string line;
    for (int cx = -2 * R; cx <= 2 * R; cx++) {
        double dx = cx / 2.0, dy = (double)ry;            // chars are ~2x taller than wide
        double d = sqrt(dx * dx + dy * dy);
        double diff = atan2(dy, dx) - theta;
        while (diff >  ANIM_PI) diff -= 2 * ANIM_PI;
        while (diff < -ANIM_PI) diff += 2 * ANIM_PI;
        double d1 = fabs(diff), d2 = ANIM_PI - d1;        // distance to marker / opposite side

        if (d > R + 0.4)       line += " ";
        else if (d < 0.7)      line += string(BOLD_WHITE) + "o" + RESET;
        else if (d < 2.0) {
            if (d1 < 0.5) line += string(BOLD_WHITE) + GLYPH_FULL + RESET;          // label notch
            else          line += string(T().label) + GLYPH_FULL + RESET;           // label
        } else {
            if (d * sin(d1) < 0.45) line += string(BOLD_WHITE) + GLYPH_FULL + RESET;     // sheen
            else if ((int)(d * 1.5) % 2 == 0) line += string(DIM) + WHITE + "." + RESET; // grooves
            else                              line += string(DIM) + WHITE + ":" + RESET;
        }
    }
    return line;
}

// Row k (0 = top) of a vertical equalizer, 3 characters per bar.
inline string eqRow(const int level[], int bars, int eqh, int k) {
    int r = eqh - 1 - k;
    const char* col = (r == eqh - 1) ? T().high : (r == eqh - 2 ? T().mid : T().low);
    string row;
    for (int i = 0; i < bars; i++) {
        if (level[i] > r) row += string(col) + GLYPH_FULL + GLYPH_FULL + RESET + " ";
        else              row += "   ";
    }
    return row;
}

/*
============================================================
               11. SPINNING DISC + EQUALIZER
============================================================
*/
inline void spinningDiscAnimation(const string& title, const string& artist, const string& album,
                                  int durationSeconds, int realSeconds = 8) {
    const int R = 4, ROWS = 2 * R + 1, BARS = 14, EQH = 4;
    int level[BARS];
    for (int i = 0; i < BARS; i++) level[i] = rand() % (EQH + 1);

    double theta = 0.0;
    int frames = realSeconds * 12;   // ~80 ms per frame, about 45 rpm
    cout << HIDE_CURSOR;
    for (int f = 0; f <= frames; f++) {
        if (f > 0) moveUp(ROWS);
        int elapsed = (durationSeconds * f) / frames;

        string right[ROWS];
        right[0] = string(T().primary) + GLYPH_NOTE + " NOW PLAYING" + RESET;
        right[1] = string(BOLD_WHITE) + title + RESET;
        right[2] = string(DIM) + T().secondary + artist + "  |  " + album + RESET;
        right[3] = "";
        for (int i = 0; i < BARS; i++) {
            level[i] += (rand() % 3) - 1;
            if (rand() % 6 == 0) level[i] += (rand() % 5) - 2;
            if (level[i] < 0) level[i] = 0;
            if (level[i] > EQH) level[i] = EQH;
        }
        for (int k = 0; k < EQH; k++) right[4 + k] = eqRow(level, BARS, EQH, k);
        right[8] = string(T().low) + GLYPH_PLAY + RESET + " " + formatTime(elapsed) + " " +
                   makeBar(elapsed, durationSeconds, 16) + " " + formatTime(durationSeconds);

        for (int ry = -R; ry <= R; ry++)
            cout << CLEAR_LINE << "  " << discRow(ry, R, theta) << "   " << right[ry + R] << ERASE_EOL << "\n";
        cout << flush;
        theta += 0.38;
        if (theta > 2 * ANIM_PI) theta -= 2 * ANIM_PI;
        SLEEP_MS(80);
    }
    cout << SHOW_CURSOR;
}

/*
=====================================================================
                      12. LIVE PLAYER
  Full-screen and resize-safe: keyboard controls, shuffle, repeat,
  volume, sleep timer, vinyl scratch on skip
=====================================================================
*/
struct AnimTrack {
    string title;
    string artist;
    string album;
    int    duration;
    AnimTrack() : duration(0) {}
};

struct PlayerState {
    int    cur, prev, played;
    int    repeatMode;          // 0 = off, 1 = all, 2 = one
    int    volume;              // 0..10
    int    scratch;             // frames of scratch effect left
    int    sleepIdx;
    bool   paused, ended, shuffle;
    double elapsed, sleepLeft;
    PlayerState() : cur(0), prev(0), played(1), repeatMode(0), volume(7), scratch(0), sleepIdx(0),
                    paused(false), ended(false), shuffle(false), elapsed(0), sleepLeft(0) {}
};

inline int pickRandomOther(int cur, int n) {
    if (n <= 1) return cur;
    int x;
    do { x = rand() % n; } while (x == cur);
    return x;
}

inline void startTrack(PlayerState& s, int idx, bool manual) {
    s.prev = s.cur;
    s.cur = idx;
    s.elapsed = 0;
    s.played++;
    s.ended = false;
    if (manual) s.scratch = 10;
}

inline string scrollText(const string& text, int width, int frame) {
    if (utf8Len(text) <= width) return padTo(text, width);
    string loop = text + "   *   ";
    string doubled = loop + loop;
    return utf8Sub(doubled, (frame / 2) % utf8Len(loop), width);
}

inline string clipStr(const string& s, int w) {
    return utf8Sub(s, 0, w);
}
/* 
	Full-screen player. Redraws from the top-left every frame (no cursor-up
	arithmetic), so resizing the window can never stack or tear the frames.
	Layout (13 rows):  title line / 9-row disc + info panel / controls / volume / message
*/
inline void livePlayer(const AnimTrack tracks[], int n, double speedFactor = 10.0) {
    if (n <= 0) return;
    const int R = 4, ROWS = 2 * R + 1, BARS = 14, EQH = 4;
    const int MIN_ROWS = ROWS + 4, MIN_COLS = 46;
    const int sleepOpts[4] = {0, 15, 30, 60};
    int level[BARS];
    for (int i = 0; i < BARS; i++) level[i] = 0;

    PlayerState s;
    double theta = 0.0, spin = 0.38;
    bool quit = false, sleepEnded = false;
    int msgTtl = 0, frame = 0, lastCols = -1, lastRows = -1;
    string msg;

    rawModeOn();
    cout << ALT_SCREEN_ON << HIDE_CURSOR << CLEAR_SCREEN << flush;     // alternate screen
    while (!quit) {
        // 1. keyboard
        while (keyAvailable()) {
            int k = readKey();
            if (k < 0) break;
            if (k == 'q' || k == 'Q') { quit = true; break; }
            else if (k == ' ') {
                if (s.ended) { startTrack(s, 0, false); s.paused = false; msg = "Restarted playlist"; }
                else { s.paused = !s.paused; msg = s.paused ? "Paused" : "Playing"; }
            }
            else if (k == 'n' || k == 'N') {
                startTrack(s, s.shuffle ? pickRandomOther(s.cur, n) : (s.cur + 1) % n, true);
                s.paused = false; msg = ">> Next track";
            }
            else if (k == 'p' || k == 'P') {
                startTrack(s, s.shuffle ? s.prev : (s.cur - 1 + n) % n, true);
                s.paused = false; msg = "<< Previous track";
            }
            else if (k == 's' || k == 'S') { s.shuffle = !s.shuffle; msg = s.shuffle ? "Shuffle ON" : "Shuffle OFF"; }
            else if (k == 'r' || k == 'R') {
                s.repeatMode = (s.repeatMode + 1) % 3;
                msg = s.repeatMode == 0 ? "Repeat OFF" : (s.repeatMode == 1 ? "Repeat ALL" : "Repeat ONE");
            }
            else if (k == '+' || k == '=') { if (s.volume < 10) s.volume++; msg = "Volume up"; }
            else if (k == '-' || k == '_') { if (s.volume > 0)  s.volume--; msg = "Volume down"; }
            else if (k == 't' || k == 'T') {
                s.sleepIdx = (s.sleepIdx + 1) % 4;
                s.sleepLeft = sleepOpts[s.sleepIdx];
                msg = s.sleepIdx == 0 ? "Sleep timer OFF" : "Sleep timer set";
            }
            else if (k == 'h' || k == 'H') {
                themeIndex() = (themeIndex() + 1) % THEME_COUNT;
                msg = string("Theme: ") + T().name;
            }
            else continue;
            msgTtl = 25;
        }
        if (quit) break;

        // 2. playback logic
        if (!s.paused && !s.ended) {
            s.elapsed += 0.08 * speedFactor;
            if (s.elapsed >= tracks[s.cur].duration) {
                if (s.repeatMode == 2) s.elapsed = 0;
                else {
                    bool last = s.shuffle ? (s.repeatMode == 0 && s.played >= n)
                                          : (s.cur == n - 1 && s.repeatMode == 0);
                    if (last) { s.ended = true; s.elapsed = tracks[s.cur].duration; msg = "End of playlist"; msgTtl = 60; }
                    else startTrack(s, s.shuffle ? pickRandomOther(s.cur, n) : (s.cur + 1) % n, false);
                }
            }
        }
        if (s.sleepLeft > 0) {
            s.sleepLeft -= 0.08;
            if (s.sleepLeft <= 0) { s.sleepLeft = 0; sleepEnded = true; quit = true; }
        }
        const AnimTrack& tr = tracks[s.cur];

        // 3. disc rotation (eases up/down, scratches on skip)
        double target = (s.paused || s.ended) ? 0.0 : 0.38;
        spin += (target - spin) * 0.2;
        if (s.scratch > 0) { theta += ((s.scratch % 2) ? -0.9 : 0.9); s.scratch--; }
        else theta += spin;
        while (theta < 0) theta += 2 * ANIM_PI;
        while (theta > 2 * ANIM_PI) theta -= 2 * ANIM_PI;

        // 4. equalizer (height follows volume)
        int maxLevel = (EQH * s.volume + 5) / 10;
        bool active = !s.paused && !s.ended;
        for (int i = 0; i < BARS; i++) {
            if (active) {
                level[i] += (rand() % 3) - 1;
                if (rand() % 6 == 0) level[i] += (rand() % 5) - 2;
                if (s.scratch > 0) level[i] = rand() % (maxLevel + 1);
            } else if (level[i] > 0) level[i]--;
            if (level[i] < 0) level[i] = 0;
            if (level[i] > maxLevel) level[i] = maxLevel;
        }

        // 5. render
        int cols, rows;
        getTermSize(cols, rows);
        if (cols != lastCols || rows != lastRows) { cout << CLEAR_SCREEN; lastCols = cols; lastRows = rows; }
        cout << CURSOR_HOME;                                              // always start at top-left

        if (cols < MIN_COLS || rows < MIN_ROWS) {
            cout << "  Window too small" << ERASE_EOL << "\n  need " << MIN_COLS << "x" << MIN_ROWS
                 << ", have " << cols << "x" << rows << ERASE_EOL << ERASE_BELOW << flush;
            SLEEP_MS(80); frame++;
            continue;
        }

        int avail = cols - 23;                                         // room right of the disc
        int bars  = avail / 3;  if (bars > BARS) bars = BARS;
        int tw    = cols - 6;   if (tw > 50) tw = 50;
        int bw    = avail - 15; if (bw > 16) bw = 16; if (bw < 4) bw = 4;

        // song title, scrolling, ONE LINE ABOVE THE DISC
        cout << "  " << T().primary << GLYPH_NOTE << RESET << " " << BOLD_WHITE << scrollText(tr.title, tw, frame) << RESET << ERASE_EOL << "\n";
		cout << ERASE_EOL << "\n";

        string right[ROWS];
        string headText = s.ended ? string("END OF PLAYLIST")
                        : s.paused ? string("|| PAUSED")
                        : string(GLYPH_NOTE) + " NOW PLAYING";
        char idxBuf[32]; sprintf(idxBuf, "   [%d/%d]", s.cur + 1, n);
        right[0] = string(s.ended || s.paused ? T().mid : T().primary) + headText + RESET + DIM + idxBuf + RESET;
        right[1] = string(DIM) + T().secondary + clipStr(tr.artist + "  |  " + tr.album, avail) + RESET;
        right[2] = string("Shuffle ") + (s.shuffle ? string(T().low) + "ON " : string(DIM) + "off") + RESET +
                   "  Repeat " + (s.repeatMode == 0 ? string(DIM) + "off" :
                                   s.repeatMode == 1 ? string(T().low) + "ALL" : string(T().mid) + "ONE") + RESET;
        right[3] = "";
        for (int k = 0; k < EQH; k++) right[4 + k] = eqRow(level, bars, EQH, k);
        int el = (int)s.elapsed;
        right[8] = string(T().low) + (s.paused || s.ended ? "||" : GLYPH_PLAY) + RESET + " " + formatTime(el) + " " +
                   makeBar(el, tr.duration, bw) + " " + formatTime(tr.duration);

        for (int ry = -R; ry <= R; ry++)
            cout << "  " << discRow(ry, R, theta) << "   " << right[ry + R] << ERASE_EOL << "\n";

        // controls, ONE LINE BELOW THE DISC (longest version that fits the window)
        const string helps[3] = {
            "[space]play/pause [n]ext [p]rev [s]huffle [r]epeat [+/-]vol [t]imer [h]theme [q]uit",
            "spc pause  n next  p prev  s shuf  r rep  +/- vol  t timer  h theme  q quit",
            "spc n p s r +/- t h q" };
        string help = helps[2];
        for (int h = 0; h < 3; h++) if ((int)helps[h].size() + 3 <= cols) { help = helps[h]; break; }
        cout << DIM << "  " << help << RESET << ERASE_EOL << "\n";

        char volBuf[64];
        if (s.volume == 0) sprintf(volBuf, "MUTE");
        else sprintf(volBuf, "%d%%", s.volume * 10);
        char sleepBuf[32];
        if (s.sleepLeft > 0) sprintf(sleepBuf, "%ds", (int)ceil(s.sleepLeft)); else sprintf(sleepBuf, "off");
        cout << "  Vol [" << makeBar(s.volume, 10, 10) << "] " << volBuf
             << "     Sleep: " << sleepBuf << ERASE_EOL << "\n";

        if (s.scratch > 0) cout << T().mid << "  *scratch*" << RESET;
        else if (msgTtl > 0) { cout << T().secondary << "  " << clipStr(msg, cols - 4) << RESET; msgTtl--; }
        cout << ERASE_EOL << ERASE_BELOW << flush;                       // last row: no newline, clear anything below

        SLEEP_MS(80);
        frame++;
    }
    rawModeOff();
    cout << ALT_SCREEN_OFF << SHOW_CURSOR;                              // back to the normal screen
    if (sleepEnded) printInfo("Sleep timer finished. Goodnight!");
    else            printInfo("Player closed.");
}

/*
============================================================
   13. STACK ANIMATION (push / pop, like Recently Played)
============================================================
*/
inline void drawStack(const string stk[], int k, int cap, const string& label,
                      const string& flying, int flyRow, int flyIndent, int w = 14) {
    cout << CLEAR_LINE << "  " << T().secondary << label << RESET << ERASE_EOL << "\n";
    for (int r = 0; r <= cap; r++) {
        int idx = cap - r;
        cout << CLEAR_LINE << "  ";
        if (!flying.empty() && r == flyRow) {
            if (flyIndent > 0) cout << string(flyIndent * 3, ' ') << DIM << padTo(flying, w) << RESET;
            else cout << DIM << "|" << RESET << T().primary << padTo(flying, w) << RESET << DIM << "|" << RESET;
        } else if (r >= 1 && idx < k) {
            cout << DIM << "|" << RESET << (idx == k - 1 ? T().low : WHITE) << padTo(stk[idx], w) << RESET
                 << DIM << "|" << RESET << (idx == k - 1 ? "  <- top" : "");
        } else {
            cout << DIM << "|" << string(w, ' ') << "|" << RESET;
        }
        cout << ERASE_EOL << "\n";
    }
    cout << CLEAR_LINE << "  " << DIM << "+" << string(w, '-') << "+" << RESET << ERASE_EOL << "\n" << flush;
}

inline void stackAnimation(const string base[], int baseCount, const string pushes[], int pushCount) {
    const int cap = 5, lines = cap + 3;
    string stk[8];
    int k = 0;
    for (int i = 0; i < baseCount && k < cap - 1; i++) stk[k++] = base[i];
    int pushed = 0;

    cout << HIDE_CURSOR;
    for (int i = 0; i < lines; i++) cout << "\n";
    moveUp(lines); drawStack(stk, k, cap, "Stack (LIFO)", "", 0, 0); SLEEP_MS(600);

    for (int p = 0; p < pushCount && k < cap; p++) {
        for (int fr = 0; fr <= cap - k; fr++) {
            moveUp(lines); drawStack(stk, k, cap, "PUSH: " + pushes[p], pushes[p], fr, 0); SLEEP_MS(110);
        }
        stk[k++] = pushes[p]; pushed++;
        moveUp(lines); drawStack(stk, k, cap, "PUSH: " + pushes[p] + "  (O(1))", "", 0, 0); SLEEP_MS(450);
    }
    while (pushed > 0) {
        string item = stk[k - 1];
        k--; pushed--;
        for (int fr = cap - k; fr >= 0; fr--) {
            moveUp(lines); drawStack(stk, k, cap, "POP: " + item, item, fr, 0); SLEEP_MS(110);
        }
        for (int ind = 1; ind <= 6; ind++) {
            moveUp(lines); drawStack(stk, k, cap, "POP: " + item, item, 0, ind); SLEEP_MS(70);
        }
        moveUp(lines); drawStack(stk, k, cap, "POP: " + item + "  (O(1))", "", 0, 0); SLEEP_MS(350);
    }
    cout << SHOW_CURSOR;
}

/*
============================================================
   14. QUEUE ANIMATION (enqueue / dequeue, like Up Next)
============================================================
*/
inline string queueCell(const string& s, int w = 10) { return "[" + padTo(s, w) + "]"; }

inline void drawQueue(const string q[], int c, int headShrink, int tailGap,
                      const string& incoming, const string& label, int w = 10) {
    cout << CLEAR_LINE << "  " << T().secondary << label << RESET << ERASE_EOL << "\n";
    cout << CLEAR_LINE << "  " << DIM << "OUT <= " << RESET;
    for (int i = 0; i < c; i++) {
        string cell = queueCell(q[i], w);
        if (i == 0 && headShrink >= 0) cout << T().high << utf8Sub(cell, 0, headShrink) << RESET;
        else cout << (i == 0 ? T().low : WHITE) << cell << RESET;
    }
    if (tailGap >= 0 && !incoming.empty()) cout << string(tailGap, ' ') << T().primary << queueCell(incoming, w) << RESET;
    cout << DIM << "  <= IN" << RESET << ERASE_EOL << "\n" << flush;
}

inline void queueAnimation(const string base[], int baseCount, const string incoming[], int inCount) {
    string q[8];
    int c = 0;
    for (int i = 0; i < baseCount && c < 6; i++) q[c++] = base[i];

    cout << HIDE_CURSOR << "\n\n";
    moveUp(2); drawQueue(q, c, -1, -1, "", "Queue (FIFO)"); SLEEP_MS(600);

    for (int i = 0; i < inCount && c < 6; i++) {
        for (int gap = 22; gap >= 0; gap -= 2) {
            moveUp(2); drawQueue(q, c, -1, gap, incoming[i], "ENQUEUE: " + incoming[i]); SLEEP_MS(60);
        }
        q[c++] = incoming[i];
        moveUp(2); drawQueue(q, c, -1, -1, "", "ENQUEUE: " + incoming[i] + "  (O(1), added at rear)"); SLEEP_MS(450);
    }
    while (c > 0) {
        string item = q[0];
        for (int w = 12; w >= 0; w -= 2) {
            moveUp(2); drawQueue(q, c, w, -1, "", "DEQUEUE: " + item); SLEEP_MS(70);
        }
        for (int i = 1; i < c; i++) q[i - 1] = q[i];
        c--;
        moveUp(2); drawQueue(q, c, -1, -1, "", "DEQUEUE: " + item + "  (O(1), removed from front)"); SLEEP_MS(450);
    }
    cout << SHOW_CURSOR;
}

/*
============================================================
		STACK + QUEUE ANIMATIONS FOR REAL DATA
  Used by the History stack and the Up Next queue screens.
============================================================
*/
const int STACK_CAP = 5;

// Short text for a cell: optionally drops the "Artist - " prefix and clips with "..".
inline string cellName(const string& title, int w, bool dropArtist) {
    string t = title;
    size_t p = t.find(" - ");
    if (dropArtist && p != string::npos && p + 3 < t.size()) t = t.substr(p + 3);
    if (utf8Len(t) > w) t = utf8Sub(t, 0, w - 2) + "..";
    return t;
}

// How many queue cells fit on one terminal line.
inline int queueMaxVisible() {
    int cols, rows;
    getTermSize(cols, rows);
    int m = (cols - 38) / 14;
    return m < 2 ? 2 : (m > 5 ? 5 : m);
}

// An item falls from the top and lands on the stack (the stack keeps stk[0..k-1]).
inline void stackFall(string stk[], int& k, const string& item, int w, int delayMs) {
    const int cap = STACK_CAP, lines = cap + 3;
    for (int fr = 0; fr <= cap - k; fr++) {
        moveUp(lines);
        drawStack(stk, k, cap, "PUSH: " + item, item, fr, 0, w);
        SLEEP_MS(delayMs);
    }
    stk[k++] = item;
}

// items[0] = oldest ... items[n-1] = newest. Builds the stack by pushing the newest 5.
inline void stackBuildAnimation(const string items[], int n, int w = 36) {
    const int cap = STACK_CAP, lines = cap + 3;
    string stk[8];
    int k = 0;
    cout << HIDE_CURSOR;
    for (int i = 0; i < lines; i++) cout << "\n";
    moveUp(lines);
    drawStack(stk, k, cap, "Recently played stack (LIFO)", "", 0, 0, w);
    SLEEP_MS(300);
    for (int i = (n > cap ? n - cap : 0); i < n; i++) stackFall(stk, k, cellName(items[i], w, false), w, 30);
    moveUp(lines);
    drawStack(stk, k, cap, "Recently played stack (LIFO): the top is the newest", "", 0, 0, w);
    cout << SHOW_CURSOR;
}

// Pops the newest item: it lifts off the top and slides away.
inline void stackPopAnimation(const string items[], int n, int w = 36) {
    const int cap = STACK_CAP, lines = cap + 3;
    string stk[8];
    int k = 0;
    for (int i = (n > cap ? n - cap : 0); i < n; i++) stk[k++] = cellName(items[i], w, false);
    if (k == 0) return;
    cout << HIDE_CURSOR;
    for (int i = 0; i < lines; i++) cout << "\n";
    moveUp(lines);
    drawStack(stk, k, cap, "Recently played stack (LIFO)", "", 0, 0, w);
    SLEEP_MS(500);
    string item = stk[--k];
    for (int fr = cap - k; fr >= 0; fr--) {
        moveUp(lines); drawStack(stk, k, cap, "POP: " + item, item, fr, 0, w); SLEEP_MS(90);
    }
    for (int ind = 1; ind <= 6; ind++) {
        moveUp(lines); drawStack(stk, k, cap, "POP: " + item, item, 0, ind, w); SLEEP_MS(60);
    }
    moveUp(lines);
    drawStack(stk, k, cap, "POP: " + item + "  (O(1), newest removed)", "", 0, 0, w);
    SLEEP_MS(500);
    cout << SHOW_CURSOR;
}

// Shows the front of the queue; items[0] plays first. total = real queue length.
inline void queueBuildAnimation(const string items[], int n, int total) {
    const int W = 12, maxVis = queueMaxVisible();
    string q[8];
    int c = 0;
    char b[64];
    cout << HIDE_CURSOR << "\n\n";
    moveUp(2);
    drawQueue(q, c, -1, -1, "", "Up Next queue (FIFO)", W);
    SLEEP_MS(300);
    for (int i = 0; i < n && i < maxVis; i++) {
        string it = cellName(items[i], W, true);
        for (int gap = 6; gap >= 0; gap -= 2) {
            moveUp(2); drawQueue(q, c, -1, gap, it, "ENQUEUE: " + it, W); SLEEP_MS(35);
        }
        q[c++] = it;
    }
    sprintf(b, "Up Next queue (FIFO): %d queued, the front plays first", total);
    moveUp(2);
    drawQueue(q, c, -1, -1, "", b, W);
    cout << SHOW_CURSOR;
}

// A new song slides in at the rear of the queue.
inline void queueEnqueueAnimation(const string items[], int n, int total, const string& newItem) {
    const int W = 12, maxVis = queueMaxVisible();
    string q[8];
    int c = 0;
    char b[32];
    for (int i = 0; i < n && c < maxVis; i++) q[c++] = cellName(items[i], W, true);
    string inc = cellName(newItem, W, true);
    cout << HIDE_CURSOR << "\n\n";
    moveUp(2);
    drawQueue(q, c, -1, -1, "", "Up Next queue (FIFO)", W);
    SLEEP_MS(500);
    for (int gap = 6; gap >= 0; gap -= 2) {
        moveUp(2); drawQueue(q, c, -1, gap, inc, "ENQUEUE: " + inc, W); SLEEP_MS(90);
    }
    if (n < maxVis) q[c++] = inc;
    sprintf(b, "  (O(1), %d queued)", total + 1);
    moveUp(2);
    drawQueue(q, c, -1, -1, "", "ENQUEUE: " + inc + b, W);
    SLEEP_MS(600);
    cout << SHOW_CURSOR;
}

// The front song leaves the queue and everything shuffles forward.
inline void queueDequeueAnimation(const string items[], int n, int total) {
    const int W = 12, maxVis = queueMaxVisible();
    string q[8];
    int c = 0;
    char b[32];
    for (int i = 0; i < n && c < maxVis; i++) q[c++] = cellName(items[i], W, true);
    if (c == 0) return;
    string item = q[0];
    cout << HIDE_CURSOR << "\n\n";
    moveUp(2);
    drawQueue(q, c, -1, -1, "", "Up Next queue (FIFO)", W);
    SLEEP_MS(500);
    for (int w = W + 2; w >= 0; w -= 2) {
        moveUp(2); drawQueue(q, c, w, -1, "", "DEQUEUE: " + item, W); SLEEP_MS(70);
    }
    for (int i = 1; i < c; i++) q[i - 1] = q[i];
    c--;
    if (n > maxVis) q[c++] = cellName(items[maxVis], W, true);                // next hidden song moves up
    sprintf(b, "  (O(1), %d left)", total - 1);
    moveUp(2);
    drawQueue(q, c, -1, -1, "", "DEQUEUE: " + item + b, W);
    SLEEP_MS(600);
    cout << SHOW_CURSOR;
}

/*
============================================================
              15. BOOT SEQUENCE (system check)
============================================================
*/
inline void bootSequence(int songCount = 3, int userCount = 2) {
    char b1[64], b2[64];
    sprintf(b1, "Loading songs.txt (%d tracks)", songCount);
    sprintf(b2, "Loading users.txt (%d accounts)", userCount);
    const string steps[7] = {
        "Allocating hash table (100 buckets)", b1, "Indexing library by title", b2,
        "Linking circular playlist", "Preparing Up Next queue and history stack", "Starting dashboard"
    };
    cout << HIDE_CURSOR << "\n" << T().primary << "  ECHO ENGINE :: SYSTEM CHECK" << RESET << "\n\n";
    for (int i = 0; i < 7; i++) {
        int cycles = 6 + rand() % 8;
        for (int c = 0; c < cycles; c++) {
            cout << CLEAR_LINE << "  [" << T().secondary << SPIN_FRAMES[c % SPIN_COUNT] << RESET << "] " << steps[i] << flush;
            SLEEP_MS(70);
        }
        cout << CLEAR_LINE << "  [ " << T().low << "OK" << RESET << " ] " << steps[i] << "\n";
    }
    cout << "\n" << T().low << "  " << GLYPH_CHECK << " All systems ready." << RESET << "\n\n" << SHOW_CURSOR;
}
/*
============================================================
                     16. THEME SELECTOR
============================================================
*/
inline void themeMenu() {
    printSectionHeader("COLOUR THEMES");
    const Theme* t = themeTable();
    for (int i = 0; i < THEME_COUNT; i++) {
        cout << "  " << (i + 1) << ". " << padTo(t[i].name, 13)
             << t[i].primary << GLYPH_FULL << GLYPH_FULL << t[i].secondary << GLYPH_FULL << GLYPH_FULL
             << t[i].label << GLYPH_FULL << GLYPH_FULL << t[i].low << GLYPH_FULL << GLYPH_FULL
             << t[i].mid << GLYPH_FULL << GLYPH_FULL << t[i].high << GLYPH_FULL << GLYPH_FULL << RESET
             << (i == themeIndex() ? "  (current)" : "") << "\n";
    }
    cout << "\n  Choose theme (1-" << THEME_COUNT << "): ";
    int c;
    if (cin >> c && c >= 1 && c <= THEME_COUNT) {
        themeIndex() = c - 1;
        printSuccess(string("Theme set to ") + T().name);
    } else {
        cin.clear();
        cin.ignore(1000, '\n');
        printWarn("Theme unchanged.");
    }
}

#endif
