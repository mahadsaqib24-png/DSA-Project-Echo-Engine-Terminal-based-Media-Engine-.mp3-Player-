#include "MediaPlayerApp.h"
#include "AppPaths.h"
#include <ctime>

static string plural(int n, const string& word) { return intStr(n) + " " + word + (n == 1 ? "" : "s"); }

static bool saidYes(const string& s) { return !s.empty() && (s[0] == 'y' || s[0] == 'Y'); }

MediaPlayerApp::MediaPlayerApp() : library(HASH_INITIAL_SIZE), player(plists, manager, library, ui), inputClosed(false) {}

/*
====================================================================
  Input helpers (getline only, so cin is never left half-consumed)
====================================================================
*/
string MediaPlayerApp::readLine(const string& prompt) {
    cout << prompt << flush;
    string s;
#ifdef _WIN32
    if (stdinIsConsole()) {                       // ReadConsoleW keeps accented letters intact under code page 65001
        if (!readConsoleLineUtf8(s)) { inputClosed = true; return ""; }
        return trim(s);
    }
#endif
#ifndef _WIN32
    if (!readStdinLine(s)) { inputClosed = true; return ""; }       // no read-ahead, see FileUtils.h
#else
    if (!getline(cin, s)) { inputClosed = true; return ""; }        // piped / redirected input
#endif
    return trim(stripCR(s));
}

int MediaPlayerApp::readInt(const string& prompt) {
    string s = readLine(prompt);
    if (inputClosed) return 0;
    int v;
    return toInt(s, v) ? v : -1;
}

string MediaPlayerApp::readPassword(const string& prompt) {
    cout << prompt << flush;
    string pw;
    rawModeOn();
    while (true) {
        int k = readKey();
        if (k < 0) {
#ifdef _WIN32
            continue;  // arrow / function key
#else
            inputClosed = true;  // EOF
            break;
#endif
        }
        if (k == '\r' || k == '\n') break;
        if (k == 8 || k == 127) {
            if (!pw.empty()) { pw.erase(pw.size() - 1); cout << "\b \b" << flush; }
        } else if (k >= 32 && k < 127) {
            pw += (char)k;
            cout << '*' << flush;
        }
    }
    rawModeOff();
    cout << "\n";
    return pw;
}

void MediaPlayerApp::waitForEnter() {
    readLine(string("\n  ") + DIM + "Press Enter to continue..." + RESET);
}

/*
============================================================
                    First-run data files
============================================================
*/
void MediaPlayerApp::seedDataFiles() {
    ensureDataDirs();
    makeDir("music");

    string sp = dataPath("songs.txt");
    if (!fileExists(sp)) {
        ofstream f(sp.c_str());
        f << "# Title|Artist|Album|DurationSecs\n"
          << "Blinding Lights|The Weeknd|After Hours|200\n"
          << "Shape of You|Ed Sheeran|Divide|234\n"
          << "Believer|Imagine Dragons|Evolve|204\n";
        printInfo("songs.txt not found: created with 3 sample tracks (No Audio).");
    }
    string up = dataPath("users.txt");
    if (!fileExists(up)) {
        ofstream f(up.c_str());
        f << "# Username|Password\n"
          << "user1|1234\n"
          << "user2|1234\n";
        printInfo("users.txt not found: created demo accounts user1 / user2 (password 1234).");
    }
}

/*
================================================================================
  run(): boot -> (log in | create account) -> master dashboard -> logout loop
================================================================================
*/
void MediaPlayerApp::run() {
    initTerminal();

    string home;
    bool fallback = false;
    bool writable = setupWorkingDirectory(home, fallback);

    ui.splash();
    if (!writable)
        printError("No writable folder found: saving will not work. Move Echo Engine to a normal folder.");
    else if (fallback)
        printWarn("The program folder is protected. Using: " + home);
    else
        printInfo("Data folder: " + home);
    seedDataFiles();

    ui.loading("song database");
    library.loadDatabaseFromFile(dataPath("songs.txt"));
    manager.loadUsers(dataPath("users.txt"));
    syncMusicFolder();
    ui.boot(library.getCount(), manager.userCount(), library.getBucketCount());

    if (AudioPlayer::isSupported()) printInfo("Audio engine: " + AudioPlayer::backendName());
    else                            printWarn(AudioPlayer::installHint());

    while (!inputClosed) {
        if (!authScreen()) break;

        startSession();
        handleMenu();
        endSession();

        string again = readLine("\n  Log in as another user? (y/n): ");
        if (!saidYes(again)) break;
    }

    cout << SHOW_CURSOR;
    printInfo("Thanks for listening. Bye!");
    ui.goodbye();
}

