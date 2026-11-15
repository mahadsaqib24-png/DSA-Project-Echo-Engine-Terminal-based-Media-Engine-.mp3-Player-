#ifndef STRUCTS_H
#define STRUCTS_H

#include <string>
#include <cstddef>
using namespace std;

/*
============================================================
  Shared data payload and node types (used by all modules)
============================================================
*/

struct Song {
    string title;
    string artist;
    string album;
    int durationSeconds;

    Song() : durationSeconds(0) {}
};

// Circular Doubly Linked List node (Playlist)
struct SongNode {
    Song data;
    SongNode* next;
    SongNode* prev;

    SongNode() : next(NULL), prev(NULL) {}
};

// Hash Table chain node (Library)
struct HashNode {
    Song data;
    HashNode* next;

    HashNode() : next(NULL) {}
};

// Queue (Up Next) and Stack (History) node
struct PlaybackNode {
    Song data;
    PlaybackNode* next;

    PlaybackNode() : next(NULL) {}
};

// Singly Linked List node (Authentication)
struct UserNode {
    string username;
    string password;
    UserNode* next;

    UserNode() : next(NULL) {}
};

#endif
