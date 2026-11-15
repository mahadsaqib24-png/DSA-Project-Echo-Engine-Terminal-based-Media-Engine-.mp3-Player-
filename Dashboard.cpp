#include "Dashboard.h"

static void menuItem(int n, const char* text) { cout << "   " << T().primary << (n < 10 ? " " : "") << n << RESET << "  " << text << "\n"; }

/*
============================================================
                       Screen control
============================================================
*/
void Dashboard::clearScreen() const {
    // ANSI: wipe the screen AND the scrollback in one go.
    if (g_ansiEnabled && g_scrollClearOk) {
        cout << RESET << "\033[2J\033[3J\033[H" << flush;
        return;
    }
    // Fallback: no ANSI, or a legacy console whose scrollback ESC[3J cannot clear.
    cout << RESET << flush;
    if (!g_stdoutIsTty) return;            // redirected output: nothing to clear
    int rc;
#ifdef _WIN32
    rc = system("cls");
#else
    rc = system("clear");
#endif
    (void)rc;
}

void Dashboard::splash() const {
    clearScreen();
    splashAnimation();
}

void Dashboard::boot(int songs, int users, int buckets) const {
    bootSequence(songs, users, buckets);
}

void Dashboard::displayAuthMenu() const {
    clearScreen();
    printBanner("E C H O   E N G I N E", "Sign in to continue");
    printSectionHeader("WELCOME");
    menuItem(1, "Log in");
    menuItem(2, "Create an account");
    menuItem(0, "Exit");
    cout << "\n  Choose: " << flush;
}

void Dashboard::dashboardIntro(const string& user) const {
    const int W = uiWidth();
    clearScreen();
    cout << HIDE_CURSOR << "\n";
    for (int half = 0; half <= W / 2; half += 2) {
        cout << CLEAR_LINE << T().primary << string((W / 2) - half, ' ') << string(half * 2, '=') << RESET << flush;
        SLEEP_MS(18);
    }
    cout << "\n" << string((W - 21) / 2, ' ');
    typewriter("E C H O   E N G I N E", BOLD_WHITE, 45);
    cout << "\n" << string((W - 31) / 2, ' ');
    typewriter("M A S T E R   D A S H B O A R D", T().secondary, 22);
    cout << "\n" << T().primary << string(W, '=') << RESET << "\n";

    const int BARS = 26;
    int level[BARS];
    for (int i = 0; i < BARS; i++) level[i] = rand() % 8;
    for (int f = 0; f < 22; f++) {
        string eq = "  ";
        for (int i = 0; i < BARS; i++) {
            level[i] += (rand() % 5) - 2;
            if (f > 14 && level[i] > 0) level[i]--;  // settle to a flat line at the end
            if (level[i] < 0) level[i] = 0;
            if (level[i] > 7) level[i] = 7;
            const char* col = level[i] < 3 ? T().low : (level[i] < 6 ? T().mid : T().high);
            eq += string(col) + eqLevel(level[i]) + RESET;
        }
        cout << CLEAR_LINE << eq << flush;
        SLEEP_MS(55);
    }
    cout << CLEAR_LINE << "  " << T().low << GLYPH_CHECK << " Welcome, " << RESET << BOLD_WHITE << user << RESET << "\n" << SHOW_CURSOR << flush;
    SLEEP_MS(600);
}

void Dashboard::loginResult(const string& user, bool ok) const {
    spinnerAnimation("Authenticating " + user + "...", 18);
    if (!ok) { printError("Invalid username or password."); return; }
    cout << T().low << "  " << GLYPH_CHECK << " Access granted: " << RESET << BOLD_WHITE << user << RESET << "\n";
}

void Dashboard::goodbye() const {
    cout << HIDE_CURSOR << "\n  ";
    typewriter("See you next time!", T().primary, 45);
    cout << "\n\n";
    spinnerAnimation("Closing Echo Engine...", 16);
    cout << T().primary << string(uiWidth(), '=') << RESET << "\n" << SHOW_CURSOR << flush;
    SLEEP_MS(1000);
}

void Dashboard::loading(const string& what, bool ok) const {
    loadingAnimation(6, what, ok);
}

void Dashboard::saving(const string& what, bool ok) const {
    savingAnimation(6, what, ok);
}