bool MediaPlayerApp::authScreen() {
    while (!inputClosed) {
        ui.displayAuthMenu();
        int c = readInt("");
        if (c == 0) return false;
        if (c == 1 && login())              return true;
        if (c == 2 && createAccount())      return true;
        if (c < 0 || c > 2) { printWarn("Please choose 1, 2 or 0."); SLEEP_MS(PAUSE_SHORT_MS); }
    }
    return false;
}

/*
==================================================================
  login(): 3 attempts, masked password, checked against UserList.
==================================================================
*/
bool MediaPlayerApp::login() {
    ui.clearScreen();
    printBanner("L O G I N", "Sign in to Echo Engine");
    printInfo("Leave the username blank to go back.");

    for (int attempt = 1; attempt <= LOGIN_ATTEMPTS && !inputClosed; attempt++) {
        string user = readLine("  Username: ");
        if (inputClosed || user.empty()) return false;
        string pass = readPassword("  Password: ");
        if (inputClosed) return false;

        bool ok = manager.login(user, pass);
        ui.loginResult(user, ok);
        if (ok) {
            currentUser = user;
            SLEEP_MS(PAUSE_SHORT_MS);
            return true;
        }
        if (attempt < LOGIN_ATTEMPTS) printWarn("Try again (" + plural(LOGIN_ATTEMPTS - attempt, "attempt") + " left).");
    }
    if (!inputClosed) { printError("Too many failed attempts."); SLEEP_MS(PAUSE_MEDIUM_MS); }
    return false;
}

/*
==============================================================================
  createAccount(): username 3-16 letters / digits / underscore, password 4-32
  characters without spaces or '|'. The new account is saved and logged in.
==============================================================================
*/
static bool validUsername(const string& u, string& why) {
    if ((int)u.size() < USERNAME_MIN || (int)u.size() > USERNAME_MAX) { why = "Username must be " + intStr(USERNAME_MIN) + " to " + intStr(USERNAME_MAX) + " characters."; return false; }
    for (size_t i = 0; i < u.size(); i++)
        if (!isalnum((unsigned char)u[i]) && u[i] != '_') { why = "Use only letters, digits and underscore."; return false; }
    return true;
}

static bool validPassword(const string& p, string& why) {
    if ((int)p.size() < PASSWORD_MIN) { why = "Password must be at least " + intStr(PASSWORD_MIN) + " characters."; return false; }
    if ((int)p.size() > PASSWORD_MAX) { why = "Password must be at most " + intStr(PASSWORD_MAX) + " characters."; return false; }
    for (size_t i = 0; i < p.size(); i++)
        if (p[i] == ' ' || p[i] == FIELD_DELIM) { why = "Password cannot contain spaces or the | character."; return false; }
    return true;
}

bool MediaPlayerApp::createAccount() {
    ui.clearScreen();
    printBanner("C R E A T E   A C C O U N T", "Join Echo Engine");
    printInfo("Username: 3-16 letters, digits or _.   Password: 4+ characters.");
    printInfo("Leave the username blank to go back.");
    cout << "\n";

    string user, pass, why;
    while (!inputClosed) {
        user = readLine("  Choose a username: ");
        if (inputClosed || user.empty()) return false;
        if (!validUsername(user, why)) { printError(why); continue; }
        spinnerAnimation("Checking availability...", 10);
        if (manager.userExists(user)) { printError("That username is already taken."); continue; }
        printSuccess("\"" + user + "\" is available.");
        break;
    }
    while (!inputClosed) {
        pass = readPassword("  Choose a password: ");
        if (inputClosed) return false;
        if (!validPassword(pass, why)) { printError(why); continue; }
        string again = readPassword("  Confirm password: ");
        if (inputClosed) return false;
        if (again != pass) { printError("Passwords do not match. Try again."); continue; }
        break;
    }
    if (inputClosed) return false;

    spinnerAnimation("Creating your account...", 16);
    bool ok = manager.registerUser(user, pass, dataPath("users.txt"));
    ui.saving("users.txt", ok);
    if (!ok) { printError("Could not save the account. Is the data folder writable?"); waitForEnter(); return false; }

    printSuccess("Account created. Welcome to Echo Engine, " + user + "!");
    SLEEP_MS(PAUSE_MEDIUM_MS);
    currentUser = user;
    return true;
}

