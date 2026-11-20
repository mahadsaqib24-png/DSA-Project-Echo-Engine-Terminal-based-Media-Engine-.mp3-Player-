<div align="center">

# 🎵🎶 Echo Engine 🎶🎵

### Your music. Your playlists. Your terminal. 🔥

**A console-based music player with playlists, a play queue, and listening history, built in C++.**

![Platform](https://img.shields.io/badge/platform-Windows%20%7C%20Linux-blue?style=for-the-badge)
![Language](https://img.shields.io/badge/language-C%2B%2B98-00599C?style=for-the-badge)
![Audio](https://img.shields.io/badge/audio-MP3%20%7C%20WAV%20%7C%20WMA-orange?style=for-the-badge)
![DSA](https://img.shields.io/badge/powered%20by-DSA-ff69b4?style=for-the-badge)

🎧 &nbsp;**Drop your songs → Hit play → Watch the data structures dance!**&nbsp; 💃

</div>

---

## ✨ What Can It Do?

| | Feature | Details |
|:-:|---------|---------|
| 💿 | **Full-screen player** | Spinning disc, live equalizer, progress bar, volume meter and a scrolling title |
| 🎛️ | **Playback controls** | Pause, next, previous, shuffle, repeat (off / all / one), volume and a sleep timer |
| 🎨 | **Colour themes** | Neon, Ocean, Retro Green and Sunset |
| 🎧 | **Auto song import** | Drop audio files into `music/` and they're imported on startup, no fuss |
| 👤 | **User accounts** | Log in or create your very own account |
| 📂 | **Playlists** | Up to **10 playlists per user**: create, rename, delete, reorder and edit |
| ⏭️ | **Up Next queue** | Watch the queue come alive with a **queue animation** |
| 🕘 | **Listening history** | Travel back in time with a **stack animation** |
| 🔎 | **Typo-tolerant search** | Search by title, artist or album, and "believr" still finds "Believer" |
| ⬆️ | **Upload tracks** | Add your own songs to the shared library |
| ⏱️ | **Accurate song lengths** | Read straight from the MP3 / WAV files and re-checked at every launch |
| 🔤 | **Accent support** | Song names with accents show up perfectly |
| ❓ | **Built-in help** | Every Master Dashboard option explained |

---

## 🚀 Quick Start

> ⏱️ Three steps and you're jamming!

### 📁 Step 1: Set up your folder

Put **all** project files together in **one** folder, for example `C:\EchoEngine` or `~/EchoEngine`.

Echo Engine keeps its `data/` and `music/` folders **next to the executable**. Copy your audio files into `music/` 🎶 (`.mp3`, `.wav`, `.wma`, plus `.ogg`, `.flac` and `.m4a` on Linux).

```
EchoEngine/
├── 📄 EchoEngine.dev   (Dev-C++ project)
├── 📄 Makefile         (Linux build)
├── 📄 ... (source files)
├── 💾 data/       ← created automatically
└── 🎵 music/
    ├── song1.mp3
    └── song2.mp3
```

> 🛡️ If the program folder is write-protected, Echo Engine switches automatically to `%LOCALAPPDATA%\EchoEngine` (Linux: `~/EchoEngine`) and tells you at startup.

### 🔨 Step 2: Build & run

The program is written in **C++98** and needs six source files:
`main.cpp`, `MediaPlayerApp.cpp`, `Dashboard.cpp`, `PlayerEngine.cpp`, `animations.cpp` and `colours.cpp`. Every `.h` file is a header and just has to sit in the same folder.

<details open>
<summary><b>🪟 Windows</b></summary>

<br>

**🧰 Option A: Dev-C++ IDE**

1. Open `EchoEngine.dev`.
2. Go to **Project → Project Options → Parameters → Linker** and make sure it contains:
   ```
   -lwinmm -static
   ```
3. Press **F11** to compile and run 🚀 (the program is created as `EchoEngine.exe`)

**⌨️ Option B: Command line (MinGW)**

```bash
g++ -std=c++98 main.cpp MediaPlayerApp.cpp Dashboard.cpp PlayerEngine.cpp animations.cpp colours.cpp -o EchoEngine.exe -lwinmm -static
```

> 💡 **Pro tip:** Use **Windows Terminal** and make the window at least **80 columns wide** (80 × 30 or bigger looks even nicer ✨).

</details>

<details open>
<summary><b>🐧 Linux</b></summary>

<br>

```bash
# 1️⃣ Install a music player (once)
sudo apt install mpv

# 2️⃣ Build
make

# 3️⃣ Run
./echoengine
```

No `make`? Compile by hand (and do **not** add `-lwinmm` on Linux):

```bash
g++ -std=c++98 main.cpp MediaPlayerApp.cpp Dashboard.cpp PlayerEngine.cpp animations.cpp colours.cpp -o echoengine
```

🔊 Echo Engine uses the first player it finds: **`mpv`** (recommended) → `ffplay` → `mpg123` (MP3 only). Installing `ffmpeg` also gives `ffprobe` for exact lengths of non-MP3 files. With no player installed, playback is **simulated** (the disc, equalizer and timer still run).

</details>

> 🔣 Seeing odd boxes instead of the block and spinner characters? Add `-DECHO_ASCII` to the compile command for a plain-ASCII look.

### 🔐 Step 3: Log in

Jump in with a demo account:

| 👤 Username | 🔑 Password |
|:-----------:|:-----------:|
| `user1`     | `1234`      |
| `user2`     | `1234`      |

Or choose **"Create an account"** to make it yours 😎 (username: 3-16 letters, digits or `_`; password: 4-32 characters, no spaces or `|`). Usernames are case-sensitive.

---

## 🧭 Your Menu Map

| Menu | What it does |
|:----:|--------------|
| **1**  | ▶️ **Play my playlist**: opens the full-screen player (Up Next songs play first) |
| **2**  | 🔎 **Search songs**: typos are fine |
| **3**  | 📚 **Browse library**: every song sorted A-Z |
| **4**  | 📂 **My playlists**: manage up to 10 per user |
| **5**  | ⏭️ **Up Next**: shows the queue animation |
| **6**  | 🕘 **Listening history**: shows the stack animation |
| **7**  | 💾 **Save playlists** (they also save automatically on logout) |
| **8**  | 🎨 **Change colour theme** |
| **9**  | ⬆️ **Upload new track** |
| **10** | 🔄 **Rescan** the `music/` folder while the program is running |
| **11** | ❓ **Help**: explains every option of the Master Dashboard |
| **0**  | 🚪 **Logout** |

> 🎵 Songs in `music/` are imported automatically every time the program starts.

---

## 🎹 Player Keys

No Enter needed, just press!

| Key | Action | Key | Action |
|:---:|--------|:---:|--------|
| `space` | Play / pause | `s` | Shuffle on / off |
| `n` | Next track | `r` | Repeat: off → all → one |
| `p` | Previous track (restarts it after 3 s) | `+` `-` | Volume up / down |
| `h` | Change theme | `t` | Sleep timer: 15, 30, 60 min |
| `q` | Close the player | | |

**Playback order:** 🥇 Up Next songs play first → 🥈 then your active playlist continues and loops from the last song back to the first → 🥉 with repeat off, the player stops after the last song.

---

## 🛠️ Something Look Off?

<details>
<summary><b>🔇 No sound, and the screen says "(simulated)"</b></summary>

<br>

Check that the audio file is inside `music/` next to the program and that its name matches the song (`Title.mp3` or `Artist - Title.mp3`). Then run menu **10** (Rescan). On Windows, make sure the build used `-lwinmm`. On Linux, install a player (`sudo apt install mpv`).

</details>

<details>
<summary><b>😵 Strange symbols instead of blocks, spinners or accented letters</b></summary>

<br>

Use **Windows Terminal**, or compile with `-DECHO_ASCII` for a plain-ASCII look.

</details>

<details>
<summary><b>⏱️ Song lengths look wrong</b></summary>

<br>

Relax, nothing to do! 😌 Lengths are read from the MP3 / WAV files themselves and verified on every start (or with menu **10**), and any wrong value is **corrected automatically**.

</details>

<details>
<summary><b>🖥️ "Window too small" in the player</b></summary>

<br>

Enlarge the console. The player needs at least **46 × 16** characters; 80 × 30 is comfortable.

</details>

<details>
<summary><b>🔑 Login always fails</b></summary>

<br>

Use **Create an account** on the welcome screen, or check `data/users.txt` (format `Username|Password`, one account per line). Remember that usernames are case-sensitive.

</details>

<details>
<summary><b>🧱 Build error: undefined reference to <code>mciSendStringW</code></b></summary>

<br>

`-lwinmm` is missing. In Dev-C++ it must go in the **Linker** box, not the Compiler box.

</details>

---

## 📋 Requirements

| Platform | You'll need |
|----------|-------------|
| 🪟 **Windows** | Windows 10 or newer, Dev-C++ / MinGW (links against `winmm`), a terminal with ANSI + UTF-8 support (Windows Terminal recommended) |
| 🐧 **Linux**   | GCC with C++98 support, `make`, and one of `mpv` / `ffplay` / `mpg123` |

---

## 🧪 Unit Tests

The data structures, parsers, search and shuffle logic are covered by `UnitTests.cpp` (plain C++98, no testing framework):
(It is not a part of the project, just a checking file.)

```bash
make test
```
## View The Architecture
[![Architecture diagram of mahadsaqib24-png/dsa-project-echo-engine-terminal-based-media-engine-.mp3-player-](https://gitdiagram.com/mahadsaqib24-png/dsa-project-echo-engine-terminal-based-media-engine-.mp3-player-/diagram.png)](https://gitdiagram.com/mahadsaqib24-png/dsa-project-echo-engine-terminal-based-media-engine-.mp3-player-?utm_source=readme&utm_medium=picture)
---

## 👥 Meet the Team

| 🧑‍💻 Name | 🪪 Roll No. |
|------|----------|
| **Mahad Saqib**    | 25K-0509 |
| **Arsalan Ahmed**  | 25K-0754 |
| **M. Yasir Uddin** | 25K-1007 |

🎓 *Data Structures Final Project, FAST-NUCES Karachi*

---

<div align="center">

### Made with luv and passion for Music and DSA 🎵

⭐ *Enjoyed Echo Engine? Give the repo a star!* ⭐

</div>