/*
============================================================
          Main dashboard: banner + playlist window
============================================================
*/
void Dashboard::displayDashboard(const PlaylistEngine& p, const string& name, int index, int total) const {
    clearScreen();
    printBanner("E C H O   E N G I N E", "Terminal Media Engine");

    cout << "  " << T().primary << "ACTIVE PLAYLIST" << RESET;
    if (!name.empty())
        cout << ": " << BOLD_WHITE << name << RESET << DIM << "   (playlist " << index << " of " << total << ")" << RESET;
    cout << "\n";

    if (p.isEmpty()) {
        printInfo("This playlist is empty. Use Search or Browse to add tracks.");
        cout << "\n";
        return;
    }

    int tw, aw;
    listWidths(tw, aw);
    tw -= 1;                                   // numbers are 3 wide (up to 999)
    int n = p.getSize(), cur = p.getCurrentIndex();
    const int WINDOW = PLAYER_DISC_R * 2 + 1;
    int first = 0;
    if (n > WINDOW) {  // keep the current track inside the window
        first = cur - WINDOW / 2;
        if (first < 0) first = 0;
        if (first > n - WINDOW) first = n - WINDOW;
    }
    int last = (first + WINDOW < n) ? first + WINDOW : n;

    cout << DIM << "  " << n << " track" << (n == 1 ? "" : "s") << ", " << formatTime(p.totalSeconds())
         << " in total, loops tail <-> head" << RESET << "\n";
    cout << DIM << string(5, ' ') << " #  " << padTo("TITLE", tw) << " " << padTo("ARTIST", aw) << " TIME" << RESET << "\n";
    if (first > 0) cout << DIM << "      ... " << first << " earlier\n" << RESET;
    for (int i = first; i < last; i++) {
        Song s = p.getAt(i);
        bool isCur = (i == cur);
        cout << "  " << (isCur ? string(T().mid) + GLYPH_PLAY + " " : string("  "))
             << (isCur ? BOLD_WHITE : WHITE)
             << padNum(i + 1, 3) << ". " << padTo(s.title, tw) << RESET << " "
             << DIM << padTo(s.artist, aw) << " " << formatTime(s.durationSeconds) << RESET << "\n";
    }
    if (last < n) cout << DIM << "      ... " << (n - last) << " more\n" << RESET;
    cout << "\n";
}

void Dashboard::displayMenu() const {
    printSectionHeader("MASTER DASHBOARD");
    menuItem(1, "Play my playlist");
    menuItem(2, "Search songs");
    menuItem(3, "Browse library");
    menuItem(4, "My playlists");
    menuItem(5, "Up Next queue");
    menuItem(6, "Listening history");
    menuItem(7, "Save playlists");
    menuItem(8, "Change colour theme");
    menuItem(9, "Upload new track");
    menuItem(10, "Rescan music folder");
    menuItem(11, "Help");
    menuItem(0, "Logout");
    cout << "\n  Choose: " << flush;
}

/*
============================================================
                   Help pages (option 11)
============================================================
*/
static void helpItem(int n, const char* title, const char* l1, const char* l2 = "") {
    cout << "  " << T().primary << (n < 10 ? " " : "") << n << RESET << "  " << BOLD_WHITE << title << RESET << "\n"
         << DIM << "        " << l1 << "\n";
    if (*l2) cout << "        " << l2 << "\n";
    cout << RESET << "\n";
}

