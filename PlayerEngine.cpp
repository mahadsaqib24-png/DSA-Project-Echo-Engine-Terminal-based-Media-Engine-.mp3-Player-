#include "PlayerEngine.h"

PlayerEngine::PlayerEngine(PlaylistCollection& pc, PlaybackManager& pm, LibraryIndex& lib, const Dashboard& d)
    : plists(pc), manager(pm), library(lib), ui(d), volPending(false), volAt(0) {}

/*
==================================================================================
  Player: spinning disc + equalizer + progress bar (Dashboard draws, this drives)
==================================================================================
*/
void PlayerEngine::say(PlayerView& v, const string& text) {
    v.msg = text;
    v.msgTtl = MSG_TTL_FRAMES;
}

// record = false for a restart or repeat-one, so the history never holds the same play twice.
void PlayerEngine::beginTrack(const Song& s, PlayerView& v, bool manual, bool record) {
    string oldTitle = v.song.title;
    audio.stop();

    string file = AudioPlayer::findTrackFile(s, library.countTitle(s.title) > 1);
    v.hasAudio = !file.empty() && audio.open(file) && audio.play();
    if (v.hasAudio) audio.setVolume(v.volume * 10);
    volPending = false;

    if (record) manager.recordPlay(s);
    v.song = s;
    v.elapsed = 0;
    v.ended = false;
    v.paused = false;
    v.total = pl().getSize();
    v.index = pl().getCurrentIndex() + 1;
    if (manual) { v.scratch = SCRATCH_FRAMES; if (!oldTitle.empty()) ui.startSlide(v, oldTitle, true); }
    if (!v.hasAudio && v.msgTtl == 0) say(v, AudioPlayer::isSupported() ? "No file in music/: simulated playback" : "Simulated playback");
}

// Up Next songs go first, then the circular playlist. Handles repeat, shuffle and the end of the list.
void PlayerEngine::advanceTrack(PlayerView& v, bool manual) {
    if (!manual && v.repeatMode == 2) { beginTrack(v.song, v, false, false); return; }

    bool queued = manager.hasUpNext();
    if (!manual && !queued && v.repeatMode == 0) {
        bool last = v.shuffle ? pl().shuffleCycleDone()
                              : (pl().getCurrentIndex() == pl().getSize() - 1);
        if (last) {
            v.ended = true;
            v.elapsed = v.song.durationSeconds;
            audio.stop();
            say(v, "End of playlist");
            v.msgTtl = END_MSG_TTL_FRAMES;
            return;
        }
    }

    Song next;
    if (queued) {
        next = manager.getNextSong();
        v.fromQueue = true;
    } else {
        if (v.shuffle) pl().playNextShuffled();    // every track once, then a fresh Fisher-Yates deck
        else           pl().playNext();
        next = pl().getCurrent();
        v.fromQueue = false;
    }
    beginTrack(next, v, manual, true);
}

// Previous: restart a long-playing track, else take the last song off the history stack.
void PlayerEngine::previousTrack(PlayerView& v) {
    if (v.fromQueue || v.elapsed > PREV_RESTART_SEC) {
        beginTrack(v.song, v, true, false);
        say(v, "Restarted track");
        return;
    }
    string old = v.song.title;
    Song prev;
    bool fromHistory = false;
    if (!manager.historyEmpty()) {
        manager.popHistory();                      // the track that is playing right now
        if (!manager.historyEmpty()) { prev = manager.popHistory(); fromHistory = true; }
    }
    if (fromHistory) {
        v.fromQueue = !pl().jumpTo(prev);          // a song that is not in the playlist came from Up Next
    } else {
        pl().playPrevious();                       // history is empty: walk the circle backwards
        prev = pl().getCurrent();
        v.fromQueue = false;
    }
    if (v.shuffle) pl().shuffleSync();
    beginTrack(prev, v, false, true);
    v.scratch = SCRATCH_FRAMES;
    ui.startSlide(v, old, false);
    say(v, "<< Previous track");
}

