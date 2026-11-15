#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include "FileUtils.h"
#include "PlaylistEngine.h"
#include "LibraryIndex.h"
#include "SongQueue.h"
#include "HistoryStack.h"
#include "UserList.h"
#include "AudioPlayer.h"

/*
=====================================================================
   ECHO ENGINE UNIT TESTS (pure C++98, no STL containers) make test
=====================================================================
*/
static int g_checks = 0, g_failed = 0;

#define CHECK(cond) do { g_checks++; if (!(cond)) { g_failed++; \
    std::cout << "  FAIL " << __FILE__ << ":" << __LINE__ << "  " << #cond << "\n"; } } while (0)

static Song mk(const string& title, const string& artist, int secs) {
    Song s;
    s.title = title; s.artist = artist; s.album = "Album"; s.durationSeconds = secs;
    return s;
}

static string tempName(const char* name) { return string("echo_test_") + name; }   // keep the real extension (.mp3 / .wav)

/*
============================================================
                  splitLine / toInt / helpers
============================================================
*/
static void testSplitLine() {
    string p[MAX_FIELDS];
    CHECK(splitLine("a|b|c|4", '|', p, MAX_FIELDS) == 4);
    CHECK(p[0] == "a" && p[1] == "b" && p[2] == "c" && p[3] == "4");
    CHECK(splitLine("", '|', p, MAX_FIELDS) == 1);
    CHECK(p[0] == "");
    CHECK(splitLine("a||b", '|', p, MAX_FIELDS) == 3);
    CHECK(p[0] == "a" && p[1] == "" && p[2] == "b");
    CHECK(splitLine("  x  |  y y  \r", '|', p, MAX_FIELDS) == 2);   // fields are trimmed, CR removed
    CHECK(p[0] == "x" && p[1] == "y y");
    CHECK(splitLine("1|2|3|4|5", '|', p, 2) == 5);                  // total count even when parts[] is small
    CHECK(p[0] == "1" && p[1] == "2");
    CHECK(splitLine("a|", '|', p, MAX_FIELDS) == 2 && p[1] == "");
}

static void testToInt() {
    int v = -1;
    CHECK(toInt("42", v) && v == 42);
    CHECK(toInt("  7 ", v) && v == 7);
    CHECK(toInt("-15", v) && v == -15);
    CHECK(toInt("0", v) && v == 0);
    CHECK(toInt("123456789", v) && v == 123456789);
    CHECK(!toInt("", v));
    CHECK(!toInt("   ", v));
    CHECK(!toInt("abc", v));
    CHECK(!toInt("12a", v));
    CHECK(!toInt("3.5", v));
    CHECK(!toInt("-", v));
    CHECK(!toInt("1234567890", v));                                 // 10 digits: refused (overflow guard)
}

static void testStringHelpers() {
    CHECK(trim("  hi \t") == "hi");
    CHECK(toLower("AbC") == "abc");
    CHECK(intStr(0) == "0" && intStr(305) == "305" && intStr(-12) == "-12");
    CHECK(safeFileName("a:b*c") == "a_b_c");
    CHECK(userFileName("bob") == "bob");
    CHECK(userFileName("Bob") == "-bob");
    CHECK(userFileName("Bob") != userFileName("bob"));
    CHECK(userFileName("a_B") == "a_-b");
    CHECK(isBlankOrComment("   ") && isBlankOrComment("# x") && !isBlankOrComment("a|b"));
}

/*
============================================================
                       levenshtein
============================================================
*/
static void testLevenshtein() {
    CHECK(levenshtein("kitten", "sitting") == 3);
    CHECK(levenshtein("", "abc") == 3);
    CHECK(levenshtein("abc", "") == 3);
    CHECK(levenshtein("", "") == 0);
    CHECK(levenshtein("same", "same") == 0);
    CHECK(levenshtein("flaw", "lawn") == 2);
    CHECK(levenshtein("ab", "ba") == 2);
}

