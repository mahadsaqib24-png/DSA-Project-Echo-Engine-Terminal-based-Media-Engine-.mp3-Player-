#ifndef PLAYLIST_ENGINE_H
#define PLAYLIST_ENGINE_H

#include "Structs.h"
#include "FileUtils.h"

/*
============================================================
           MODULE 1: Circular Doubly Linked List
============================================================
*/
class PlaylistEngine {
private:
    SongNode* head;
    SongNode* tail;
    SongNode* currentSong;
    int size;

    PlaylistEngine(const PlaylistEngine&);              // non-copyable (owns raw nodes)
    PlaylistEngine& operator=(const PlaylistEngine&);

    SongNode* findNode(const string& title) const {
        string key = toLower(title);
        SongNode* n = head;
        for (int i = 0; i < size; i++, n = n->next)
            if (toLower(n->data.title) == key) return n;
        return NULL;
    }

public:
    PlaylistEngine() : head(NULL), tail(NULL), currentSong(NULL), size(0) {}

    // O(1): insert at tail, keep the circle closed.
    void addSong(Song s) {
        SongNode* n = new SongNode();
        n->data = s;
        if (head == NULL) {
            head = tail = currentSong = n;
            n->next = n->prev = n;
        } else {
            n->prev = tail;
            n->next = head;
            tail->next = n;
            head->prev = n;
            tail = n;
        }
        size++;
    }

    // O(n): remove the first track with this title (case-insensitive).
    bool removeSong(string title) {
        SongNode* n = findNode(title);
        if (n == NULL) return false;
        if (size == 1) {
            head = tail = currentSong = NULL;
        } else {
            n->prev->next = n->next;
            n->next->prev = n->prev;
            if (n == head) head = n->next;
            if (n == tail) tail = n->prev;
            if (n == currentSong) currentSong = n->next;
        }
        delete n;
        size--;
        return true;
    }

    void playNext()     { if (currentSong) currentSong = currentSong->next; }   // wraps tail -> head
    void playPrevious() { if (currentSong) currentSong = currentSong->prev; }   // wraps head -> tail
    Song getCurrent() const { return currentSong ? currentSong->data : Song(); }
    bool isEmpty() const    { return size == 0; }
    int  getSize() const    { return size; }

    // helpers used by the UI
    bool contains(const string& title) const { return findNode(title) != NULL; }

    bool jumpTo(const string& title) {
        SongNode* n = findNode(title);
        if (n == NULL) return false;
        currentSong = n;
        return true;
    }

    void resetToHead() { currentSong = head; }

    int getCurrentIndex() const {                       // 0-based, -1 if empty
        if (currentSong == NULL) return -1;
        SongNode* n = head;
        for (int i = 0; i < size; i++, n = n->next)
            if (n == currentSong) return i;
        return -1;
    }

    Song getAt(int index) const {                       // O(n), index 0..size-1
        if (index < 0 || index >= size) return Song();
        SongNode* n = head;
        while (index-- > 0) n = n->next;
        return n->data;
    }

    int totalSeconds() const {                          // O(n): length of the whole playlist
        int t = 0;
        SongNode* n = head;
        for (int i = 0; i < size; i++, n = n->next) t += n->data.durationSeconds;
        return t;
    }

    // Swap a track with its neighbour (dir -1 = up, +1 = down). O(n). The playing track keeps playing.
    bool moveSong(int index, int dir) {
        int other = index + dir;
        if (index < 0 || index >= size || other < 0 || other >= size) return false;
        SongNode* a = head;
        for (int i = 0; i < index; i++) a = a->next;
        SongNode* b = (dir > 0) ? a->next : a->prev;
        Song tmp = a->data; a->data = b->data; b->data = tmp;
        if (currentSong == a) currentSong = b;
        else if (currentSong == b) currentSong = a;
        return true;
    }

    void setDuration(int index, int secs) {             // O(n): fix a stored length
        if (index < 0 || index >= size) return;
        SongNode* n = head;
        while (index-- > 0) n = n->next;
        n->data.durationSeconds = secs;
    }

    void writeSongs(ostream& out) const {               // Title|Artist|Album|DurationSecs, one per line
        SongNode* n = head;
        for (int i = 0; i < size; i++, n = n->next)
            out << n->data.title << FIELD_DELIM << n->data.artist << FIELD_DELIM
                << n->data.album << FIELD_DELIM << n->data.durationSeconds << "\n";
    }

    // per-user filing: data/playlists/<username>.txt
    static string playlistPath(const string& username) {
        string safe = username;
        for (size_t i = 0; i < safe.size(); i++)
            if (!isalnum((unsigned char)safe[i])) safe[i] = '_';
        return dataPath("playlists/" + safe + ".txt");
    }

    void savePlaylistToFile(string username) {
        ofstream out(playlistPath(username).c_str());
        if (!out) return;
        out << "# Title|Artist|Album|DurationSecs\n";
        SongNode* n = head;
        for (int i = 0; i < size; i++, n = n->next)
            out << n->data.title << FIELD_DELIM << n->data.artist << FIELD_DELIM
                << n->data.album << FIELD_DELIM << n->data.durationSeconds << "\n";
    }

    void loadPlaylistFromFile(string username) {
        string path = playlistPath(username);
        ifstream in(path.c_str());
        if (!in) return;                                 // first login: nothing saved yet
        destroy();
        string line, p[MAX_FIELDS];
        int lineNo = 0, dur = 0;
        while (getline(in, line)) {
            lineNo++;
            if (isBlankOrComment(line)) continue;
            if (splitLine(line, FIELD_DELIM, p, MAX_FIELDS) != 4) { reportBadLine(path, lineNo, "expected 4 fields"); continue; }
            if (p[0].empty() || !toInt(p[3], dur) || dur <= 0)     { reportBadLine(path, lineNo, "bad title or duration"); continue; }
            Song s;
            s.title = p[0]; s.artist = p[1]; s.album = p[2]; s.durationSeconds = dur;
            if (!contains(s.title)) addSong(s);
        }
    }

    // Break the circle, delete every node, leave the object reusable.
    void destroy() {
        if (head == NULL) return;
        tail->next = NULL;
        while (head != NULL) {
            SongNode* t = head;
            head = head->next;
            delete t;
        }
        tail = currentSong = NULL;
        size = 0;
    }

    ~PlaylistEngine() { destroy(); }
};

#endif
