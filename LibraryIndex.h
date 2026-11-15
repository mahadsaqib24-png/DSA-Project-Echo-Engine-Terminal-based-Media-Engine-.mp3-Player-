#ifndef LIBRARY_INDEX_H
#define LIBRARY_INDEX_H
#include "Structs.h"
#include "FileUtils.h"
#include "Constants.h"

/*
==================================================================================
		Hash table with chaining + songs.txt filing
	Lookup by title is O(1) average (polynomial rolling hash).
	The table grows to the next prime size when the load factor
	goes above 0.75 (101 -> 211 -> 431 -> 863 ...).
	A track is unique by artist + title, so one title can have
	several entries. searchRanked() adds substring + Levenshtein typo tolerance.
==================================================================================
*/
class LibraryIndex {
private:
    HashNode** table;
    int tableSize;
    int count;

    LibraryIndex(const LibraryIndex&);
    LibraryIndex& operator=(const LibraryIndex&);

    // polynomial rolling hash on the lower-cased title
    static int hashWith(const string& title, int size) {
        string t = toLower(title);
        unsigned long h = 0;
        for (size_t i = 0; i < t.size(); i++)
            h = (h * 31 + (unsigned char)t[i]) % (unsigned long)size;
        return (int)h;
    }

    int hashFunction(const string& title) const { return hashWith(title, tableSize); }

    static bool isPrime(int n) {
        if (n < 2) return false;
        for (int d = 2; d * d <= n; d++)
            if (n % d == 0) return false;
        return true;
    }

    static int nextPrime(int n) {                      // smallest prime >= n
        if (n < 2) n = 2;
        while (!isPrime(n)) n++;
        return n;
    }

    // Re-links every node into a new bucket array (nodes are not copied, so Song* stay valid).
    void rehash(int newSize) {
        HashNode** bigger = new HashNode*[newSize];
        for (int i = 0; i < newSize; i++) bigger[i] = NULL;
        for (int b = 0; b < tableSize; b++) {
            HashNode* n = table[b];
            while (n != NULL) {
                HashNode* next = n->next;
                int h = hashWith(n->data.title, newSize);
                n->next = bigger[h];
                bigger[h] = n;
                n = next;
            }
        }
        delete[] table;
        table = bigger;
        tableSize = newSize;
    }

    // Lower score = better match. -1 = no match.
    // 0 exact, 1 prefix, 2 substring, 3 artist/album hit, 10+d typo of distance d.
    static int scoreSong(const Song& s, const string& q) {
        string t = toLower(s.title);
        if (t == q) return 0;
        if (t.compare(0, q.size(), q) == 0) return 1;
        if (t.find(q) != string::npos) return 2;
        string ar = toLower(s.artist);
        if (ar.find(q) != string::npos || toLower(s.album).find(q) != string::npos) return 3;

        int thr = ((int)q.size() + 2) / 3;                 // allowed typos grow with length
        if (q.size() < 3) return -1;
        int best = levenshtein(q, t);
        int d = levenshtein(q, ar);
        if (d < best) best = d;
        size_t start = 0;                                  // also compare against each title word
        while (start <= t.size()) {
            size_t sp = t.find(' ', start);
            if (sp == string::npos) sp = t.size();
            if (sp > start) {
                d = levenshtein(q, t.substr(start, sp - start));
                if (d < best) best = d;
            }
            start = sp + 1;
        }
        return best <= thr ? 10 + best : -1;
    }

    static void insertRanked(Song* out[], int scores[], int& n, int maxOut, Song* s, int score) {
        if (n == maxOut && score >= scores[n - 1]) return;
        int i = (n < maxOut) ? n++ : n - 1;
        while (i > 0 && scores[i - 1] > score) { out[i] = out[i - 1]; scores[i] = scores[i - 1]; i--; }
        out[i] = s;
        scores[i] = score;
    }

    // title first, artist breaks ties (so the order is the same every run)
    static bool lessSong(const Song& a, const Song& b) {
        string ta = toLower(a.title), tb = toLower(b.title);
        if (ta != tb) return ta < tb;
        return toLower(a.artist) < toLower(b.artist);
    }

    static void mergeSort(Song* a[], Song* tmp[], int lo, int hi) {   // sorts a[lo..hi) by title
        if (hi - lo < 2) return;
        int mid = (lo + hi) / 2;
        mergeSort(a, tmp, lo, mid);
        mergeSort(a, tmp, mid, hi);
        int i = lo, j = mid, k = lo;
        while (i < mid && j < hi)
            tmp[k++] = lessSong(*a[j], *a[i]) ? a[j++] : a[i++];
        while (i < mid) tmp[k++] = a[i++];
        while (j < hi)  tmp[k++] = a[j++];
        for (k = lo; k < hi; k++) a[k] = tmp[k];
    }

public:
    LibraryIndex(int size = HASH_INITIAL_SIZE) : tableSize(nextPrime(size > 0 ? size : HASH_INITIAL_SIZE)), count(0) {
        table = new HashNode*[tableSize];
        for (int i = 0; i < tableSize; i++) table[i] = NULL;
    }

    bool exists(const string& title, const string& artist) const {
        string key = songKey(artist, title);
        for (HashNode* n = table[hashFunction(title)]; n != NULL; n = n->next)
            if (songKey(n->data) == key) return true;
        return false;
    }