/*
============================================================
              song line format (shared helpers)
============================================================
*/
static void testSongLines() {
    Song s = mk("Title", "Artist", 215), back;
    string why;
    CHECK(songToLine(s) == "Title|Artist|Album|215");
    CHECK(parseSongLine(songToLine(s), back, why));
    CHECK(back.title == "Title" && back.artist == "Artist" && back.album == "Album" && back.durationSeconds == 215);
    CHECK(!parseSongLine("a|b|c", back, why) && why == "expected 4 fields");
    CHECK(!parseSongLine("|b|c|5", back, why) && why == "empty title or artist");
    CHECK(!parseSongLine("a||c|5", back, why) && why == "empty title or artist");
    CHECK(!parseSongLine("a|b|c|x", back, why) && why == "bad duration");
    CHECK(!parseSongLine("a|b|c|0", back, why) && why == "bad duration");
    CHECK(songKey(mk("Hi", "Bob", 1)) == songKey(mk("HI", "bob", 2)));
    CHECK(songKey(mk("Hi", "Bob", 1)) != songKey(mk("Hi", "Ann", 1)));
    CHECK(songFileStem(mk("T", "A", 1)) == "A - T");
}

/*
============================================================
                    MP3 / WAV length parser
============================================================
*/
static void writeMp3(const string& path, int frames, bool xing, int xingFrames) {
    // MPEG1 Layer III, 128 kbps, 44100 Hz, stereo: header FF FB 90 00, frame = 417 bytes
    FILE* f = fopen(path.c_str(), "wb");
    unsigned char fr[417];
    for (int i = 0; i < frames; i++) {
        memset(fr, 0, sizeof(fr));
        fr[0] = 0xFF; fr[1] = 0xFB; fr[2] = 0x90; fr[3] = 0x00;
        if (xing && i == 0) {
            memcpy(fr + 36, "Xing", 4);
            fr[36 + 7] = 1;                                         // flags: frame count present
            fr[36 + 8] = (unsigned char)(xingFrames >> 24); fr[36 + 9] = (unsigned char)(xingFrames >> 16);
            fr[36 + 10] = (unsigned char)(xingFrames >> 8); fr[36 + 11] = (unsigned char)xingFrames;
        }
        fwrite(fr, 1, sizeof(fr), f);
    }
    fclose(f);
}

static void testMp3Parser() {
    string path = tempName("cbr.mp3");
    writeMp3(path, 1000, false, 0);                                  // 417000 bytes * 8 / 128000 = 26.06 s
    CHECK(nativeLengthSeconds(path) == 26);
    remove(path.c_str());

    writeMp3(path, 50, true, 2000);                                  // Xing says 2000 frames: 2000*1152/44100 = 52.2 s
    CHECK(nativeLengthSeconds(path) == 52);
    remove(path.c_str());

    // ID3v2 tag in front must be skipped
    FILE* f = fopen(path.c_str(), "wb");
    unsigned char id3[10] = { 'I', 'D', '3', 3, 0, 0, 0, 0, 0, 20 };   // 20 bytes of tag body
    unsigned char body[20]; memset(body, 0, sizeof(body));
    fwrite(id3, 1, 10, f); fwrite(body, 1, 20, f);
    fclose(f);
    {
        FILE* g = fopen(path.c_str(), "ab");
        unsigned char fr[417];
        for (int i = 0; i < 500; i++) { memset(fr, 0, sizeof(fr)); fr[0] = 0xFF; fr[1] = 0xFB; fr[2] = 0x90; fwrite(fr, 1, sizeof(fr), g); }
        fclose(g);
    }
    CHECK(nativeLengthSeconds(path) == 13);                          // 500 frames: 208500*8/128000 = 13.03
    remove(path.c_str());

    f = fopen(path.c_str(), "wb");                                   // garbage: no frame found
    unsigned char junk[300]; memset(junk, 7, sizeof(junk));
    fwrite(junk, 1, sizeof(junk), f); fclose(f);
    CHECK(nativeLengthSeconds(path) == 0);
    remove(path.c_str());
    CHECK(nativeLengthSeconds("echo_test_missing.mp3") == 0);
}