/*
============================================================
       Session start / end (per-user playlist filing)
============================================================
*/
void MediaPlayerApp::startSession() {
    plists.loadFromFile(currentUser);
    loadSettings();
    for (int i = 0; i < plists.getCount(); i++) {  // saved lengths may be stale: use the library's corrected ones
        PlaylistEngine& p = plists.at(i);
        for (int j = 0; j < p.getSize(); j++) {
            Song saved = p.getAt(j);
            Song* lib = library.findSong(saved.artist, saved.title);
            if (lib != NULL && lib->durationSeconds != saved.durationSeconds) p.setDuration(j, lib->durationSeconds);
        }
    }
    ui.loading("your playlists");
    int tracks = 0;
    for (int i = 0; i < plists.getCount(); i++) tracks += plists.at(i).getSize();
    if (tracks > 0) {
        printSuccess("Restored " + plural(tracks, "track") + " in " + plural(plists.getCount(), "playlist") + " from your saved file.");
        SLEEP_MS(PAUSE_SHORT_MS);
    }
}

void MediaPlayerApp::endSession() {
    plists.saveToFile(currentUser);
    bool ok = fileExists(PlaylistEngine::playlistPath(currentUser));
    ui.saving("playlists", ok);
    saveSettings();
    player.stopAudio();
    plists.destroy();
    manager.resetSession();
    currentUser = "";
    themeIndex() = 0;                       // the login screen always uses the default theme
}

/*
============================================================
                   Master Dashboard
============================================================
*/
void MediaPlayerApp::handleMenu() {
    ui.dashboardIntro(currentUser);
    while (!inputClosed) {
        showDashboard();
        ui.displayMenu();
        int c = readInt("");
        if (c == 0) return;
        if (!handleChoice(c)) { printWarn("Unknown option."); SLEEP_MS(PAUSE_SHORT_MS); }
    }
}

// Runs one menu choice. Returns false if the number is not a valid option.
bool MediaPlayerApp::handleChoice(int c) {
    switch (c) {
        case 1:  playerSession(); return true;
        case 2:  searchFlow();    return true;
        case 3:  browseFlow();    return true;
        case 4:  playlistHub();   return true;
        case 5:  upNextScreen();  return true;
        case 6:  historyScreen(); return true;
        case 7:  savePlaylist();  return true;
        case 8:  changeTheme();   return true;
        case 9:  uploadTrack();   return true;
        case 11: helpScreen();    return true;
        case 10:
            ui.clearScreen();
            printSectionHeader("RESCAN MUSIC FOLDER");
            if (syncMusicFolder() == 0) printInfo("No new audio files found in music/.");
            waitForEnter();
            return true;
    }
    return false;
}

/*
================================================================
  Search (hash lookup + substring + Levenshtein typo tolerance)
================================================================
*/
void MediaPlayerApp::searchFlow() {
    ui.clearScreen();
    printSectionHeader("SEARCH");
    string q = readLine("  Song, artist or album (typos are fine): ");
    if (q.empty()) return;

    Song* res[SEARCH_RESULTS];
    spinnerAnimation("Searching library...", 10);
    int n = library.searchRanked(q, res, SEARCH_RESULTS);
    if (n == 0) { printWarn("No matches for \"" + q + "\"."); waitForEnter(); return; }

    ui.displaySongList(res, n, 1, "RESULTS");
    int pick = readInt("  Pick a number (0 = back): ");
    if (pick >= 1 && pick <= n) { Song s = *res[pick - 1]; songActions(s); }
}

