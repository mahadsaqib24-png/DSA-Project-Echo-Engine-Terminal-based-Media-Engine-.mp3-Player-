<div align="center">

# 🎵 Echo Engine

**A console-based music player with playlists, a play queue, and listening history, built in C++.**

![Platform](https://img.shields.io/badge/platform-Windows%20%7C%20Linux-blue)
![Language](https://img.shields.io/badge/language-C%2B%2B-00599C)
![Audio](https://img.shields.io/badge/audio-MP3-orange)

</div>

---

## ✨ Features

- 🎧 **Automatic song import**: drop `.mp3` files into the `music/` folder and they are imported on startup
- 👤 **User accounts**: log in or create your own account
- 📂 **Playlists**: up to **10 playlists per user**
- ⏭️ **Up Next queue**: watch the queue animation in action
- 🕘 **Listening history**: view your history with a stack animation
- ⏱️ **Accurate song lengths**: read directly from the files and re-checked at every launch
- 🔤 **Accent support**: song names with accents display correctly
- 🔄 **Live rescan**: add new songs while the program is running
- ❓ **Built-in help**: every Master Dashboard option explained

---

## 🚀 Quick Start

### 1. Prepare the folder

Put **all** project files together in **one** folder, for example `C:\EchoEngine` or `~/EchoEngine`.

Then create a folder named `music` next to them and copy your `.mp3` files into it:

```
EchoEngine/
├── build.bat
├── EchoEnginePrototype.dev
├── Makefile
├── ... (source files)
├── data/
└── music/
    ├── song1.mp3
    └── song2.mp3
```

### 2. Build and run

<details open>
<summary><b>🪟 Windows</b></summary>

<br>

**Option A: build script**

1. Install **Dev-C++ / MinGW** (required).
2. Double-click **`build.bat`**. It creates `echo.exe`.
3. Double-click **`echo.exe`** to start.

**Option B: Dev-C++ IDE**

1. Open `EchoEnginePrototype.dev`.
2. Go to **Project → Project Options → Parameters → Linker** and add:
   ```
   -lwinmm -static
   ```
3. Press **F11** to compile and run.

> 💡 **Tip:** Use **Windows Terminal** and make the window at least **80 columns wide** (wider looks nicer).

</details>

<details open>
<summary><b>🐧 Linux</b></summary>

<br>

```bash
# 1. Install a music player (once)
sudo apt install mpv

# 2. Build
make

# 3. Run
./echo
```

</details>

### 3. Log in

Use the demo account:

| Username | Password |
|----------|----------|
| `user1`  | `1234`   |

Or choose **"Create an account"** to make your own.

---

## 🧭 Using the Program

| Menu | What it does |
|:----:|--------------|
| **4**  | Manage your playlists (up to 10 per user) |
| **5**  | **Up Next**: shows the queue animation |
| **6**  | **Listening history**: shows the stack animation |
| **10** | Rescan the `music/` folder while the program is running |
| **11** | **Help**: explains every option of the Master Dashboard |

> Songs in `music/` are imported automatically when the program starts.

---

## 🛠️ Troubleshooting

<details>
<summary><b>Song names with accents look broken (e.g. "Duhé")</b></summary>

<br>

This happens if you ran an older version of the program. Delete the file:

```
data/songs.txt
```

Then start the program again. It will re-import everything correctly.

</details>

<details>
<summary><b>Song lengths look wrong (e.g. doubled)</b></summary>

<br>

Nothing to do. Lengths are read from the files themselves and verified on every start, so incorrect values saved by older versions are **corrected automatically**.

</details>

---

## 📋 Requirements

| Platform | Needs |
|----------|-------|
| **Windows** | Dev-C++ / MinGW (links against `winmm`) |
| **Linux**   | `mpv`, plus a C++ compiler and `make` |

---

<div align="center">

Made with ❤️ and a love for music.

</div>