static void testWavParser() {
    string path = tempName("pcm.wav");
    FILE* f = fopen(path.c_str(), "wb");
    unsigned char h[44]; memset(h, 0, sizeof(h));
    memcpy(h, "RIFF", 4); memcpy(h + 8, "WAVE", 4); memcpy(h + 12, "fmt ", 4);
    h[16] = 16; h[20] = 1; h[22] = 2;                                // PCM, 2 channels
    unsigned long rate = 44100, byteRate = 176400, dataSize = 176400 * 3;
    h[24] = (unsigned char)(rate); h[25] = (unsigned char)(rate >> 8);
    h[28] = (unsigned char)(byteRate); h[29] = (unsigned char)(byteRate >> 8); h[30] = (unsigned char)(byteRate >> 16);
    memcpy(h + 36, "data", 4);
    h[40] = (unsigned char)(dataSize); h[41] = (unsigned char)(dataSize >> 8); h[42] = (unsigned char)(dataSize >> 16);
    fwrite(h, 1, 44, f);
    unsigned char zeros[4096]; memset(zeros, 0, sizeof(zeros));
    for (unsigned long w = 0; w < dataSize; w += sizeof(zeros)) fwrite(zeros, 1, (dataSize - w < sizeof(zeros)) ? (size_t)(dataSize - w) : sizeof(zeros), f);
    fclose(f);
    CHECK(nativeLengthSeconds(path) == 3);
    remove(path.c_str());
}

/*
============================================================
        PlaylistEngine: circular doubly linked list
============================================================
*/
static void testPlaylist() {
    PlaylistEngine p;
    CHECK(p.isEmpty() && p.getSize() == 0 && p.getCurrentIndex() == -1);
    CHECK(p.getCurrent().title == "");
    p.playNext(); p.playPrevious();                                  // harmless on an empty list

    p.addSong(mk("A", "x", 10));
    CHECK(p.getSize() == 1 && p.getCurrent().title == "A");
    p.playNext();
    CHECK(p.getCurrent().title == "A");                              // a one-song circle points at itself
    p.addSong(mk("B", "x", 20));
    p.addSong(mk("C", "x", 30));
    CHECK(p.getSize() == 3 && p.totalSeconds() == 60);
    CHECK(p.getCurrentIndex() == 0);
    p.playNext(); p.playNext();
    CHECK(p.getCurrent().title == "C");
    p.playNext();                                                    // tail -> head
    CHECK(p.getCurrent().title == "A");
    p.playPrevious();                                                // head -> tail
    CHECK(p.getCurrent().title == "C");

    CHECK(p.getAt(1).title == "B" && p.getAt(5).title == "" && p.getAt(-1).title == "");
    CHECK(p.contains(mk("b", "X", 0)));                              // case-insensitive artist + title
    CHECK(!p.contains(mk("B", "other artist", 0)));

    CHECK(p.moveSong(0, 1));                                         // A swaps with B
    CHECK(p.getAt(0).title == "B" && p.getAt(1).title == "A");
    CHECK(!p.moveSong(0, -1) && !p.moveSong(2, 1));
    CHECK(p.getCurrent().title == "C");                              // the playing track keeps playing

    CHECK(p.jumpTo(mk("A", "x", 0)) && p.getCurrentIndex() == 1);
    CHECK(p.jumpToIndex(2) && p.getCurrent().title == "C");
    CHECK(!p.jumpToIndex(3) && !p.jumpToIndex(-1));
    p.setDuration(2, 99);
    CHECK(p.getAt(2).durationSeconds == 99);

    CHECK(p.removeSong(mk("C", "x", 0)));                            // remove the playing tail
    CHECK(p.getSize() == 2 && p.getCurrent().title == "B");          // current moves on to the next node
    CHECK(!p.removeSong(mk("C", "x", 0)));
    p.playPrevious();
    CHECK(p.getCurrent().title == "A");                              // circle is still closed after removing the tail
    CHECK(p.removeSong(mk("B", "x", 0)));                            // remove the head
    CHECK(p.getSize() == 1 && p.getCurrent().title == "A");
    CHECK(p.removeSong(mk("A", "x", 0)));
    CHECK(p.isEmpty() && p.getCurrentIndex() == -1);
    p.addSong(mk("Z", "z", 5));                                      // reusable after becoming empty
    CHECK(p.getCurrent().title == "Z");
    p.destroy();
    CHECK(p.isEmpty());
}

