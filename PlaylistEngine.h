#ifndef PLAYLIST_ENGINE_H
#define PLAYLIST_ENGINE_H

#include <cstdlib>
#include "Structs.h"
#include "FileUtils.h"

/*
============================================================
    			Circular Doubly Linked List
  Tracks are identified by artist + title (songKey), so two
  songs with the same title by different artists can coexist.
  Shuffle uses a Fisher-Yates deck kept in a new[] array.
============================================================
*/
class PlaylistEngine {
private:
    SongNode* head;
    SongNode* tail;
    SongNode* currentSong;
    int size;

    int* deck;                // shuffle order (indices into the list), new[] / delete[]
    int  deckSize;            // 0 = no valid deck
    int  deckPos;             // index inside deck of the track that is playing

    PlaylistEngine(const PlaylistEngine&);              // non-copyable (owns raw nodes)
    PlaylistEngine& operator=(const PlaylistEngine&);

    SongNode* findNode(const string& key) const {
        SongNode* n = head;
        for (int i = 0; i < size; i++, n = n->next)
            if (songKey(n->data) == key) return n;
        return NULL;
    }

    void freeDeck() {
        delete[] deck;
        deck = NULL;
        deckSize = 0;
        deckPos = 0;
    }

    // Fisher-Yates over 0..size-1. firstIdx (if >= 0) is moved to position 0;
    // otherwise avoidFirst (if >= 0) is kept out of position 0 so a track never repeats back to back.
    void buildDeck(int firstIdx, int avoidFirst) {
        freeDeck();
        if (size == 0) return;
        deck = new int[size];
        deckSize = size;
        for (int i = 0; i < size; i++) deck[i] = i;
        for (int i = size - 1; i > 0; i--) {
            int j = rand() % (i + 1);
            int t = deck[i]; deck[i] = deck[j]; deck[j] = t;
        }
        if (firstIdx >= 0) {
            for (int i = 0; i < size; i++)
                if (deck[i] == firstIdx) { int t = deck[0]; deck[0] = deck[i]; deck[i] = t; break; }
        } else if (avoidFirst >= 0 && size > 1 && deck[0] == avoidFirst) {
            int t = deck[0]; deck[0] = deck[1]; deck[1] = t;
        }
        deckPos = 0;
    }

public:
    PlaylistEngine() : head(NULL), tail(NULL), currentSong(NULL), size(0), deck(NULL), deckSize(0), deckPos(0) {}

    // O(1): insert at tail, keep the circle closed.
    void addSong(const Song& s) {
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
        deckSize = 0;                                   // shuffle deck is out of date
    }

    // O(n): remove the track with this artist + title.
    bool removeSong(const Song& s) {
        SongNode* n = findNode(songKey(s));
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
        deckSize = 0;
        return true;
    }

    void playNext()     { if (currentSong) currentSong = currentSong->next; }   // wraps tail -> head
    void playPrevious() { if (currentSong) currentSong = currentSong->prev; }   // wraps head -> tail
    Song getCurrent() const { return currentSong ? currentSong->data : Song(); }
    bool isEmpty() const    { return size == 0; }
    int  getSize() const    { return size; }

    // helpers used by the UI
    bool contains(const Song& s) const { return findNode(songKey(s)) != NULL; }

    bool jumpTo(const Song& s) {
        SongNode* n = findNode(songKey(s));
        if (n == NULL) return false;
        currentSong = n;
        return true;
    }

    bool jumpToIndex(int index) {                       // O(n)
        if (index < 0 || index >= size) return false;
        SongNode* n = head;
        while (index-- > 0) n = n->next;
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
        deckSize = 0;
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
        for (int i = 0; i < size; i++, n = n->next) out << songToLine(n->data) << "\n";
    }

    // per-user filing: data/playlists/<username>.txt
    static string playlistPath(const string& username) {
        return dataPath("playlists/" + userFileName(username) + ".txt");
    }

    /*
    ============================================================
       Shuffle: every track plays once, then the deck is reshuffled
    ============================================================
    */
    // Starts a fresh Fisher-Yates deck with the playing track first (already "played").
    void shuffleReset() { buildDeck(getCurrentIndex(), -1); }

    void shuffleOff() { freeDeck(); }

    // True when the playing track is the last one of the current deck.
    bool shuffleCycleDone() const {
        if (size <= 1) return true;
        return deckSize == size && deckPos == size - 1;
    }

    // Moves to the next unplayed track. When the deck is used up it is reshuffled
    // (the new first track differs from the one that just played) and true is returned.
    bool playNextShuffled() {
        if (size == 0) return false;
        if (deckSize != size) buildDeck(getCurrentIndex(), -1);
        bool reshuffled = false;
        if (deckPos + 1 >= deckSize) {
            buildDeck(-1, deck[deckSize - 1]);
            reshuffled = true;
        } else {
            deckPos++;
        }
        jumpToIndex(deck[deckPos]);
        return reshuffled;
    }

    // After jumping elsewhere (Previous), point the deck at the playing track again.
    void shuffleSync() {
        if (deckSize != size) return;
        int cur = getCurrentIndex();
        for (int i = 0; i < deckSize; i++)
            if (deck[i] == cur) { deckPos = i; return; }
    }

    // Break the circle, delete every node, leave the object reusable.
    void destroy() {
        freeDeck();
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