/*
============================================================
     Browse: whole library sorted by title, "n" per page
============================================================
*/
void MediaPlayerApp::browseFlow() {
    int total = library.getCount();
    if (total == 0) { printWarn("The library is empty."); waitForEnter(); return; }

    Song** all = new Song*[total];
    int n = library.getAllSorted(all, total);
    const int PAGE = BROWSE_PAGE_SIZE;
    int pages = (n + PAGE - 1) / PAGE, page = 0;

    while (!inputClosed) {
        ui.clearScreen();
        int first = page * PAGE;
        int cnt = (n - first < PAGE) ? n - first : PAGE;
        ui.displaySongList(all + first, cnt, first + 1,
                           "LIBRARY  (page " + intStr(page + 1) + "/" + intStr(pages) + ", " + intStr(n) + " tracks)");
        string in = readLine("  Number to select, [n]ext, [p]rev, 0 = back: ");
        if (in == "0" || in.empty()) break;
        if (in == "n" || in == "N") { if (page < pages - 1) page++; continue; }
        if (in == "p" || in == "P") { if (page > 0) page--; continue; }
        int v;
        if (toInt(in, v) && v >= 1 && v <= n) { Song s = *all[v - 1]; songActions(s); }
    }
    delete[] all;
}

/*
============================================================
              What to do with one chosen song
============================================================
*/
PlaylistEngine& MediaPlayerApp::pl() { return plists.current(); }

void MediaPlayerApp::showDashboard() {
    ui.displayDashboard(pl(), plists.nameAt(plists.getActive()), plists.getActive() + 1, plists.getCount());
}

void MediaPlayerApp::addToPlaylist(const Song& s, int idx) {
    PlaylistEngine& p = plists.at(idx);
    if (p.contains(s)) printWarn("\"" + s.title + "\" is already in \"" + plists.nameAt(idx) + "\".");
    else { p.addSong(s); printSuccess("Added to \"" + plists.nameAt(idx) + "\": " + s.title); }
}

// Enter picks the active playlist; with only one playlist there is nothing to ask.
int MediaPlayerApp::choosePlaylist(const string& heading) {
    if (plists.getCount() == 1) return 0;
    ui.displayPlaylists(plists, heading);
    string in = readLine("  Playlist number (Enter = active, 0 = cancel): ");
    if (inputClosed) return -1;
    if (in.empty()) return plists.getActive();
    int v;
    if (!toInt(in, v) || v == 0) return -1;
    if (v < 1 || v > plists.getCount()) { printWarn("No such playlist."); SLEEP_MS(PAUSE_SHORT_MS); return -1; }
    return v - 1;
}

void MediaPlayerApp::songActions(const Song& s) {
    ui.clearScreen();
    printSectionHeader(s.title);
    cout << "  " << T().secondary << s.artist << RESET << DIM << "  |  " << s.album << "  |  "
         << formatTime(s.durationSeconds) << RESET << "\n\n"
         << "   1  Play now                    (adds it to the active playlist and starts the player)\n"
         << "   2  Add to a playlist           (you choose which one)\n"
         << "   3  Add to the Up Next queue    (plays before the playlist continues)\n"
         << "   0  Back\n";
    int c = readInt("\n  Choose: ");
    if (c == 1) {
        addToPlaylist(s, plists.getActive());
        pl().jumpTo(s);
        playerSession();
    } else if (c == 2) {
        int i = choosePlaylist("ADD TO WHICH PLAYLIST?");
        if (i >= 0) { addToPlaylist(s, i); SLEEP_MS(PAUSE_MEDIUM_MS); }
    } else if (c == 3) {
        Song items[QUEUE_VIEW_MAX];
        int n = manager.queueSnapshot(items, QUEUE_VIEW_MAX), total = manager.upNextCount();
        cout << "\n";
        ui.queueEnqueue(items, n, total, s);
        manager.addToUpNext(s);
        printSuccess("Queued: " + s.title);
        SLEEP_MS(PAUSE_SHORT_MS);
    }
}

