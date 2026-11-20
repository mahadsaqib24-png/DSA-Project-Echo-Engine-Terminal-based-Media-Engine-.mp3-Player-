#ifndef DEMOS_H
#define DEMOS_H

/*
============================================================
        ECHO ENGINE: DEMO ANIMATIONS (not used by the app)
  Moved out of animations.h. Included by AnimationLab.cpp.
  Header-only; link AnimationLab with animations.cpp and
  colours.cpp.
============================================================
*/

#include "animations.h"
#include "FileUtils.h"

/*
============================================================
                      LOGIN RESULT
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
      NOW PLAYING (horizontal equalizer + progress)
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
            eq += string(col) + eqLevel(level[i]) + eqLevel(level[i]) + " ";
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
                         MARQUEE
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
                    TRACK TRANSITION
============================================================
*/
inline void trackTransition(const string& from, const string& to, bool forward, int W = 34) {
    string a = padTo(from, W), b = padTo(to, W);
    string strip = forward ? a + b : b + a;
    cout << HIDE_CURSOR;
    for (int step = 0; step <= W; step++) {
        int offset = forward ? step : W - step;
        cout << CLEAR_LINE << "  " << (forward ? string("") : string(GLYPH_BIARROW) + " ") << BOLD_WHITE
             << strip.substr(offset, W) << RESET << flush;
        SLEEP_MS(18);
    }
    cout << "\n" << SHOW_CURSOR;
}

/*
============================================================
               CIRCULAR LIST POINTER WALK
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
                 HASH LOOKUP VISUALISER
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
    for (int i = 0; i < shown; i++) cout << (i == target ? string(T().mid) + " " + GLYPH_PTR + " " + RESET : string("    "));
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
   STACK ANIMATION (push / pop, like Recently Played)
============================================================
*/
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
   QUEUE ANIMATION (enqueue / dequeue, like Up Next)
============================================================
*/
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
                     THEME SELECTOR
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
    cout << "\n  Choose theme (1-" << THEME_COUNT << "): " << flush;
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

/*
============================================================
     LIVE PLAYER (the original stand-alone version)
  Full-screen and resize-safe: keyboard controls, shuffle, repeat,
  volume, sleep timer, vinyl scratch on skip. The real player is
  PlayerEngine.cpp; this one plays made-up tracks.
============================================================
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
        Unused file helper (moved out of FileUtils.h)
============================================================
*/
// If the file is missing, create it empty and print a warning (filing rule 2).
// Returns true if it had to be created.
inline bool ensureFileExists(const string& path) {
    if (fileExists(path)) return false;
    ofstream f(path.c_str());
    cout << "  [~] " << path << " not found: created an empty file.\n";
    return true;
}

#endif