static void testPlaylistSameTitle() {
    PlaylistEngine p;
    p.addSong(mk("Hello", "Adele", 100));
    p.addSong(mk("Hello", "Lionel", 200));
    CHECK(p.getSize() == 2);
    CHECK(p.contains(mk("Hello", "Adele", 0)) && p.contains(mk("Hello", "Lionel", 0)));
    CHECK(p.removeSong(mk("Hello", "Lionel", 0)));                   // only the right artist is removed
    CHECK(p.getSize() == 1 && p.getAt(0).artist == "Adele");
}

static void testPlaylistWrite() {
    PlaylistEngine p;
    p.addSong(mk("One", "A", 1));
    p.addSong(mk("Two", "B", 2));
    string path = tempName("pl.txt");
    {
        ofstream out(path.c_str());
        p.writeSongs(out);
    }
    ifstream in(path.c_str());
    string l1, l2;
    getline(in, l1); getline(in, l2);
    CHECK(l1 == "One|A|Album|1" && l2 == "Two|B|Album|2");
    in.close();
    remove(path.c_str());
}

/*
============================================================
              Fisher-Yates shuffle (PlaylistEngine)
============================================================
*/
static void testShuffle() {
    srand(12345);
    const int N = 12;
    for (int round = 0; round < 20; round++) {
        PlaylistEngine p;
        char name[16];
        for (int i = 0; i < N; i++) { sprintf(name, "S%02d", i); p.addSong(mk(name, "a", 10)); }
        p.jumpToIndex(round % N);
        int startIdx = p.getCurrentIndex();
        p.shuffleReset();

        int seen[N];
        for (int i = 0; i < N; i++) seen[i] = 0;
        seen[startIdx] = 1;                                          // the playing track counts as played
        for (int step = 1; step < N; step++) {
            CHECK(!p.shuffleCycleDone());
            bool wrapped = p.playNextShuffled();
            CHECK(!wrapped);
            seen[p.getCurrentIndex()]++;
        }
        bool all = true;
        for (int i = 0; i < N; i++) if (seen[i] != 1) all = false;
        CHECK(all);                                                  // every track exactly once, no repeats
        CHECK(p.shuffleCycleDone());

        int last = p.getCurrentIndex();
        bool wrapped = p.playNextShuffled();                         // deck used up: reshuffle
        CHECK(wrapped);
        CHECK(p.getCurrentIndex() != last);                          // never the same track back to back
        CHECK(!p.shuffleCycleDone());
    }

    // the order really is shuffled (not just 0,1,2,...)
    PlaylistEngine q;
    for (int i = 0; i < N; i++) { char nm[16]; sprintf(nm, "T%02d", i); q.addSong(mk(nm, "a", 10)); }
    q.shuffleReset();
    bool inOrder = true;
    int prev = q.getCurrentIndex();
    for (int i = 1; i < N; i++) { q.playNextShuffled(); int c = q.getCurrentIndex(); if (c != (prev + 1) % N) inOrder = false; prev = c; }
    CHECK(!inOrder);

    // shuffleSync follows a manual jump
    q.shuffleReset();
    q.playNextShuffled();
    q.jumpToIndex(5);
    q.shuffleSync();
    bool ok = true;
    for (int i = 0; i < N - 1 && !q.shuffleCycleDone(); i++) q.playNextShuffled();
    CHECK(ok);

    // editing the playlist invalidates the deck, the next move builds a new one
    q.addSong(mk("extra", "a", 10));
    q.playNextShuffled();
    CHECK(q.getSize() == N + 1);

    // one-song and empty playlists
    PlaylistEngine one;
    one.addSong(mk("solo", "a", 5));
    one.shuffleReset();
    CHECK(one.shuffleCycleDone());
    CHECK(one.playNextShuffled() && one.getCurrent().title == "solo");
    PlaylistEngine none;
    none.shuffleReset();
    CHECK(!none.playNextShuffled());
    q.shuffleOff();
}