/*
============================================================
       Up Next queue (FIFO) and History stack (LIFO)
============================================================
*/
void MediaPlayerApp::upNextScreen() {
    while (!inputClosed) {
        ui.clearScreen();
        printSectionHeader("UP NEXT  (queue: first in, first out)");
        Song items[QUEUE_VIEW_MAX];
        int n = manager.queueSnapshot(items, QUEUE_VIEW_MAX), total = manager.upNextCount();
        if (total == 0) {
            printInfo("Up Next is empty.");
            printInfo("Search or browse a song and choose \"Add to the Up Next queue\".");
            waitForEnter();
            return;
        }
        cout << "\n";
        ui.queueView(items, n, total);
        cout << "\n";
        manager.showUpNext();
        cout << "\n   d  Remove the next song from the queue\n   0  Back\n";
        string in = readLine("\n  Choose: ");
        if (in != "d" && in != "D") return;
        cout << "\n";
        ui.queueDequeue(items, n, total);
        Song gone = manager.getNextSong();
        printSuccess("Removed from Up Next: " + gone.title);
        SLEEP_MS(PAUSE_SHORT_MS);
    }
}

void MediaPlayerApp::historyScreen() {
    while (!inputClosed) {
        ui.clearScreen();
        printSectionHeader("LISTENING HISTORY  (stack: last in, first out)");
        Song items[HISTORY_SHOWN];
        int n = manager.historySnapshot(items, HISTORY_SHOWN);
        if (n == 0) {
            printInfo("Nothing played yet this session. Play a song and it is pushed onto this stack.");
            waitForEnter();
            return;
        }
        cout << "\n";
        ui.stackView(items, n);
        cout << "\n";
        manager.showHistory();
        cout << "\n   p  Pop the latest entry (remove it from the history)\n   0  Back\n";
        string in = readLine("\n  Choose: ");
        if (in != "p" && in != "P") return;
        cout << "\n";
        ui.stackPop(items, n);
        Song gone = manager.popHistory();
        printSuccess("Popped from history: " + gone.title);
        SLEEP_MS(PAUSE_SHORT_MS);
    }
}

/*
===========================================================================
  Playlists: up to 10 per user. The hub manages the playlists themselves;
   editPlaylist() changes the tracks inside one of them.
===========================================================================
*/
void MediaPlayerApp::playlistHub() {
    while (!inputClosed) {
        ui.clearScreen();
        ui.displayPlaylists(plists, "MY PLAYLISTS");
        cout << "  Type a playlist number to open and edit it, or:\n"
             << "   c  Create a new playlist  (up to " << MAX_PLAYLISTS << ")\n"
             << "   r  Rename a playlist\n"
             << "   d  Delete a playlist\n"
             << "   s  Set the active playlist  (the one that plays)\n"
             << "   0  Back\n";
        string in = readLine("\n  Choose: ");
        if (in.empty() || in == "0") return;

        int v;
        if (toInt(in, v)) {
            if (v >= 1 && v <= plists.getCount()) { plists.select(v - 1); editPlaylist(v - 1); }
            else { printWarn("No such playlist."); SLEEP_MS(PAUSE_SHORT_MS); }
        } else if (in == "c" || in == "C") {
            if (plists.isFull()) { printWarn("You already have 10 playlists (the maximum). Delete one first."); SLEEP_MS(PAUSE_LONG_MS); continue; }
            string name = readLine("  Name for the new playlist: ");
            if (name.empty()) continue;
            string why = plists.checkName(name, -1);
            if (!why.empty()) { printError(why); SLEEP_MS(PAUSE_LONG_MS); continue; }
            int idx = plists.create(name);
            printSuccess("Created \"" + plists.nameAt(idx) + "\". Open it from the list to add songs.");
            SLEEP_MS(PAUSE_MEDIUM_MS);
        } else if (in == "r" || in == "R" || in == "d" || in == "D" || in == "s" || in == "S") {
            char act = (char)tolower((unsigned char)in[0]);
            const char* ask = act == 'r' ? "  Rename which playlist (number)? "
                            : act == 'd' ? "  Delete which playlist (number)? " : "  Make which playlist active (number)? ";
            int i = readInt(ask) - 1;
            if (inputClosed) return;
            if (i < 0 || i >= plists.getCount()) { printWarn("No such playlist."); SLEEP_MS(PAUSE_SHORT_MS); continue; }
            if (act == 's') {
                plists.select(i);
                printSuccess("\"" + plists.nameAt(i) + "\" is now the active playlist.");
            } else if (act == 'r') {
                string name = readLine("  New name: ");
                if (name.empty()) continue;
                string why = plists.checkName(name, i);
                if (!why.empty()) printError(why);
                else { plists.rename(i, name); printSuccess("Renamed to \"" + plists.nameAt(i) + "\"."); }
            } else {
                if (plists.getCount() == 1) { printWarn("You must keep at least one playlist."); SLEEP_MS(PAUSE_LONG_MS); continue; }
                string y = readLine("  Delete \"" + plists.nameAt(i) + "\" and its " + plural(plists.at(i).getSize(), "track") + "? (y/n): ");
                if (saidYes(y)) { printSuccess("Deleted \"" + plists.nameAt(i) + "\"."); plists.remove(i); }
                else printInfo("Nothing deleted.");
            }
            SLEEP_MS(PAUSE_MEDIUM_MS);
        } else {
            printWarn("Unknown option.");
            SLEEP_MS(PAUSE_SHORT_MS);
        }
    }
}

