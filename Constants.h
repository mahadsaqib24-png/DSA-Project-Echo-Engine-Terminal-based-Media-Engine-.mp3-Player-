#ifndef CONSTANTS_H
#define CONSTANTS_H

/*
============================================================
             ECHO ENGINE: NAMED CONSTANTS BLOCK
  	Every layout, timing and limit number lives here.
  	Set consts according to your desires (Customization ++)
============================================================
*/

/*
============================================================
                         Layout
============================================================
*/
const int UI_WIDTH          = 104;		// widest banner / header / divider (Set even, PLEASE T_T)
const int UI_MIN_WIDTH      = 40;		// never go narrower than this
const int UI_WIDTH_STEP     = 4;		// banner width is rounded down to a multiple of this

const int PLAYER_DISC_R     = 4;		// disc radius in rows
const int PLAYER_EQ_H       = 4;		// equalizer height in rows
const int PLAYER_BARS       = 14;		// equalizer bars
const int PLAYER_MIN_COLS   = 46;		// smallest window the player can draw in
const int PLAYER_EXTRA_ROWS = 7;		// rows needed besides the disc (title, help, volume, message)
const int PLAYER_SLIDE_W    = 34;		// width of the title slide on a skip

const int HELP_PAGES        = 4;
const int HISTORY_SHOWN     = 15;   	// history lines printed
const int STACK_CAP         = 5;    	// stack animation depth
const int QUEUE_VIEW_MAX    = 8;		// songs copied for the queue animation
const int BROWSE_PAGE_SIZE  = 15;		// Num of Songs-per-search-page
const int EDITOR_PAGE_SIZE  = 10;
const int SEARCH_RESULTS    = 10;

/*
============================================================
                        Timing
============================================================
*/
const int    PLAYER_FRAME_MS    = 80;		// one player frame
const int    MSG_TTL_FRAMES     = 25;		// how long a player message stays
const int    END_MSG_TTL_FRAMES = 60;		// "End of playlist" stays longer
const int    SCRATCH_FRAMES     = 10;		// vinyl scratch effect length
const double PREV_RESTART_SEC   = 3.0;		// [p] restarts the track after this many seconds
const int    PAUSE_SHORT_MS     = 700;		// short pause after a message
const int    PAUSE_MEDIUM_MS    = 1000;
const int    PAUSE_LONG_MS      = 1500;
#ifdef _WIN32
const double VOLUME_DEBOUNCE_SEC = 0.0;  	// MCI changes volume without restarting
#else
const double VOLUME_DEBOUNCE_SEC = 0.25; 	// external players restart: wait until the keys stop
#endif

/*
============================================================
                   Data and account limits
============================================================
*/
const int    HASH_INITIAL_SIZE  = 100;   	// first bucket count (rounded up to a prime)
const double HASH_MAX_LOAD      = 0.75;  	// rehash when count / buckets goes above this

const int MAX_PLAYLISTS     = 10;
const int MAX_PLAYLIST_NAME = 24;

const int LOGIN_ATTEMPTS    = 3;
const int USERNAME_MIN      = 3;
const int USERNAME_MAX      = 16;
const int PASSWORD_MIN      = 4;
const int PASSWORD_MAX      = 32;

#endif
