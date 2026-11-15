#include "animations.h"
#include "FileUtils.h"

/*
============================================================
                       COLOUR THEMES
============================================================
*/
static void setTheme(Theme& t, const char* name, const char* primary, const char* secondary,
                     const char* label, const char* low, const char* mid, const char* high) {
    t.name = name; t.primary = primary; t.secondary = secondary; t.label = label;
    t.low = low; t.mid = mid; t.high = high;
}

// Filled on every call, so the colours always match the runtime ANSI setting.
const Theme* themeTable() {
    static Theme t[THEME_COUNT];
    setTheme(t[0], "Neon",        BOLD_MAGENTA, BOLD_CYAN,    MAGENTA, BOLD_GREEN,  BOLD_YELLOW, BOLD_RED);
    setTheme(t[1], "Ocean",       BOLD_BLUE,    BOLD_CYAN,    BLUE,    BOLD_CYAN,   BOLD_BLUE,   BOLD_WHITE);
    setTheme(t[2], "Retro Green", BOLD_GREEN,   GREEN,        GREEN,   GREEN,       BOLD_GREEN,  BOLD_WHITE);
    setTheme(t[3], "Sunset",      BOLD_YELLOW,  BOLD_MAGENTA, RED,     BOLD_YELLOW, BOLD_RED,    BOLD_MAGENTA);
    return t;
}

int& themeIndex() { static int i = 0; return i; }

/*
============================================================
                  SETUP AND SMALL HELPERS
============================================================
*/
const char* eqLevel(int i) {
    static const char* uni[8] = {"\xE2\x96\x81","\xE2\x96\x82","\xE2\x96\x83","\xE2\x96\x84",
                                 "\xE2\x96\x85","\xE2\x96\x86","\xE2\x96\x87","\xE2\x96\x88"};
    static const char* asc[8] = {"_", ".", "-", "=", "+", "*", "#", "#"};
    return g_unicodeEnabled ? uni[i] : asc[i];
}

string padNum(int n, int width) {
    string s = intStr(n);
    int len = (int)s.size();
    return len >= width ? s : string(width - len, ' ') + s;
}

string formatTime(int sec) {
    char buf[32];
    sprintf(buf, "%02d:%02d", sec / 60, sec % 60);
    return buf;
}

// UTF-8 aware text helpers: columns are counted in characters, never in bytes
int utf8Len(const string& s) {
    int n = 0;
    for (size_t i = 0; i < s.size(); i++)
        if (((unsigned char)s[i] & 0xC0) != 0x80) n++;
    return n;
}

size_t utf8Off(const string& s, int cp) {          // byte offset of character number cp
    int n = 0;
    for (size_t i = 0; i < s.size(); i++)
        if (((unsigned char)s[i] & 0xC0) != 0x80) { if (n == cp) return i; n++; }
    return s.size();
}

string utf8Sub(const string& s, int start, int len) {
    size_t a = utf8Off(s, start), b = utf8Off(s, start + len);
    return s.substr(a, b - a);
}

string padTo(const string& text, int w) {
    string s = utf8Sub(text, 0, w);
    return s + string(w - utf8Len(s), ' ');
}

string makeBar(int value, int total, int width) {
    int filled = (total > 0) ? (value * width) / total : 0;
    if (filled > width) filled = width;
    if (filled < 0) filled = 0;
    return string(T().low) + repeatStr(GLYPH_FULL, filled) + RESET + DIM +
           repeatStr(GLYPH_EMPTY, width - filled) + RESET;
}

string scrollText(const string& text, int width, int frame) {
    if (utf8Len(text) <= width) return padTo(text, width);
    string loop = text + "   *   ";
    string doubled = loop + loop;
    return utf8Sub(doubled, (frame / 2) % utf8Len(loop), width);
}

string clipStr(const string& s, int w) {
    return utf8Sub(s, 0, w);
}

void listWidths(int& tw, int& aw) {
    int cols, rows;
    getTermSize(cols, rows);
    aw = (cols >= 110) ? 24 : 18;
    tw = cols - 1 - 16 - aw;
    if (tw > 64) tw = 64;
    if (tw < 16) tw = 16;
}