void MediaPlayerApp::editPlaylist(int idx) {
    const int PER_PAGE = EDITOR_PAGE_SIZE;
    int page = 0;
    while (!inputClosed) {
        PlaylistEngine& p = plists.at(idx);
        int n = p.getSize();
        int pages = (n + PER_PAGE - 1) / PER_PAGE;
        if (pages < 1) pages = 1;
        if (page >= pages) page = pages - 1;
        ui.displayPlaylistEditor(p, plists.nameAt(idx), idx + 1, plists.getCount(), page, PER_PAGE);
        cout << "   1  Remove a track              4  Add songs (search)\n"
             << "   2  Move a track up             5  Add songs (browse library)\n"
             << "   3  Move a track down           6  Rename this playlist\n"
             << "   7  Remove ALL tracks           n / p  Next / previous page\n"
             << "   0  Back\n";
        string in = readLine("\n  Choose: ");
        if (in.empty() || in == "0") return;
        if (in == "n" || in == "N") { if (page < pages - 1) page++; continue; }
        if (in == "p" || in == "P") { if (page > 0) page--; continue; }

        int c;
        if (!toInt(in, c)) { printWarn("Unknown option."); SLEEP_MS(PAUSE_SHORT_MS); continue; }
        if (c == 1 || c == 2 || c == 3) {
            if (p.isEmpty()) { printWarn("The playlist is empty."); SLEEP_MS(PAUSE_SHORT_MS); continue; }
            int t = readInt("  Track number: ");
            if (inputClosed) return;
            if (t < 1 || t > n) { printWarn("No such track."); SLEEP_MS(PAUSE_SHORT_MS); continue; }
            Song s = p.getAt(t - 1);
            if (c == 1) {
                p.removeSong(s);
                printSuccess("Removed: " + s.title);
            } else {
                int dir = (c == 2) ? -1 : 1;
                if (p.moveSong(t - 1, dir)) printSuccess("Moved to position " + intStr(t + dir) + ": " + s.title);
                else printWarn(c == 2 ? "It is already first." : "It is already last.");
            }
            SLEEP_MS(PAUSE_SHORT_MS);
        } else if (c == 4) {
            searchFlow();
        } else if (c == 5) {
            browseFlow();
        } else if (c == 6) {
            string name = readLine("  New name: ");
            if (name.empty()) continue;
            string why = plists.checkName(name, idx);
            if (!why.empty()) printError(why);
            else { plists.rename(idx, name); printSuccess("Renamed to \"" + plists.nameAt(idx) + "\"."); }
            SLEEP_MS(PAUSE_MEDIUM_MS);
        } else if (c == 7) {
            if (p.isEmpty()) { printWarn("Nothing to remove."); SLEEP_MS(PAUSE_SHORT_MS); continue; }
            string y = readLine("  Remove all tracks from \"" + plists.nameAt(idx) + "\"? (y/n): ");
            if (saidYes(y)) { p.destroy(); printSuccess("The playlist is now empty."); SLEEP_MS(PAUSE_MEDIUM_MS); }
        } else {
            printWarn("Unknown option.");
            SLEEP_MS(PAUSE_SHORT_MS);
        }
    }
}