void Dashboard::displayHelp(int page) const {
    clearScreen();
    printSectionHeader("HELP: THE MASTER DASHBOARD  (page " + intStr(page + 1) + " of " + intStr(HELP_PAGES) + ")");
    cout << "\n";
    if (page == 0) {
        helpItem(1, "Play my playlist",
                 "Opens the full-screen player on the current track of your",
                 "active playlist. Songs in Up Next play first.");
        helpItem(2, "Search songs",
                 "Type part of a title, artist or album. Spelling mistakes are fine.",
                 "Pick a result to play it, add it to a playlist or to Up Next.");
        helpItem(3, "Browse library",
                 "Every song in the library sorted A-Z, 8 per page. Use n / p for",
                 "the next or previous page, or type a number to pick a song.");
        helpItem(4, "My playlists",
                 "Up to 10 playlists per user. Create, rename, delete, choose the",
                 "active one, or open one to remove, move and add tracks.");
    } else if (page == 1) {
        helpItem(5, "Up Next queue",
                 "Songs waiting to play before the playlist continues (first in,",
                 "first out), shown as a queue animation. d removes the next song.");
        helpItem(6, "Listening history",
                 "Songs played this session, newest first (a stack: last in, first",
                 "out), shown as a stack animation. p removes the latest entry.");
        helpItem(7, "Save playlists",
                 "Saves all your playlists right now. They are also saved",
                 "automatically when you log out.");
        helpItem(8, "Change colour theme",
                 "Neon, Ocean, Retro Green or Sunset. Press h in the player too.");
    } else if (page == 2) {
        helpItem(9, "Upload new track",
                 "Enter a title, artist, album and the path of an .mp3 / .wav file.",
                 "It is copied into music/ and added to the library for everyone.");
        helpItem(10, "Rescan music folder",
                 "Adds audio files you copied into music/ and fixes wrong song",
                 "lengths. Title = file name, artist = the text before \" - \".");
        helpItem(11, "Help", "This screen.");
        helpItem(0, "Logout",
                 "Saves your playlists, clears Up Next and the history, and returns",
                 "to the login screen.");
    } else {
        cout << "  " << T().primary << "PLAYER KEYS" << RESET << DIM << "  (no Enter needed)" << RESET << "\n\n"
             << "   space  play / pause             s  shuffle on / off\n"
             << "   n      next track               r  repeat: off, all, one\n"
             << "   p      previous track           +  -  volume up / down\n"
             << "          (restarts it after 3 s)  t  sleep timer: 15, 30, 60 min\n"
             << "   h      change theme             q  close the player\n\n"
             << "  " << T().primary << "PLAYBACK ORDER" << RESET << "\n\n"
             << "   1. Songs in Up Next play first.\n"
             << "   2. Then your active playlist continues and loops from the last\n"
             << "      song back to the first.\n"
             << "   3. With repeat off, the player stops after the last song.\n\n";
    }
}

void Dashboard::displaySongList(Song* const list[], int n, int firstNumber, const string& heading) const {
    int tw, aw;
    listWidths(tw, aw);
    tw -= 1;                                   // numbers are 3 wide (up to 999)
    printSectionHeader(heading);
    cout << DIM << "    #  " << padTo("TITLE", tw) << " " << padTo("ARTIST", aw) << " TIME" << RESET << "\n";
    for (int i = 0; i < n; i++) {
        int num = firstNumber + i;
        cout << "  " << T().primary << padNum(num, 3) << RESET << ". "
             << BOLD_WHITE << padTo(list[i]->title, tw) << RESET << " "
             << T().secondary << padTo(list[i]->artist, aw) << RESET << " "
             << DIM << formatTime(list[i]->durationSeconds) << RESET << "\n";
    }
    cout << "\n";
}

void Dashboard::displayThemes() const {
    const Theme* t = themeTable();
    printSectionHeader("COLOUR THEMES");
    for (int i = 0; i < THEME_COUNT; i++) {
        cout << "  " << (i + 1) << ". " << padTo(t[i].name, 13)
             << t[i].primary << GLYPH_FULL << GLYPH_FULL << t[i].secondary << GLYPH_FULL << GLYPH_FULL
             << t[i].low << GLYPH_FULL << GLYPH_FULL << t[i].mid << GLYPH_FULL << GLYPH_FULL
             << t[i].high << GLYPH_FULL << GLYPH_FULL << RESET
             << (i == themeIndex() ? "  (current)" : "") << "\n";
    }
}