/*
============================================================
              Animations that used to live in colours.h
============================================================
*/
// helpers used by the four animations below
// Fixed-width dots: "   ", ".  ", ".. ", "..." (no jitter when the width changes)
static string dotsText(int beat) {
    int d = beat % 4;
    return string(d, '.') + string(3 - d, ' ');
}

static string simpleBar(int value, int total, int width, const char* colour) {
    int filled = (total > 0) ? (value * width) / total : 0;
    if (filled > width) filled = width;
    if (filled < 0) filled = 0;
    return string(colour) + repeatStr(GLYPH_FULL, filled) + RESET + DIM +
           repeatStr(GLYPH_EMPTY, width - filled) + RESET;
}

// Final result line: green tick + elapsed time, or red cross on failure
static void finishAnimation(bool ok, const string& okText, const string& failText, int frames, int delayMs) {
    char buf[32];
    sprintf(buf, " (%.1fs)", frames * delayMs / 1000.0);
    cout << CLEAR_LINE;
    if (ok) cout << BOLD_GREEN << "  " << GLYPH_CHECK << " " << okText   << RESET << DIM << buf << RESET << "\n";
    else    cout << BOLD_RED   << "  " << GLYPH_CROSS << " " << failText << RESET << DIM << buf << RESET << "\n";
    cout << SHOW_CURSOR;
}

// "Processing" Animation: spinner + fixed-width dots
void processingAnimation(int cycles, const string& subject, bool success) {
    int frames = cycles * 4;
    string what = subject.empty() ? string("") : " " + subject;
    cout << HIDE_CURSOR;
    for (int i = 0; i < frames; i++) {
        cout << CLEAR_LINE << BOLD_CYAN << "  " << spinFrame(i)
             << " Processing" << what << dotsText(i / 4) << RESET << flush;
        SLEEP_MS(65);
    }
    finishAnimation(success, "Done!", "Processing failed" + (subject.empty() ? string("") : ": " + subject), frames, 65);
}