void MediaPlayerApp::savePlaylist() {
    plists.saveToFile(currentUser);
    bool ok = fileExists(PlaylistEngine::playlistPath(currentUser));
    ui.saving("playlists", ok);
    waitForEnter();
}

void MediaPlayerApp::helpScreen() {
    for (int page = 0; page < HELP_PAGES && !inputClosed; page++) {
        ui.displayHelp(page);
        string in = readLine(page < HELP_PAGES - 1 ? "  Press Enter for the next page, or 0 to close: "
                                                   : "  Press Enter to close: ");
        if (in == "0") return;
    }
}

void MediaPlayerApp::changeTheme() {
    ui.clearScreen();
    ui.displayThemes();
    int c = readInt("\n  Choose theme (1-" + intStr(THEME_COUNT) + ", 0 = keep): ");
    if (c >= 1 && c <= THEME_COUNT) { themeIndex() = c - 1; saveSettings(); printSuccess(string("Theme set to ") + T().name); }
    else printInfo("Theme unchanged.");
    SLEEP_MS(PAUSE_SHORT_MS);
}

/*
=============================================================================
             Upload New Track (any user)
  Copies the audio into music/, appends songs.txt and indexes it instantly.
=============================================================================
*/
void MediaPlayerApp::uploadTrack() {
    ui.clearScreen();
    printSectionHeader("UPLOAD NEW TRACK");

    string title = readLine("  Title: ");
    if (title.empty()) return;
    string artist = readLine("  Artist [" + currentUser + "]: ");
    if (artist.empty()) artist = currentUser;
    string album = readLine("  Album [Single]: ");
    if (album.empty()) album = "Single";
    string src = readLine("  Path to audio file (.mp3 / .wav): ");
    if (src.size() >= 2 && src[0] == '"' && src[src.size() - 1] == '"') src = src.substr(1, src.size() - 2);  // drag-and-drop adds quotes
    if (inputClosed) return;

    if (containsDelim(title) || containsDelim(artist) || containsDelim(album)) {
        printError("Fields must not contain the '|' character."); waitForEnter(); return;
    }
    if (safeFileName(title) != title) {
        printError("The title cannot contain any of  \\ / : * ? \" < > |  (it becomes the file name)."); waitForEnter(); return;
    }
    if (library.exists(title, artist)) {
        printError("This artist already has a track with that title in the library."); waitForEnter(); return;
    }
    string ext = fileExtension(src);
    if (!fileExistsU(src) || (ext != ".mp3" && ext != ".wav" && ext != ".wma")) {      // UTF-8 safe on Windows
        printError("Audio file not found (supported: .mp3 .wav .wma)."); waitForEnter(); return;
    }

    Song s;
    s.title = title; s.artist = artist; s.album = album;

    // Another artist already uses this title: every file with that title is named "Artist - Title".
    bool shared = library.countTitle(title) > 0;
    if (shared) renameLegacyFiles(title);

    spinnerAnimation("Validating track details...", 14);
    string dst = string(MUSIC_DIR) + (shared ? songFileStem(s) : safeFileName(title)) + ext;
    if (src != dst && !copyFileBinary(src, dst)) {
        printError("Could not copy the file into the music folder."); waitForEnter(); return;
    }

    int dur = AudioPlayer::probeLengthSeconds(dst);
    if (dur <= 0) {
        dur = readInt("  Duration in seconds: ");
        if (dur <= 0) { printError("Invalid duration."); waitForEnter(); return; }
    }

    s.durationSeconds = dur;
    progressBarAnimation("Writing songs.txt   ", 25);
    library.appendSongToFile(s, dataPath("songs.txt"));
    spinnerAnimation("Inserting into hash table...", 14);
    library.insertToLibrary(s);
    printSuccess("Track is now available to all users.");
    waitForEnter();
}

