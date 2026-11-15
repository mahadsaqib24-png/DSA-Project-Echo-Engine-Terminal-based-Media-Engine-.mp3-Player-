#ifndef PLAYBACK_MANAGER_H
#define PLAYBACK_MANAGER_H

#include "SongQueue.h"
#include "HistoryStack.h"
#include "UserList.h"

/*
============================================================
       MODULE 3: Facade over Queue + History + Users
============================================================
*/
class PlaybackManager {
private:
    SongQueue    queue;
    HistoryStack history;
    UserList     users;

public:
    void addToUpNext(const Song& s)  { queue.enqueueSong(s); }
    bool hasUpNext() const    { return !queue.isEmpty(); }
    int  upNextCount() const  { return queue.size(); }
    Song getNextSong()        { return queue.dequeueSong(); }
    void recordPlay(const Song& s)   { history.pushToHistory(s); }

    int  queueSnapshot(Song out[], int n) const   { return queue.snapshot(out, n); }     // front first
    int  historySnapshot(Song out[], int n) const { return history.snapshot(out, n); }   // newest first
    int  historyCount() const { return history.size(); }
    bool historyEmpty() const { return history.isEmpty(); }
    Song popHistory()         { return history.pop(); }

    void showUpNext() const   { queue.displayQueue(); }
    void showHistory() const  { history.displayHistory(); }

    void loadUsers(const string& filename)  { users.loadUsers(filename); }
    bool login(const string& username, const string& pass) { return users.authenticateUser(username, pass); }
    int  userCount() const                    { return users.getCount(); }
    bool userExists(const string& name) const { return users.exists(name); }

    // Sign-up: write to users.txt first, then add to the in-memory list.
    bool registerUser(const string& name, const string& pass, const string& filename) {
        if (users.exists(name)) return false;
        if (!users.appendUserToFile(name, pass, filename)) return false;
        return users.addUser(name, pass);
    }

    // Called on logout: queue and history belong to one session.
    void resetSession()       { queue.destroy(); history.destroy(); }
};

#endif
