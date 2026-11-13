#ifndef PLAYLIST_COLLECTION_H
#define PLAYLIST_COLLECTION_H

#include "PlaylistEngine.h"

/*
=======================================================================
               MODULE 1b: Playlist Collection
  A user's playlists (up to 10), each one a circular
  doubly linked list (PlaylistEngine). One of them is "active":
  it is the one that plays and the one new songs go into by default.
========================================================================
*/
const int MAX_PLAYLISTS = 10;
const int MAX_PLAYLIST_NAME = 24;

class PlaylistCollection {
private:
    PlaylistEngine* lists[MAX_PLAYLISTS];      // owned: created with new, freed in destroy()
    string names[MAX_PLAYLISTS];
    int count, active;

    PlaylistCollection(const PlaylistCollection&);
    PlaylistCollection& operator=(const PlaylistCollection&);

    // Like create(), but adds " (2)", " (3)" ... when the wanted name is invalid or taken.
    int createUnique(const string& wanted) {
        int r = create(wanted);
        for (int k = 2; r < 0 && count < MAX_PLAYLISTS && k < 100; k++) {
            char b[16];
            sprintf(b, " (%d)", k);
            r = create(trim(wanted).substr(0, MAX_PLAYLIST_NAME - 6) + b);
        }
        return r;
    }

public:
    PlaylistCollection() : count(0), active(0) {
        for (int i = 0; i < MAX_PLAYLISTS; i++) lists[i] = NULL;
    }

    int getCount() const  { return count; }
    int getActive() const { return active; }
    bool isFull() const   { return count >= MAX_PLAYLISTS; }
    const string& nameAt(int i) const { return names[i]; }
    PlaylistEngine& current()         { return *lists[active]; }
    PlaylistEngine& at(int i)         { return *lists[i]; }
    const PlaylistEngine& at(int i) const { return *lists[i]; }

    // "" if the name is acceptable, otherwise the reason it is not. 'ignore' = index being renamed (or -1).
    string checkName(const string& name, int ignore) const {
        string n = trim(name);
        if (n.empty())                          return "The name cannot be empty.";
        if ((int)n.size() > MAX_PLAYLIST_NAME)  return "The name can be at most 24 characters.";
        if (containsDelim(n))                   return "The name cannot contain the | character.";
        if (n[0] == '@' || n[0] == '#')         return "The name cannot start with @ or #.";
        for (int i = 0; i < count; i++)
            if (i != ignore && toLower(names[i]) == toLower(n)) return "You already have a playlist with that name.";
        return "";
    }

    // Adds an empty playlist. Returns its index, or -1 (full / bad name).
    int create(const string& name) {
        if (count >= MAX_PLAYLISTS || !checkName(name, -1).empty()) return -1;
        lists[count] = new PlaylistEngine();
        names[count] = trim(name);
        return count++;
    }

    bool rename(int i, const string& name) {
        if (i < 0 || i >= count || !checkName(name, i).empty()) return false;
        names[i] = trim(name);
        return true;
    }

    bool select(int i) {
        if (i < 0 || i >= count) return false;
        active = i;
        return true;
    }

    // Deletes a playlist (and frees its nodes). The last remaining playlist cannot be deleted.
    bool remove(int i) {
        if (count <= 1 || i < 0 || i >= count) return false;
        delete lists[i];
        for (int k = i; k < count - 1; k++) { lists[k] = lists[k + 1]; names[k] = names[k + 1]; }
        lists[count - 1] = NULL;
        names[count - 1] = "";
        count--;
        if (i < active) active--;
        else if (active >= count) active = count - 1;
        return true;
    }

    void ensureDefault() { if (count == 0) { create("My Playlist"); active = 0; } }

    void saveToFile(const string& username) const {
        ofstream out(PlaylistEngine::playlistPath(username).c_str());
        if (!out) return;
        out << "# Echo Engine playlists for " << username << "\n"
            << "# @PLAYLIST|Name starts a playlist, then Title|Artist|Album|DurationSecs lines\n";
        for (int i = 0; i < count; i++) {
            out << "@PLAYLIST" << FIELD_DELIM << names[i] << "\n";
            lists[i]->writeSongs(out);
        }
        out << "@ACTIVE" << FIELD_DELIM << (active + 1) << "\n";
    }

    // Replaces everything in memory with the user's file (a new user gets one empty "My Playlist").
    void loadFromFile(const string& username) {
        destroy();
        string path = PlaylistEngine::playlistPath(username);
        ifstream in(path.c_str());
        if (in) {
            string line, p[MAX_FIELDS];
            int lineNo = 0, dur = 0, cur = -1, wantActive = 1;
            bool skipping = false;
            while (getline(in, line)) {
                lineNo++;
                if (isBlankOrComment(line)) continue;
                int f = splitLine(line, FIELD_DELIM, p, MAX_FIELDS);
                if (p[0] == "@PLAYLIST") {
                    cur = createUnique(f >= 2 ? p[1] : string("Playlist"));
                    skipping = (cur < 0);                            // more than 10 playlists in the file
                    continue;
                }
                if (p[0] == "@ACTIVE") { if (f >= 2) toInt(p[1], wantActive); continue; }
                if (skipping) continue;
                if (f != 4) { reportBadLine(path, lineNo, "expected 4 fields"); continue; }
                if (p[0].empty() || !toInt(p[3], dur) || dur <= 0) { reportBadLine(path, lineNo, "bad title or duration"); continue; }
                if (cur < 0) cur = create("My Playlist");            // old single-list file
                Song s;
                s.title = p[0]; s.artist = p[1]; s.album = p[2]; s.durationSeconds = dur;
                if (cur >= 0 && !lists[cur]->contains(s.title)) lists[cur]->addSong(s);
            }
            if (wantActive >= 1 && wantActive <= count) active = wantActive - 1;
        }
        ensureDefault();
    }

    void destroy() {
        for (int i = 0; i < MAX_PLAYLISTS; i++) { delete lists[i]; lists[i] = NULL; names[i] = ""; }
        count = 0;
        active = 0;
    }
    ~PlaylistCollection() { destroy(); }
};

#endif