/*
============================================================
               Playlists (up to 10 per user)
============================================================
*/
void Dashboard::displayPlaylists(const PlaylistCollection& pc, const string& heading) const {
    printSectionHeader(heading);
    for (int i = 0; i < pc.getCount(); i++) {
        const PlaylistEngine& p = pc.at(i);
        bool act = (i == pc.getActive());
        cout << "  " << (act ? string(T().mid) + GLYPH_PLAY + " " : string("  ")) << (act ? BOLD_WHITE : WHITE)
             << (i + 1 < 10 ? " " : "") << (i + 1) << ". " << padTo(pc.nameAt(i), MAX_PLAYLIST_NAME) << RESET << "  "
             << DIM << p.getSize() << " track" << (p.getSize() == 1 ? " " : "s") << "   " << formatTime(p.totalSeconds())
             << (act ? "   (active)" : "") << RESET << "\n";
    }
    cout << DIM << "\n  " << pc.getCount() << " of " << MAX_PLAYLISTS << " playlists used.   "
         << GLYPH_PLAY << " marks the active playlist (the one that plays)." << RESET << "\n\n";
}

void Dashboard::displayPlaylistEditor(const PlaylistEngine& p, const string& name, int index, int total,
                                      int page, int perPage) const {
    clearScreen();
    printBanner("E D I T   P L A Y L I S T", name);
    int n = p.getSize();
    int pages = (n + perPage - 1) / perPage;
    if (pages < 1) pages = 1;
    cout << "  " << DIM << "Playlist " << index << " of " << total << "   |   " << n << " track" << (n == 1 ? "" : "s")
         << "   |   " << formatTime(p.totalSeconds()) << " in total   |   page " << (page + 1) << "/" << pages << RESET << "\n\n";
    if (n == 0) { printInfo("This playlist is empty. Choose 4 or 5 below to add songs."); cout << "\n"; return; }

    int tw, aw;
    listWidths(tw, aw);
    tw -= 1;                                   // numbers are 3 wide (up to 999)
    cout << DIM << "    #  " << padTo("TITLE", tw) << " " << padTo("ARTIST", aw) << " TIME" << RESET << "\n";
    int first = page * perPage, last = (first + perPage < n) ? first + perPage : n;
    for (int i = first; i < last; i++) {
        Song s = p.getAt(i);
        cout << "  " << T().primary << padNum(i + 1, 3) << RESET << ". " << BOLD_WHITE << padTo(s.title, tw)
             << RESET << " " << T().secondary << padTo(s.artist, aw) << RESET << " " << DIM << formatTime(s.durationSeconds) << RESET << "\n";
    }
    cout << "\n";
}

/*
====================================================================
  Up Next queue and History stack, animated with the real contents
====================================================================
*/
void Dashboard::queueView(const Song items[], int n, int total) const {
    string nm[QUEUE_VIEW_MAX];
    for (int i = 0; i < n && i < QUEUE_VIEW_MAX; i++) nm[i] = items[i].title;
    queueBuildAnimation(nm, n < QUEUE_VIEW_MAX ? n : QUEUE_VIEW_MAX, total);
}

void Dashboard::queueEnqueue(const Song items[], int n, int total, const Song& added) const {
    string nm[QUEUE_VIEW_MAX];
    for (int i = 0; i < n && i < QUEUE_VIEW_MAX; i++) nm[i] = items[i].title;
    queueEnqueueAnimation(nm, n < QUEUE_VIEW_MAX ? n : QUEUE_VIEW_MAX, total, added.title);
}

void Dashboard::queueDequeue(const Song items[], int n, int total) const {
    string nm[QUEUE_VIEW_MAX];
    for (int i = 0; i < n && i < QUEUE_VIEW_MAX; i++) nm[i] = items[i].title;
    queueDequeueAnimation(nm, n < QUEUE_VIEW_MAX ? n : QUEUE_VIEW_MAX, total);
}

// items[0] is the newest song, but the animation pushes oldest first, so the order is reversed here.
void Dashboard::stackView(const Song items[], int n) const {
    int use = n < STACK_CAP ? n : STACK_CAP;
    string nm[STACK_CAP];
    for (int i = 0; i < use; i++) nm[i] = items[use - 1 - i].title;
    stackBuildAnimation(nm, use);
}

void Dashboard::stackPop(const Song items[], int n) const {
    int use = n < STACK_CAP ? n : STACK_CAP;
    string nm[STACK_CAP];
    for (int i = 0; i < use; i++) nm[i] = items[use - 1 - i].title;
    stackPopAnimation(nm, use);
}

/*
==============================================================
  Now Playing: short disc intro before the full-screen player
==============================================================
*/
void Dashboard::displayNowPlaying(const Song& s) const {
    clearScreen();
    cout << "\n";
    spinningDiscAnimation(s.title, s.artist, s.album, s.durationSeconds, 2);
}

