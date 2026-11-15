#ifndef PLAYER_ENGINE_H
#define PLAYER_ENGINE_H

#include "PlaylistCollection.h"
#include "LibraryIndex.h"
#include "PlaybackManager.h"
#include "Dashboard.h"
#include "AudioPlayer.h"

/*
============================================================
  The live player: full-screen loop, audio playback, shuffle,
  repeat, Up Next, history based Previous, sleep timer.
  MediaPlayerApp only decides WHEN to open it.
============================================================
*/
class PlayerEngine {
private:
    PlaylistCollection& plists;
    PlaybackManager&    manager;
    LibraryIndex&       library;
    const Dashboard&    ui;
    AudioPlayer         audio;
    bool                volPending;   // a volume change is waiting for the keys to stop
    double              volAt;        // time of the last volume key

    PlayerEngine(const PlayerEngine&);
    PlayerEngine& operator=(const PlayerEngine&);

    PlaylistEngine& pl() { return plists.current(); }
    void say(PlayerView& v, const string& text);
    void beginTrack(const Song& s, PlayerView& v, bool manual, bool record);
    void advanceTrack(PlayerView& v, bool manual);
    void previousTrack(PlayerView& v);
    void changeVolume(PlayerView& v, int delta);

public:
    PlayerEngine(PlaylistCollection& pc, PlaybackManager& pm, LibraryIndex& lib, const Dashboard& d);

    void run();                              // one full-screen player session
    void stopAudio() { audio.stop(); }
};

#endif