/*
============================================================
            LibraryIndex: hash table + rehash + search
============================================================
*/
static bool isPrimeNum(int n) {
    if (n < 2) return false;
    for (int d = 2; d * d <= n; d++) if (n % d == 0) return false;
    return true;
}

static void testLibrary() {
    LibraryIndex lib;
    CHECK(lib.getCount() == 0);
    CHECK(lib.getBucketCount() == 101);                              // 100 rounded up to a prime
    lib.insertToLibrary(mk("Blinding Lights", "The Weeknd", 200));
    lib.insertToLibrary(mk("Shape of You", "Ed Sheeran", 234));
    lib.insertToLibrary(mk("Believer", "Imagine Dragons", 204));
    CHECK(lib.getCount() == 3);
    CHECK(lib.searchSong("blinding lights") != NULL);                // case-insensitive
    CHECK(lib.searchSong("Nope") == NULL);
    lib.insertToLibrary(mk("believer", "IMAGINE DRAGONS", 1));       // same artist + title: ignored
    CHECK(lib.getCount() == 3);
    CHECK(lib.exists("Believer", "imagine dragons"));
    CHECK(!lib.exists("Believer", "someone else"));

    // same title, different artist: both are kept and found separately
    lib.insertToLibrary(mk("Believer", "Cher", 220));
    CHECK(lib.getCount() == 4);
    CHECK(lib.countTitle("Believer") == 2);
    Song* a = lib.findSong("Imagine Dragons", "Believer");
    Song* b = lib.findSong("Cher", "believer");
    CHECK(a != NULL && b != NULL && a != b && a->durationSeconds == 204 && b->durationSeconds == 220);
    Song* both[4];
    CHECK(lib.findAllByTitle("Believer", both, 4) == 2);
    CHECK(lib.findByStem("Cher - Believer") == b);                   // file name "Artist - Title"
    CHECK(lib.findByStem("Blinding Lights") != NULL);                // file name = title
    CHECK(lib.findByStem("Unknown - Thing") == NULL);

    // ranked search: exact, substring, artist, typo
    Song* res[10];
    int n = lib.searchRanked("blinding lights", res, 10);
    CHECK(n >= 1 && res[0]->title == "Blinding Lights");
    n = lib.searchRanked("sheeran", res, 10);
    CHECK(n == 1 && res[0]->artist == "Ed Sheeran");
    n = lib.searchRanked("Blindng Lights", res, 10);                 // typo
    CHECK(n >= 1 && res[0]->title == "Blinding Lights");
    n = lib.searchRanked("zzzzqqq", res, 10);
    CHECK(n == 0);
    CHECK(lib.searchRanked("   ", res, 10) == 0);

    Song* all[10];
    n = lib.getAllSorted(all, 10);
    CHECK(n == 4);
    bool sorted = true;
    for (int i = 1; i < n; i++) if (toLower(all[i - 1]->title) > toLower(all[i]->title)) sorted = false;
    CHECK(sorted);
}