/*
========================================================================
  Persistent player screen (spinning disc + equalizer + progress bar)
  Redrawn from the top-left every frame, so resizing the window is safe.
========================================================================
*/
void Dashboard::enterPlayer() const {
    rawModeOn();
    cout << ALT_SCREEN_ON << HIDE_CURSOR << CLEAR_SCREEN << flush;  // alternate screen
    if (!g_ansiEnabled) clearScreen();
}

void Dashboard::leavePlayer() const {
    rawModeOff();
    cout << ALT_SCREEN_OFF << SHOW_CURSOR << flush;
}

void Dashboard::startSlide(PlayerView& v, const string& fromTitle, bool forward) const {
    v.prevTitle = fromTitle;
    v.slide = 0;
    v.slideDir = forward ? 1 : -1;
}

// Pads every line of a frame with spaces (used when ESC[K is not available).
static string padFrameLines(const string& f, int width) {
    string out;
    size_t start = 0;
    while (start <= f.size()) {
        size_t nl = f.find('\n', start);
        bool last = (nl == string::npos);
        string line = f.substr(start, last ? string::npos : nl - start);
        int len = utf8Len(line);
        out += line;
        if (len < width) out += string(width - len, ' ');
        if (last) break;
        out += "\n";
        start = nl + 1;
    }
    return out;
}

// The whole frame goes out in ONE write: no flicker, even on the Windows console.
static void writeFrame(string frame, int cols) {
    if (!g_ansiEnabled) {
        frame = padFrameLines(frame, cols - 1);
        consoleCursorHome();
    }
    cout << frame << flush;
}

