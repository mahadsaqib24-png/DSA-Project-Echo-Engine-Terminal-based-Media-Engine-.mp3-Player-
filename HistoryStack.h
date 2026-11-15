#ifndef HISTORY_STACK_H
#define HISTORY_STACK_H

#include "Structs.h"
#include "animations.h"
/*
=============================================================
  MODULE 3b: LIFO Stack ("Recently Played"), singly linked
=============================================================
*/
class HistoryStack {
private:
    PlaybackNode* top;
    int count;

    HistoryStack(const HistoryStack&);
    HistoryStack& operator=(const HistoryStack&);

public:
    HistoryStack() : top(NULL), count(0) {}

    void pushToHistory(const Song& s) {
        PlaybackNode* n = new PlaybackNode();
        n->data = s;
        n->next = top;
        top = n;
        count++;
    }

    Song pop() {
        if (top == NULL) return Song();
        PlaybackNode* t = top;
        Song s = t->data;
        top = top->next;
        delete t;
        count--;
        return s;
    }

    bool isEmpty() const { return top == NULL; }
    int  size() const { return count; }

    // Copies up to maxN songs, newest first (so the UI can animate real data).
    int snapshot(Song out[], int maxN) const {
        int i = 0;
        for (PlaybackNode* n = top; n != NULL && i < maxN; n = n->next) out[i++] = n->data;
        return i;
    }

    // Most recent first (shows at most 15 entries).
    void displayHistory() const {
        if (top == NULL) { printInfo("Nothing played yet this session."); return; }
        int tw, aw;
        listWidths(tw, aw);
        int i = 1;
        for (PlaybackNode* n = top; n != NULL && i <= HISTORY_SHOWN; n = n->next, i++)
            cout << "  " << (i == 1 ? T().mid : DIM) << (i < 10 ? " " : "") << i << "." << RESET << " " << padTo(n->data.title, tw) << " "
                 << DIM << padTo(n->data.artist, aw) << " " << formatTime(n->data.durationSeconds) << RESET << "\n";
        if (count > HISTORY_SHOWN) cout << DIM << "  ... and " << (count - HISTORY_SHOWN) << " earlier\n" << RESET;
    }

    void destroy() {
        while (top != NULL) { PlaybackNode* t = top; top = top->next; delete t; }
        count = 0;
    }
    ~HistoryStack() { destroy(); }
};

#endif