static void testRehash() {
    LibraryIndex lib;
    const int N = 700;
    Song* first = NULL;
    int sizes[8], nSizes = 0;
    sizes[nSizes++] = lib.getBucketCount();
    for (int i = 0; i < N; i++) {
        char t[32], a[32];
        sprintf(t, "Track %d", i);
        sprintf(a, "Artist %d", i % 7);
        lib.insertToLibrary(mk(t, a, 100 + i));
        if (i == 0) first = lib.searchSong("Track 0");
        int buckets = lib.getBucketCount();
        CHECK((double)lib.getCount() / buckets <= 0.75);             // load factor never stays above 0.75
        CHECK(isPrimeNum(buckets));
        if (buckets != sizes[nSizes - 1] && nSizes < 8) sizes[nSizes++] = buckets;
    }
    CHECK(lib.getCount() == N);
    CHECK(nSizes == 5);                                              // 101 -> 211 -> 431 -> 863 -> 1733
    CHECK(sizes[0] == 101 && sizes[1] == 211 && sizes[2] == 431 && sizes[3] == 863 && sizes[4] == 1733);
    CHECK(lib.searchSong("Track 0") == first);                       // nodes were re-linked, not copied
    bool allFound = true;
    for (int i = 0; i < N; i++) {
        char t[32], a[32];
        sprintf(t, "Track %d", i);
        sprintf(a, "Artist %d", i % 7);
        Song* s = lib.findSong(a, t);
        if (s == NULL || s->durationSeconds != 100 + i) allFound = false;
    }
    CHECK(allFound);
    Song** all = new Song*[N];
    CHECK(lib.getAllSorted(all, N) == N);
    delete[] all;
    lib.destroy();
    CHECK(lib.getCount() == 0 && lib.searchSong("Track 1") == NULL);
    lib.insertToLibrary(mk("again", "x", 5));                        // usable after destroy
    CHECK(lib.getCount() == 1);
}

static void testLibraryFiles() {
    string path = tempName("songs.txt");
    {
        ofstream out(path.c_str());
        out << "# header\nOne|A|Alb|100\nTwo|B|Alb|200\nbad line\nOne|A|Alb|100\nThree||Alb|5\nFour|D|Alb|zz\n";
    }
    LibraryIndex lib;
    std::streambuf* old = std::cout.rdbuf();
    std::cout.rdbuf(NULL);                                           // loader prints warnings: keep the test output clean
    lib.loadDatabaseFromFile(path);
    std::cout.rdbuf(old);
    std::cout.clear();
    CHECK(lib.getCount() == 2);                                      // bad, duplicate and empty-field lines skipped
    lib.appendSongToFile(mk("Five", "E", 55), path);
    LibraryIndex lib2;
    old = std::cout.rdbuf();
    std::cout.rdbuf(NULL);
    lib2.loadDatabaseFromFile(path);
    std::cout.rdbuf(old);
    std::cout.clear();
    CHECK(lib2.getCount() == 3 && lib2.findSong("E", "Five") != NULL);
    CHECK(lib2.saveDatabaseToFile(path));
    LibraryIndex lib3;
    lib3.loadDatabaseFromFile(path);
    CHECK(lib3.getCount() == 3);
    remove(path.c_str());
}

/*
============================================================
                SongQueue (FIFO) and HistoryStack (LIFO)
============================================================
*/
static void testQueue() {
    SongQueue q;
    CHECK(q.isEmpty() && q.size() == 0 && q.dequeueSong().title == "" && q.peek().title == "");
    q.enqueueSong(mk("1", "a", 1));
    q.enqueueSong(mk("2", "a", 2));
    q.enqueueSong(mk("3", "a", 3));
    CHECK(q.size() == 3 && q.peek().title == "1");
    Song snap[2];
    CHECK(q.snapshot(snap, 2) == 2 && snap[0].title == "1" && snap[1].title == "2");   // front first
    CHECK(q.dequeueSong().title == "1");
    CHECK(q.dequeueSong().title == "2");
    q.enqueueSong(mk("4", "a", 4));                                  // enqueue after partial dequeue
    CHECK(q.dequeueSong().title == "3");
    CHECK(q.dequeueSong().title == "4");
    CHECK(q.isEmpty());
    q.enqueueSong(mk("5", "a", 5));                                  // rear was reset when it emptied
    CHECK(q.size() == 1 && q.peek().title == "5");
    q.destroy();
    CHECK(q.isEmpty() && q.size() == 0);
}

