#ifndef SONG_QUEUE_H
#define SONG_QUEUE_H

#include "Structs.h"
#include "animations.h"

/*
===============================================================
  MODULE 3a: FIFO Queue ("Up Next"), singly linked, O(1) ends
===============================================================
*/
class SongQueue {
private:
    PlaybackNode* front;
    PlaybackNode* rear;
    int count;

    SongQueue(const SongQueue&);
    SongQueue& operator=(const SongQueue&);

public:
    SongQueue() : front(NULL), rear(NULL), count(0) {}

    void enqueueSong(const Song& s) {
        PlaybackNode* n = new PlaybackNode();
        n->data = s;
        if (rear == NULL) front = rear = n;
        else { rear->next = n; rear = n; }
        count++;
    }

    Song dequeueSong() {
        if (front == NULL) return Song();
        PlaybackNode* t = front;
        Song s = t->data;
        front = front->next;
        if (front == NULL) rear = NULL;
        delete t;
        count--;
        return s;
    }

    Song peek() const { return front ? front->data : Song(); }
    bool isEmpty() const { return front == NULL; }
    int  size() const { return count; }

    // Copies up to maxN songs, front first (so the UI can animate real data).
    int snapshot(Song out[], int maxN) const {
        int i = 0;
        for (PlaybackNode* n = front; n != NULL && i < maxN; n = n->next) out[i++] = n->data;
        return i;
    }

    void displayQueue() const {
        if (front == NULL) { printInfo("Up Next is empty."); return; }
        int tw, aw;
        listWidths(tw, aw);
        tw -= 1;                                   // numbers are 3 wide (up to 999)
        int i = 1;
        for (PlaybackNode* n = front; n != NULL; n = n->next, i++)
            cout << "  " << T().low << padNum(i, 3) << "." << RESET << " " << padTo(n->data.title, tw) << " "
                 << DIM << padTo(n->data.artist, aw) << " " << formatTime(n->data.durationSeconds) << RESET << "\n";
    }

    void destroy() {
        while (front != NULL) { PlaybackNode* t = front; front = front->next; delete t; }
        rear = NULL;
        count = 0;
    }
    ~SongQueue() { destroy(); }
};

#endif