// The label changes at once; the audio backend follows when the keys stop (a restart on Linux).
void PlayerEngine::changeVolume(PlayerView& v, int delta) {
    v.volume += delta;
    if (v.volume < 0) v.volume = 0;
    if (v.volume > 10) v.volume = 10;
    volPending = true;
    volAt = nowSec();
}

void PlayerEngine::run() {
    const int sleepMinutes[4] = { 0, 15, 30, 60 };
    int sleepIdx = 0;
    bool quit = false, sleepEnded = false;
    PlayerView v;
    pl().shuffleOff();
    volPending = false;
    beginTrack(pl().getCurrent(), v, false, true);

    double lastTick = nowSec();
    ui.enterPlayer();
    while (!quit) {
        double now = nowSec();
        double dt = now - lastTick;
        lastTick = now;

        while (keyAvailable()) {
            int k = readKey();
            if (k < 0) break;
            if (k == 'q' || k == 'Q') { quit = true; break; }
            else if (k == ' ') {
                if (v.ended) {
                    pl().resetToHead();
                    if (v.shuffle) pl().shuffleReset();
                    v.fromQueue = false;
                    beginTrack(pl().getCurrent(), v, false, true);
                    say(v, "Restarted playlist");
                } else {
                    v.paused = !v.paused;
                    if (v.paused) audio.pause(); else audio.resume();
                    say(v, v.paused ? "Paused" : "Playing");
                }
            }
            else if (k == 'n' || k == 'N') { advanceTrack(v, true); say(v, ">> Next track"); }
            else if (k == 'p' || k == 'P') previousTrack(v);
            else if (k == 's' || k == 'S') {
                v.shuffle = !v.shuffle;
                if (v.shuffle) pl().shuffleReset(); else pl().shuffleOff();
                say(v, v.shuffle ? "Shuffle ON" : "Shuffle OFF");
            }
            else if (k == 'r' || k == 'R') {
                v.repeatMode = (v.repeatMode + 1) % 3;
                say(v, v.repeatMode == 0 ? "Repeat OFF" : (v.repeatMode == 1 ? "Repeat ALL" : "Repeat ONE"));
            }
            else if (k == '+' || k == '=') { changeVolume(v, 1);  say(v, "Volume up"); }
            else if (k == '-' || k == '_') { changeVolume(v, -1); say(v, "Volume down"); }
            else if (k == 't' || k == 'T') {
                sleepIdx = (sleepIdx + 1) % 4;
                v.sleepLeft = sleepMinutes[sleepIdx] * 60.0;
                say(v, sleepIdx == 0 ? "Sleep timer OFF" : "Sleep timer set");
            }
            else if (k == 'h' || k == 'H') { themeIndex() = (themeIndex() + 1) % THEME_COUNT; say(v, string("Theme: ") + T().name); }
        }
        if (quit) break;

        if (volPending && nowSec() - volAt >= VOLUME_DEBOUNCE_SEC) {   // hold + or -: one restart at the end
            audio.setVolume(v.volume * 10);
            volPending = false;
        }

        if (!v.paused && !v.ended) {
            if (audio.isActive()) {
                double pos = audio.positionSeconds();
                if (pos >= 0) v.elapsed = pos;
            } else {
                v.elapsed += dt;
            }
            bool finished;
            if (audio.isActive())  // the +5 s guard catches a player that never reports "stopped"
                finished = (v.elapsed > 1.5 && audio.isStopped()) || v.elapsed >= v.song.durationSeconds + 5;
            else
                finished = v.elapsed >= v.song.durationSeconds;
            if (finished) advanceTrack(v, false);
        }
        if (v.sleepLeft > 0) {
            v.sleepLeft -= dt;
            if (v.sleepLeft <= 0) { v.sleepLeft = 0; sleepEnded = true; quit = true; }
        }
        v.queueCount = manager.upNextCount();

        ui.drawPlayerFrame(v);
        SLEEP_MS(PLAYER_FRAME_MS);
    }

    audio.stop();
    pl().shuffleOff();
    ui.leavePlayer();
    if (sleepEnded) printInfo("Sleep timer finished. Goodnight!");
    else            printInfo("Player closed.");
    SLEEP_MS(PAUSE_SHORT_MS);
}
