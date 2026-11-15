#ifndef USER_LIST_H
#define USER_LIST_H

#include "Structs.h"
#include "FileUtils.h"

/*
==============================================================
		Singly Linked List (authentication, users.txt)
  Every account is equal: anyone can play, search and upload.
==============================================================
*/
class UserList {
private:
    UserNode* head;
    int count;

    UserList(const UserList&);
    UserList& operator=(const UserList&);

    bool hasUser(const string& name) const {
        for (UserNode* n = head; n != NULL; n = n->next)
            if (n->username == name) return true;
        return false;
    }

public:
    UserList() : head(NULL), count(0) {}

    // File format: Username|Password
    // (an old optional third field is accepted and ignored, so existing files still work)
    void loadUsers(const string& filename) {
        ifstream in(filename.c_str());
        if (!in) return;
        string line, p[MAX_FIELDS];
        int lineNo = 0;
        while (getline(in, line)) {
            lineNo++;
            if (isBlankOrComment(line)) continue;
            int fields = splitLine(line, FIELD_DELIM, p, MAX_FIELDS);
            if (fields != 2 && fields != 3)                        { reportBadLine(filename, lineNo, "expected Username|Password"); continue; }
            if (p[0].empty() || p[1].empty())                      { reportBadLine(filename, lineNo, "empty username or password"); continue; }
            if (hasUser(p[0]))                                     { reportBadLine(filename, lineNo, "duplicate username"); continue; }
            UserNode* n = new UserNode();
            n->username = p[0]; n->password = p[1];
            n->next = head;                                        // O(1) insert at head
            head = n;
            count++;
        }
    }

    // true if the username exists and the password matches
    bool authenticateUser(const string& username, const string& password) {
        for (UserNode* n = head; n != NULL; n = n->next)
            if (n->username == username) return n->password == password;
        return false;
    }

    // Case-sensitive: "Bob" and "bob" are two different accounts.
    bool exists(const string& name) const { return hasUser(name); }

    // Adds an account in memory (O(1)). false if the name is taken.
    bool addUser(const string& name, const string& password) {
        if (hasUser(name)) return false;
        UserNode* n = new UserNode();
        n->username = name; n->password = password;
        n->next = head;
        head = n;
        count++;
        return true;
    }

    // Appends "name|password" to users.txt (fixes a missing final newline first).
    bool appendUserToFile(const string& name, const string& password, const string& filename) const {
        bool needNewline = false;
        {
            ifstream in(filename.c_str(), ios::in | ios::binary);
            if (in) {
                in.seekg(0, ios::end);
                if (in.tellg() > 0) { in.seekg(-1, ios::end); char c = 0; in.get(c); needNewline = (c != '\n'); }
            }
        }
        ofstream out(filename.c_str(), ios::app);
        if (!out) return false;
        if (needNewline) out << "\n";
        out << name << FIELD_DELIM << password << "\n";
        return out.good();
    }

    int getCount() const { return count; }

    void destroy() {
        while (head != NULL) { UserNode* t = head; head = head->next; delete t; }
        count = 0;
    }
    ~UserList() { destroy(); }
};

#endif