// "Updating" Animation: a bright wave travels across the word
void updatingAnimation(int cycles, const string& subject, bool success) {
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
void savingAnimation(int cycles, const string& subject, bool success) {
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
void loadingAnimation(int cycles, const string& subject, bool success) {
    int frames = cycles * 4;
    if (frames < 1) frames = 1;
    int stopAt = success ? frames : (frames * 70) / 100;      // a failed load stops part-way
    cout << HIDE_CURSOR;
    for (int i = 0; i <= stopAt; i++) {
        int pct = (i * 100) / frames;
        cout << CLEAR_LINE << BOLD_BLUE << "  " << spinFrame(i) << " Loading" << RESET
             << (subject.empty() ? "" : " ") << subject
             << " [" << simpleBar(i, frames, 20, BOLD_BLUE) << "] " << pct << "%" << flush;
        SLEEP_MS(50);
    }
    finishAnimation(success, "Ready!", "Load failed" + (subject.empty() ? string("") : ": " + subject), stopAt, 50);
}

/*
============================================================
                       1. TYPEWRITER
============================================================
*/
void typewriter(const string& text, const char* colour, int delayMs) {
    cout << (colour ? colour : BOLD_WHITE);
    for (size_t i = 0; i < text.size(); i++) { cout << text[i] << flush; SLEEP_MS(delayMs); }
    cout << RESET;
}

/*
============================================================
                      2. SPLASH SCREEN
============================================================
*/
void splashAnimation() {
    const int W = uiWidth();
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
void spinnerAnimation(const string& msg, int cycles, const char* colour) {
    cout << HIDE_CURSOR;
    for (int i = 0; i < cycles; i++) {
        cout << CLEAR_LINE << (colour ? colour : T().secondary) << "  "
             << spinFrame(i) << " " << msg << RESET << flush;
        SLEEP_MS(90);
    }
    cout << CLEAR_LINE << SHOW_CURSOR;
}

/*
============================================================
           4. PROGRESS BAR (loading / uploading)
============================================================
*/
void progressBarAnimation(const string& label, int steps) {
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
     SHARED DRAWING HELPERS: DISC + VERTICAL EQUALIZER
============================================================
*/

// One text row of the spinning disc (always 4*R+1 visible characters).
string discRow(int ry, int R, double theta) {
    string line;
    for (int cx = -2 * R; cx <= 2 * R; cx++) {
        double dx = cx / 2.0, dy = (double)ry;            // chars are ~2x taller than wide
        double d = sqrt(dx * dx + dy * dy);
        double diff = atan2(dy, dx) - theta;
        while (diff >  ANIM_PI) diff -= 2 * ANIM_PI;
        while (diff < -ANIM_PI) diff += 2 * ANIM_PI;
        double d1 = fabs(diff);                           // distance to the marker

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
string eqRow(const int level[], int bars, int eqh, int k) {
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
void spinningDiscAnimation(const string& title, const string& artist, const string& album,
                           int durationSeconds, int realSeconds) {
    const int R = PLAYER_DISC_R, ROWS = 2 * R + 1, BARS = 14, EQH = PLAYER_EQ_H;
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
============================================================
   STACK ANIMATION (push / pop, like Recently Played)
============================================================
*/
void drawStack(const string stk[], int k, int cap, const string& label,
               const string& flying, int flyRow, int flyIndent, int w) {
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

/*
============================================================
   QUEUE ANIMATION (enqueue / dequeue, like Up Next)
============================================================
*/
string queueCell(const string& s, int w) { return "[" + padTo(s, w) + "]"; }

void drawQueue(const string q[], int c, int headShrink, int tailGap,
               const string& incoming, const string& label, int w) {
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

/*
============================================================
		STACK + QUEUE ANIMATIONS FOR REAL DATA
  Used by the History stack and the Up Next queue screens.
============================================================
*/

// Short text for a cell: optionally drops the "Artist - " prefix and clips with "..".
string cellName(const string& title, int w, bool dropArtist) {
    string t = title;
    size_t p = t.find(" - ");
    if (dropArtist && p != string::npos && p + 3 < t.size()) t = t.substr(p + 3);
    if (utf8Len(t) > w) t = utf8Sub(t, 0, w - 2) + "..";
    return t;
}

// How many queue cells fit on one terminal line.
int queueMaxVisible() {
    int cols, rows;
    getTermSize(cols, rows);
    int m = (cols - 38) / 14;
    return m < 2 ? 2 : (m > 5 ? 5 : m);
}

// An item falls from the top and lands on the stack (the stack keeps stk[0..k-1]).
static void stackFall(string stk[], int& k, const string& item, int w, int delayMs) {
    const int cap = STACK_CAP, lines = cap + 3;
    for (int fr = 0; fr <= cap - k; fr++) {
        moveUp(lines);
        drawStack(stk, k, cap, "PUSH: " + item, item, fr, 0, w);
        SLEEP_MS(delayMs);
    }
    stk[k++] = item;
}

// items[0] = oldest ... items[n-1] = newest. Builds the stack by pushing the newest 5.
void stackBuildAnimation(const string items[], int n, int w) {
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
void stackPopAnimation(const string items[], int n, int w) {
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
void queueBuildAnimation(const string items[], int n, int total) {
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
void queueEnqueueAnimation(const string items[], int n, int total, const string& newItem) {
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
void queueDequeueAnimation(const string items[], int n, int total) {
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
              BOOT SEQUENCE (system check)
============================================================
*/
void bootSequence(int songCount, int userCount, int bucketCount) {
    char b0[64], b1[64], b2[64];
    sprintf(b0, "Allocating hash table (%d buckets)", bucketCount);
    sprintf(b1, "Loading songs.txt (%d tracks)", songCount);
    sprintf(b2, "Loading users.txt (%d accounts)", userCount);
    const string steps[7] = {
        b0, b1, "Indexing library by title", b2,
        "Linking circular playlist", "Preparing Up Next queue and history stack", "Starting dashboard"
    };
    cout << HIDE_CURSOR << "\n" << T().primary << "  ECHO ENGINE :: SYSTEM CHECK" << RESET << "\n\n";
    for (int i = 0; i < 7; i++) {
        int cycles = 6 + rand() % 8;
        for (int c = 0; c < cycles; c++) {
            cout << CLEAR_LINE << "  [" << T().secondary << spinFrame(c) << RESET << "] " << steps[i] << flush;
            SLEEP_MS(70);
        }
        cout << CLEAR_LINE << "  [ " << T().low << "OK" << RESET << " ] " << steps[i] << "\n";
    }
    cout << "\n" << T().low << "  " << GLYPH_CHECK << " All systems ready." << RESET << "\n\n" << SHOW_CURSOR;
}
