#ifndef DASHBOARD_H
#define DASHBOARD_H

#include <iostream>
#include <string>
#include "PlaylistEngine.h"
#include "PlaylistCollection.h"
#include "animations.h"
#include "Constants.h"
#include "FileUtils.h"

/*
=================================================================
  UI layer. Pure presentation: it reads state, it never owns
  playlist / queue / audio logic (that stays in MediaPlayerApp).
=================================================================
*/

// MediaPlayerApp updates the playback fields; Dashboard animates the rest.

struct PlayerView {    
	// set by MediaPlayerApp
    Song   song;
    string msg;
    double elapsed, sleepLeft;
    bool paused, ended, shuffle, fromQueue, hasAudio;
    int repeatMode;  		// 0 off, 1 all, 2 one
    int volume;  			// 0..10
    int index, total, queueCount, msgTtl;
    // animated by Dashboard
    string prevTitle;
    double theta, spin;
    int    level[PLAYER_BARS];
    int    scratch, frame, slide, slideDir, lastCols, lastRows;

    PlayerView() : elapsed(0), sleepLeft(0), paused(false), ended(false), shuffle(false), fromQueue(false),
                   hasAudio(false), repeatMode(0), volume(7), index(0), total(0), queueCount(0),
                   msgTtl(0), theta(0), spin(0.38), scratch(0), frame(0), slide(-1), slideDir(1),
                   lastCols(-1), lastRows(-1) {
        for (int i = 0; i < PLAYER_BARS; i++) level[i] = 0;
    }
};

class Dashboard {
public:
    // UML methods
    void displayDashboard(const PlaylistEngine& p, const string& name = "", int index = 0, int total = 0) const;
    void displayNowPlaying(const Song& s) const;
    void displayMenu() const;
    void displayHelp(int page) const;

    // screen / transition helpers
    void clearScreen() const;
    void splash() const;
    void boot(int songs, int users, int buckets) const;
    void displayAuthMenu() const;
    void dashboardIntro(const string& user) const;
    void loginResult(const string& user, bool ok) const;
    void goodbye() const;
    void loading(const string& what, bool ok = true) const;
    void saving(const string& what, bool ok = true) const;
    void displaySongList(Song* const list[], int n, int firstNumber, const string& heading) const;
    void displayThemes() const;

    // playlists (up to 10 per user)
    void displayPlaylists(const PlaylistCollection& pc, const string& heading) const;
    void displayPlaylistEditor(const PlaylistEngine& p, const string& name, int index, int total,
                               int page, int perPage) const;

    // Up Next queue / History stack screens (animated with the real data)
    void queueView(const Song items[], int n, int total) const;  // items[0] = front
    void queueEnqueue(const Song items[], int n, int total, const Song& added) const;
    void queueDequeue(const Song items[], int n, int total) const;
    void stackView(const Song items[], int n) const;  // items[0] = newest
    void stackPop(const Song items[], int n) const;

    // persistent Now Playing screen (Option 12 disc + equalizer)
    void enterPlayer() const;
    void leavePlayer() const;
    void startSlide(PlayerView& v, const string& fromTitle, bool forward) const;
    void drawPlayerFrame(PlayerView& v) const;  // advances the animation one step and draws
};

#endif