void Dashboard::drawPlayerFrame(PlayerView& v) const {
    const int R = PLAYER_DISC_R, ROWS = 2 * R + 1, EQH = PLAYER_EQ_H;
    const int MIN_ROWS = ROWS + PLAYER_EXTRA_ROWS, MIN_COLS = PLAYER_MIN_COLS;
    const int SLIDE_W = PLAYER_SLIDE_W;

    // the disc eases up to speed, eases down when paused, and jerks back and forth on a skip
    double target = (v.paused || v.ended) ? 0.0 : 0.38;
    v.spin += (target - v.spin) * 0.2;
    if (v.scratch > 0) { v.theta += ((v.scratch % 2) ? -0.9 : 0.9); v.scratch--; }
    else v.theta += v.spin;
    while (v.theta < 0) v.theta += 2 * ANIM_PI;
    while (v.theta > 2 * ANIM_PI) v.theta -= 2 * ANIM_PI;

    int maxLevel = (EQH * v.volume + 5) / 10;  // bar height follows the volume
    bool active = !v.paused && !v.ended;
    for (int i = 0; i < PLAYER_BARS; i++) {
        if (active) {
            v.level[i] += (rand() % 3) - 1;
            if (rand() % 6 == 0) v.level[i] += (rand() % 5) - 2;
            if (v.scratch > 0) v.level[i] = rand() % (maxLevel + 1);
        } else if (v.level[i] > 0) v.level[i]--;
        if (v.level[i] < 0) v.level[i] = 0;
        if (v.level[i] > maxLevel) v.level[i] = maxLevel;
    }

    string f;                                     // the frame is built here, then written once
    int cols, rows;
    getTermSize(cols, rows);
    if (cols != v.lastCols || rows != v.lastRows) {
        if (g_ansiEnabled) f += CLEAR_SCREEN;
        else clearScreen();
        v.lastCols = cols; v.lastRows = rows;
    }
    f += CURSOR_HOME;

    if (cols < MIN_COLS || rows < MIN_ROWS) {
        f += string("  Window too small") + ERASE_EOL + "\n  need " + intStr(MIN_COLS) + "x" + intStr(MIN_ROWS)
           + ", have " + intStr(cols) + "x" + intStr(rows) + ERASE_EOL + ERASE_BELOW;
        writeFrame(f, cols);
        v.frame++;
        return;
    }

    int avail = cols - 23;
    int bars  = avail / 3;  if (bars > PLAYER_BARS) bars = PLAYER_BARS;
    int tw    = cols - 6;   if (tw > 50) tw = 50;
    int bw    = avail - 15; if (bw > 16) bw = 16; if (bw < 4) bw = 4;

    // the title slides in on a skip and scrolls otherwise
    string titleLine;
    if (v.slide >= 0) {
        int W = (tw < SLIDE_W) ? tw : SLIDE_W;
        string a = padTo(v.prevTitle, W), b = padTo(v.song.title, W);
        int off = (v.slideDir > 0) ? v.slide : W - v.slide;
        titleLine = utf8Sub((v.slideDir > 0) ? a + b : b + a, off, W);
        v.slide += 3;
        if (v.slide > W) v.slide = -1;
    } else {
        titleLine = scrollText(v.song.title, tw, v.frame);
    }
    f += string("  ") + T().primary + GLYPH_NOTE + RESET + " " + BOLD_WHITE + titleLine + RESET + ERASE_EOL + "\n";
    f += string(ERASE_EOL) + "\n";

    string right[ROWS];
    string headText = v.ended ? string("END OF PLAYLIST")
                    : v.paused ? string("|| PAUSED")
                    : string(GLYPH_NOTE) + (v.fromQueue ? " UP NEXT" : " NOW PLAYING");
    string idx = v.fromQueue ? string("   [queued]") : "   [" + intStr(v.index) + "/" + intStr(v.total) + "]";
    right[0] = string(v.ended || v.paused ? T().mid : T().primary) + headText + RESET + DIM + idx + RESET;
    right[1] = string(DIM) + T().secondary + clipStr(v.song.artist + "  |  " + v.song.album, avail) + RESET;
    right[2] = string("Shuffle ") + (v.shuffle ? string(T().low) + "ON " : string(DIM) + "off") + RESET +
               "  Repeat " + (v.repeatMode == 0 ? string(DIM) + "off" :
                               v.repeatMode == 1 ? string(T().low) + "ALL" : string(T().mid) + "ONE") + RESET;
    string q = "Up Next: " + intStr(v.queueCount) + " queued" + (v.hasAudio ? "" : "  (simulated)");
    right[3] = string(DIM) + clipStr(q, avail) + RESET;
    for (int k = 0; k < EQH; k++) right[4 + k] = eqRow(v.level, bars, EQH, k);
    int el = (int)v.elapsed;
    right[8] = string(T().low) + (v.paused || v.ended ? "||" : GLYPH_PLAY) + RESET + " " + formatTime(el) + " " +
               makeBar(el, v.song.durationSeconds, bw) + " " + formatTime(v.song.durationSeconds);

    for (int ry = -R; ry <= R; ry++)
        f += "  " + discRow(ry, R, v.theta) + "   " + right[ry + R] + ERASE_EOL + "\n";
    f += string(ERASE_EOL) + "\n";

    const string helps[3] = {
        "[space]play/pause [n]ext [p]rev [s]huffle [r]epeat [+/-]vol [t]imer [h]theme [q]uit",
        "spc pause  n next  p prev  s shuf  r rep  +/- vol  t timer  h theme  q quit",
        "spc n p s r +/- t h q" };
    string help = helps[2];
    for (int h = 0; h < 3; h++) if ((int)helps[h].size() + 3 <= cols) { help = helps[h]; break; }
    f += string(DIM) + "  " + help + RESET + ERASE_EOL + "\n" + ERASE_EOL + "\n";

    string volText = (v.volume == 0) ? string("MUTE") : intStr(v.volume * 10) + "%";
    int sl = (int)v.sleepLeft;
    string sleepText = "off";
    if (sl > 0) sleepText = intStr(sl / 60) + "m " + (sl % 60 < 10 ? "0" : "") + intStr(sl % 60) + "s";
    f += "  Vol [" + makeBar(v.volume, 10, 10) + "] " + volText + "     Sleep: " + sleepText + ERASE_EOL + "\n";

    if (v.scratch > 0) f += string(T().mid) + "  *scratch*" + RESET;
    else if (v.msgTtl > 0) { f += string(T().secondary) + "  " + clipStr(v.msg, cols - 4) + RESET; v.msgTtl--; }
    f += string(ERASE_EOL) + ERASE_BELOW;

    writeFrame(f, cols);
    v.frame++;
}
