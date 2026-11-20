// Echo Engine: Animation Lab
// Build:  g++ AnimationLab.cpp -o lab
// ASCII fallback:  g++ -DECHO_ASCII AnimationLab.cpp -o lab
#include "animations.h"

int main() {
    initTerminal();

    const string playlist[4] = {"Blinding Lights", "Shape of You", "Believer", "Levitating"};

    AnimTrack tracks[4];
    tracks[0].title = "Blinding Lights"; tracks[0].artist = "The Weeknd";     tracks[0].album = "After Hours"; tracks[0].duration = 200;
    tracks[1].title = "Shape of You";    tracks[1].artist = "Ed Sheeran";     tracks[1].album = "Divide";      tracks[1].duration = 234;
    tracks[2].title = "Believer";        tracks[2].artist = "Imagine Dragons";tracks[2].album = "Evolve";      tracks[2].duration = 204;
    tracks[3].title = "Levitating (Featuring DaBaby) [Remastered Deluxe Edition]";
    tracks[3].artist = "Dua Lipa";       tracks[3].album = "Future Nostalgia";tracks[3].duration = 203;

    const string history[2] = {"Blinding Lights", "Shape of You"};
    const string newPlays[3] = {"Believer", "Levitating", "Stay"};
    const string upNext[2]  = {"Believer", "Stay"};
    const string toQueue[2] = {"Levitating", "Peaches"};

    while (true) {
        printSectionHeader("ANIMATION LAB");
        cout << "  Theme: " << T().primary << T().name << RESET << "\n\n"
             << "   1. Splash screen\n"
             << "   2. Login (spinner)\n"
             << "   3. Loading library (progress bar)\n"
             << "   4. Now Playing (equalizer + progress)\n"
             << "   5. Long-title marquee\n"
             << "   6. Track transition (next)\n"
             << "   7. Track transition (previous)\n"
             << "   8. Circular list pointer walk (wrap-around)\n"
             << "   9. Hash lookup visualiser\n"
             << "  10. Artist upload sequence\n"
             << "  11. Spinning disc + equalizer\n"
             << "  12. LIVE PLAYER (keyboard, shuffle, repeat, volume, timer)\n"
             << "  13. Stack animation (push / pop)\n"
             << "  14. Queue animation (enqueue / dequeue)\n"
             << "  15. Boot sequence (system check)\n"
             << "  16. Colour theme selector\n"
             << "  17. colours.h classics (processing, updating, saving, loading)\n"
             << "   0. Exit\n\n  Choose: ";
        int c;
        if (!(cin >> c) || c == 0) break;
        cout << "\n";

        switch (c) {
            case 1:  splashAnimation(); break;
            case 2:  loginAnimation("artist1", 1); loginAnimation("wronguser", -1); break;
            case 3:  progressBarAnimation("Loading songs.txt", 40); printSuccess("3 songs indexed."); break;
            case 4:  nowPlayingAnimation("Blinding Lights", "The Weeknd", "After Hours", 200, 6); break;
            case 5:  marqueeAnimation("Stairway to Heaven (Remastered 2012 Deluxe Edition)"); break;
            case 6:  trackTransition(playlist[0], playlist[1], true); break;
            case 7:  trackTransition(playlist[1], playlist[0], false); break;
            case 8:  circularListAnimation(playlist, 4, 6, true); break;
            case 9:  hashLookupAnimation("Believer", 100, 3, 1); break;
            case 10:
                spinnerAnimation("Validating track details...", 14);
                progressBarAnimation("Writing songs.txt   ", 25);
                spinnerAnimation("Inserting into hash table...", 14);
                printSuccess("Track is now available to all users.");
                break;
            case 11: spinningDiscAnimation("Blinding Lights", "The Weeknd", "After Hours", 200, 8); break;
            case 12: livePlayer(tracks, 4, 10.0); break;
            case 13: stackAnimation(history, 2, newPlays, 3); break;
            case 14: queueAnimation(upNext, 2, toQueue, 2); break;
            case 15: bootSequence(3, 2); break;
            case 16: themeMenu(); break;
            case 17:
                processingAnimation(8, "songs.txt");
                updatingAnimation(8, "hash table");
                savingAnimation(8, "playlist.txt");
                loadingAnimation(8, "library");
                cout << "\n  Failure variants:\n";
                savingAnimation(8, "users.txt", false);
                loadingAnimation(8, "library", false);
                cout << "\n  Old-style calls still work:\n";
                processingAnimation(5);
                break;
            default: printWarn("Unknown option."); break;
        }
    }
    cout << SHOW_CURSOR;
    printInfo("Bye!");
    return 0;
}