// A title-only file ("Title.mp3") is renamed to "Artist - Title.mp3" once a second artist uses the title.
void MediaPlayerApp::renameLegacyFiles(const string& title) {
    Song* same[8];
    int n = library.findAllByTitle(title, same, 8);
    for (int i = 0; i < n; i++) {
        string f = locateTrackFile(*same[i], false);
        if (f.empty()) continue;
        string stem = toLower(trim(fileStem(f.substr(string(MUSIC_DIR).size()))));
        if (stem == toLower(songFileStem(*same[i]))) continue;               // already named "Artist - Title"
        string to = string(MUSIC_DIR) + songFileStem(*same[i]) + fileExtension(f);
        if (!fileExistsU(to)) renameFileU(f, to);
    }
}

/*
============================================================
        Per-user settings: data/<username>_settings.txt
============================================================
*/
void MediaPlayerApp::loadSettings() {
    themeIndex() = 0;
    ifstream in(settingsPath(currentUser).c_str());
    if (!in) return;
    string line, p[MAX_FIELDS];
    while (getline(in, line)) {
        if (isBlankOrComment(line)) continue;
        int f = splitLine(line, FIELD_DELIM, p, MAX_FIELDS), v = 0;
        if (f == 2 && p[0] == "theme" && toInt(p[1], v) && v >= 0 && v < THEME_COUNT) themeIndex() = v;
    }
}

void MediaPlayerApp::saveSettings() {
    if (currentUser.empty()) return;
    ofstream out(settingsPath(currentUser).c_str());
    if (!out) return;
    out << "# Echo Engine settings for " << currentUser << "\n"
        << "theme" << FIELD_DELIM << themeIndex() << "\n";
}

/*
========================================================================
                    Music folder import
  Title = file name without extension (so it matches the file exactly),
        artist = text before " - " in the file name.
========================================================================
*/
static string guessArtist(const string& stem) {
    size_t p = stem.find(" - ");
    if (p == string::npos) p = stem.find(" \xE2\x80\x93 ");  // UTF-8 en dash
#ifdef _WIN32
    if (p == string::npos) p = stem.find(" \x96 ");  // ANSI en dash
#endif
    if (p == string::npos || p == 0) return "Unknown Artist";
    return trim(stem.substr(0, p));
}

int MediaPlayerApp::syncMusicFolder() {
    int n = 0, added = 0, fixed = 0;
    string* files = listMusicFiles(MUSIC_DIR, n);

    for (int i = 0; i < n; i++) {
        string stem = trim(fileStem(files[i]));
        if (stem.empty()) continue;
        if (containsDelim(stem)) { printWarn("Skipped (name contains '|'): " + files[i]); continue; }

        Song* known = library.findByStem(stem);
        if (known != NULL) {  // already in the library: correct a wrong length
            string ext = fileExtension(files[i]);
            int real = (ext == ".mp3" || ext == ".wav") ? nativeLengthSeconds(string(MUSIC_DIR) + files[i]) : 0;
            if (real > 0 && (real > known->durationSeconds + 2 || real < known->durationSeconds - 2)) {
                known->durationSeconds = real;
                fixed++;
            }
            continue;
        }

        cout << CLEAR_LINE << T().secondary << "  " << spinFrame(i) << " Importing "
             << clipStr(stem, 40) << RESET << flush;

        int dur = AudioPlayer::probeLengthSeconds(string(MUSIC_DIR) + files[i]);
        if (dur <= 0) { cout << CLEAR_LINE; printWarn("Could not read length of: " + files[i]); continue; }

        Song s;
        s.title = stem; s.artist = guessArtist(stem); s.album = "Local Library"; s.durationSeconds = dur;
        library.appendSongToFile(s, dataPath("songs.txt"));
        library.insertToLibrary(s);
        added++;
    }
    delete[] files;

    cout << CLEAR_LINE;
    if (fixed > 0) {
        library.saveDatabaseToFile(dataPath("songs.txt"));
        printSuccess("Corrected the length of " + plural(fixed, "song") + ".");
    }
    if (added > 0) printSuccess("Imported " + plural(added, "new track") + " from music/.");
    return added;
}

/*
==================================================================================
  Player: the live loop (disc, equalizer, audio) is in PlayerEngine.cpp
==================================================================================
*/
void MediaPlayerApp::playerSession() {
    if (pl().isEmpty()) { printWarn("Your playlist is empty. Search or browse to add tracks."); waitForEnter(); return; }

    ui.loading("audio engine");
    ui.displayNowPlaying(pl().getCurrent());
    player.run();
}