static void testHistory() {
    HistoryStack h;
    CHECK(h.isEmpty() && h.pop().title == "");
    h.pushToHistory(mk("1", "a", 1));
    h.pushToHistory(mk("2", "a", 2));
    h.pushToHistory(mk("3", "a", 3));
    CHECK(h.size() == 3);
    Song snap[5];
    CHECK(h.snapshot(snap, 5) == 3 && snap[0].title == "3" && snap[2].title == "1");   // newest first
    CHECK(h.pop().title == "3");
    CHECK(h.pop().title == "2");
    CHECK(h.size() == 1);
    h.pushToHistory(mk("4", "a", 4));
    CHECK(h.pop().title == "4" && h.pop().title == "1" && h.isEmpty());
    for (int i = 0; i < 100; i++) h.pushToHistory(mk("x", "a", i + 1));
    CHECK(h.size() == 100);
    h.destroy();
    CHECK(h.isEmpty() && h.size() == 0);
}

/*
============================================================
                         UserList
============================================================
*/
static void testUsers() {
    UserList u;
    CHECK(u.getCount() == 0 && !u.authenticateUser("a", "b"));
    CHECK(u.addUser("bob", "pw1"));
    CHECK(u.addUser("Bob", "pw2"));                                  // case-sensitive: a different user
    CHECK(!u.addUser("bob", "other"));
    CHECK(u.getCount() == 2);
    CHECK(u.authenticateUser("bob", "pw1") && !u.authenticateUser("bob", "pw2"));
    CHECK(u.authenticateUser("Bob", "pw2") && !u.authenticateUser("Bob", "pw1"));
    CHECK(!u.authenticateUser("BOB", "pw1"));
    CHECK(u.exists("bob") && u.exists("Bob") && !u.exists("bOb"));

    string path = tempName("users.txt");
    {
        ofstream out(path.c_str());
        out << "# comment\nuser1|1234\nuser2|abcd|old\nbroken\nuser1|dup\n|nopass\n";
    }
    UserList f;
    std::streambuf* old = std::cout.rdbuf();
    std::cout.rdbuf(NULL);
    f.loadUsers(path);
    std::cout.rdbuf(old);
    std::cout.clear();
    CHECK(f.getCount() == 2);                                        // duplicate, broken and empty-name lines skipped
    CHECK(f.authenticateUser("user1", "1234") && f.authenticateUser("user2", "abcd"));
    CHECK(f.appendUserToFile("newbie", "pass", path));
    UserList g;
    old = std::cout.rdbuf();
    std::cout.rdbuf(NULL);
    g.loadUsers(path);
    std::cout.rdbuf(old);
    std::cout.clear();
    CHECK(g.getCount() == 3 && g.authenticateUser("newbie", "pass"));
    remove(path.c_str());
    {
        ofstream out(path.c_str(), ios::binary);
        out << "last|line";                                          // no final newline
    }
    CHECK(f.appendUserToFile("next", "pw", path));
    UserList h;
    h.loadUsers(path);
    CHECK(h.getCount() == 2 && h.authenticateUser("last", "line") && h.authenticateUser("next", "pw"));
    remove(path.c_str());
}

/*
============================================================
                          main
============================================================
*/
int main() {
    srand(1);
    testSplitLine();
    testToInt();
    testStringHelpers();
    testLevenshtein();
    testSongLines();
    testMp3Parser();
    testWavParser();
    testPlaylist();
    testPlaylistSameTitle();
    testPlaylistWrite();
    testShuffle();
    testLibrary();
    testRehash();
    testLibraryFiles();
    testQueue();
    testHistory();
    testUsers();
    std::cout << g_checks << " checks, " << g_failed << " failed\n";
    return g_failed == 0 ? 0 : 1;
}