    // O(1) average: insert at the head of the chain (ignores same artist + title). Rehashes when too full.
    void insertToLibrary(const Song& s) {
        if (exists(s.title, s.artist)) return;
        HashNode* n = new HashNode();
        n->data = s;
        int h = hashFunction(s.title);
        n->next = table[h];
        table[h] = n;
        count++;
        if ((double)count / tableSize > HASH_MAX_LOAD) rehash(nextPrime(tableSize * 2));
    }

    // O(1) average. First song with this title, or NULL. Pointer stays valid until destroy().
    Song* searchSong(const string& title) {
        string key = toLower(title);
        for (HashNode* n = table[hashFunction(title)]; n != NULL; n = n->next)
            if (toLower(n->data.title) == key) return &n->data;
        return NULL;
    }

    // O(1) average. The exact track (artist + title), or NULL.
    Song* findSong(const string& artist, const string& title) {
        string key = songKey(artist, title);
        for (HashNode* n = table[hashFunction(title)]; n != NULL; n = n->next)
            if (songKey(n->data) == key) return &n->data;
        return NULL;
    }

    // How many songs share this title (different artists).
    int countTitle(const string& title) const {
        string key = toLower(title);
        int c = 0;
        for (HashNode* n = table[hashFunction(title)]; n != NULL; n = n->next)
            if (toLower(n->data.title) == key) c++;
        return c;
    }

    // Fills out[] with the songs that have this title. Returns how many.
    int findAllByTitle(const string& title, Song* out[], int maxOut) {
        string key = toLower(title);
        int c = 0;
        for (HashNode* n = table[hashFunction(title)]; n != NULL && c < maxOut; n = n->next)
            if (toLower(n->data.title) == key) out[c++] = &n->data;
        return c;
    }

    // The song that an audio file belongs to: title == file name, or "Artist - Title" == file name.
    Song* findByStem(const string& stem) {
        Song* s = searchSong(stem);
        if (s != NULL) return s;
        string key = toLower(stem);
        for (int b = 0; b < tableSize; b++)
            for (HashNode* h = table[b]; h != NULL; h = h->next)
                if (toLower(songFileStem(h->data)) == key) return &h->data;
        return NULL;
    }

    // Typo-tolerant search: fills out[] (best first) and returns how many were found.
    int searchRanked(const string& text, Song* out[], int maxOut) {
        string query = toLower(trim(text));
        if (query.empty() || maxOut <= 0) return 0;
        int* scores = new int[maxOut];
        int n = 0;
        for (int b = 0; b < tableSize; b++)
            for (HashNode* h = table[b]; h != NULL; h = h->next) {
                int sc = scoreSong(h->data, query);
                if (sc >= 0) insertRanked(out, scores, n, maxOut, &h->data, sc);
            }
        delete[] scores;
        return n;
    }

    // Fills out[] with every song sorted by title (merge sort). out must hold getCount() entries.
    int getAllSorted(Song* out[], int maxOut) {
        int n = 0;
        for (int b = 0; b < tableSize && n < maxOut; b++)
            for (HashNode* h = table[b]; h != NULL && n < maxOut; h = h->next) out[n++] = &h->data;
        if (n > 1) {
            Song** tmp = new Song*[n];
            mergeSort(out, tmp, 0, n);
            delete[] tmp;
        }
        return n;
    }

    int getCount() const { return count; }
    int getBucketCount() const { return tableSize; }

    // global database filing: Title|Artist|Album|DurationSecs
    void loadDatabaseFromFile(const string& filename) {
        ifstream in(filename.c_str());
        if (!in) return;
        string line, why;
        int lineNo = 0;
        while (getline(in, line)) {
            lineNo++;
            if (isBlankOrComment(line)) continue;
            Song s;
            if (!parseSongLine(line, s, why))     { reportBadLine(filename, lineNo, why); continue; }
            if (exists(s.title, s.artist))        { reportBadLine(filename, lineNo, "duplicate track"); continue; }
            insertToLibrary(s);
        }
    }

    void appendSongToFile(const Song& s, const string& filename) {
        bool needNewline = false;
        {
            ifstream in(filename.c_str(), ios::in | ios::binary);
            if (in) {
                in.seekg(0, ios::end);
                if (in.tellg() > 0) { in.seekg(-1, ios::end); char c = 0; in.get(c); needNewline = (c != '\n'); }
            }
        }
        ofstream out(filename.c_str(), ios::app);
        if (!out) return;
        if (needNewline) out << "\n";
        out << songToLine(s) << "\n";
    }

    // Rewrites the whole database (sorted by title). Used after song lengths were corrected.
    bool saveDatabaseToFile(const string& filename) {
        Song** all = new Song*[count > 0 ? count : 1];
        int n = getAllSorted(all, count);
        ofstream out(filename.c_str());
        if (out) {
            out << "# Echo Engine song database\n# Format: Title|Artist|Album|DurationSecs\n";
            for (int i = 0; i < n; i++) out << songToLine(*all[i]) << "\n";
        }
        delete[] all;
        return out.good();
    }

    void destroy() {
        for (int i = 0; i < tableSize; i++) {
            HashNode* n = table[i];
            while (n != NULL) { HashNode* t = n; n = n->next; delete t; }
            table[i] = NULL;
        }
        count = 0;
    }
    ~LibraryIndex() { destroy(); delete[] table; }
};

#endif
