<div align="center">

# 🎵🎶 Echo Engine 🎶🎵

### Your music. Your playlists. Your terminal. 🔥

**A console-based music player with playlists, a play queue, and listening history, built in C++.**

![Platform](https://img.shields.io/badge/platform-Windows%20%7C%20Linux-blue?style=for-the-badge)
![Language](https://img.shields.io/badge/language-C%2B%2B-00599C?style=for-the-badge)
![Audio](https://img.shields.io/badge/audio-MP3-orange?style=for-the-badge)
![DSA](https://img.shields.io/badge/powered%20by-DSA-ff69b4?style=for-the-badge)

🎧 &nbsp;**Drop your songs → Hit play → Watch the data structures dance!**&nbsp; 💃

</div>

---

## ✨ What Can It Do?

| | Feature | Details |
|:-:|---------|---------|
| 🎧 | **Auto song import** | Drop `.mp3` files into `music/` and they're imported on startup, no fuss |
| 👤 | **User accounts** | Log in or create your very own account |
| 📂 | **Playlists** | Up to **10 playlists per user**, so go wild |
| ⏭️ | **Up Next queue** | Watch the queue come alive with a **queue animation** |
| 🕘 | **Listening history** | Travel back in time with a **stack animation** |
| ⏱️ | **Accurate song lengths** | Read straight from the files and re-checked at every launch |
| 🔤 | **Accent support** | Song names with accents show up perfectly |
| 🔄 | **Live rescan** | Add new songs while the program is running |
| ❓ | **Built-in help** | Every Master Dashboard option explained |

---

## 🚀 Quick Start

> ⏱️ Three steps and you're jamming!

### 📁 Step 1: Set up your folder

Put **all** project files together in **one** folder, for example `C:\EchoEngine` or `~/EchoEngine`.

Then create a folder named `music` right next to them and copy your `.mp3` files in 🎶

```
EchoEngine/
├── 🛠️  build.bat
├── 📄 EchoEnginePrototype.dev
├── 📄 Makefile
├── 📄 ... (source files)
├── 💾 data/
└── 🎵 music/
    ├── song1.mp3
    └── song2.mp3
```

### 🔨 Step 2: Build & run

<details open>
<summary><b>🪟 Windows</b></summary>

<br>

**⚡ Option A: Build script**

1. Install **Dev-C++ / MinGW** (required).
2. Double-click **`build.bat`** → it creates `echo.exe` ✅
3. Double-click **`echo.exe`** and enjoy 🎉

**🧰 Option B: Dev-C++ IDE**

1. Open `EchoEnginePrototype.dev`.
2. Go to **Project → Project Options → Parameters → Linker** and add:
   ```
   -lwinmm -static
   ```
3. Press **F11** to compile and run 🚀

> 💡 **Pro tip:** Use **Windows Terminal** and make the window at least **80 columns wide** (wider looks even nicer ✨).

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
./echo
```

</details>

### 🔐 Step 3: Log in

Jump in with the demo account:

| 👤 Username | 🔑 Password |
|:-----------:|:-----------:|
| `user1`     | `1234`      |

Or choose **"Create an account"** to make it yours 😎

---

## 🧭 Your Menu Map

| Menu | What it does |
|:----:|--------------|
| **4**  | 📂 Manage your playlists (up to 10 per user) |
| **5**  | ⏭️ **Up Next**: shows the queue animation |
| **6**  | 🕘 **Listening history**: shows the stack animation |
| **10** | 🔄 Rescan the `music/` folder while the program is running |
| **11** | ❓ **Help**: explains every option of the Master Dashboard |

> 🎵 Songs in `music/` are imported automatically every time the program starts.

---

## 🛠️ Something Look Off?

<details>
<summary><b>😵 Song names with accents look broken (e.g. "Duhé")</b></summary>

<br>

That's from an older version of the program. Just delete this file:

```
data/songs.txt
```

Then start the program again. It will re-import everything correctly ✅

</details>

<details>
<summary><b>⏱️ Song lengths look wrong (e.g. doubled)</b></summary>

<br>

Relax, nothing to do! 😌 Lengths are read from the files themselves and verified on every start, so incorrect values saved by older versions are **corrected automatically**.

</details>

---

## 📋 Requirements

| Platform | You'll need |
|----------|-------------|
| 🪟 **Windows** | Dev-C++ / MinGW (links against `winmm`) |
| 🐧 **Linux**   | `mpv`, plus a C++ compiler and `make` |

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
