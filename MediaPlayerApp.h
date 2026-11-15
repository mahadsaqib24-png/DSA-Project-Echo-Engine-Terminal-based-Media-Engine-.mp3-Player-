#ifndef MEDIA_PLAYER_APP_H
#define MEDIA_PLAYER_APP_H

#include <iostream>
#include <string>
#include "PlaylistEngine.h"
#include "PlaylistCollection.h"
#include "LibraryIndex.h"
#include "PlaybackManager.h"
#include "Dashboard.h"
#include "AudioPlayer.h"
#include "PlayerEngine.h"

/*
============================================================
          Top-level controller: owns every module
============================================================
*/
class MediaPlayerApp {
private:
    PlaylistCollection plists;
    LibraryIndex    library;
    PlaybackManager manager;
    Dashboard       ui;
    PlayerEngine    player;
    string          currentUser;
    bool            inputClosed;  // stdin hit EOF

    MediaPlayerApp(const MediaPlayerApp&);
    MediaPlayerApp& operator=(const MediaPlayerApp&);

    // input
    string readLine(const string& prompt);
    int    readInt(const string& prompt);  // -1 on bad input, 0 when stdin is closed
    string readPassword(const string& prompt);
    void   waitForEnter();

    // startup / session
    void seedDataFiles();
    bool authScreen();
    bool login();
    bool createAccount();
    void startSession();
    void endSession();

    // menu features
    bool handleChoice(int choice);
    void searchFlow();
    void browseFlow();
    void songActions(const Song& s);
    void addToPlaylist(const Song& s, int idx);
    void playlistHub();
    void editPlaylist(int idx);
    int  choosePlaylist(const string& heading);  // asks which playlist (-1 = cancelled)
    void upNextScreen();
    void historyScreen();
    void helpScreen();
    PlaylistEngine& pl();
    void showDashboard();
    void changeTheme();
    void savePlaylist();
    void uploadTrack();
    int  syncMusicFolder();

    // player (the live loop is in PlayerEngine)
    void playerSession();

    // per-user settings and audio files
    void loadSettings();
    void saveSettings();
    void renameLegacyFiles(const string& title);

public:
    MediaPlayerApp();

    void run();
    void handleMenu();
};

#endif
