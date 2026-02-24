# Arabic Morphology Engine (Root-Pattern System) 🕌

An advanced algorithmic tool for Arabic Natural Language Processing (NLP). This project implements the **Root-Pattern (Racine-Schème)** morphological model using high-performance data structures including **AVL Trees** for root indexing and **Hash Tables** for pattern management.

Designed for the **Algorithmic Project (1ING GLSI)**.

## ✨ Features

* **Morphological Generation:** Automatically generates derived words by combining a trilateral root (e.g., *K-T-B*) with a specific pattern (e.g., *maF3ouL*).
* **Reverse Extraction:** Decomposes a given Arabic word to identify its root and morphological pattern.
* **Corpus Analysis:** Processes large text files to extract roots, calculate word frequencies, and discover new potential roots using a "Strict" vs "Learning" mode.
* **Interactive Visualization:**
* Dynamic **AVL Tree** visualization of roots with Zoom/Pan capabilities.
* Tabular view of morphological families.

* **Multilingual UI:** Full interface support for English, French, and Arabic.

## 🛠 Prerequisites

Before building, ensure you have the following installed on your system:

1. **C++ Compiler:** Supporting **C++17** or higher (GCC, Clang, or MSVC).
2. **CMake:** Version **3.16** or later.
3. **Qt6 Framework:** specifically the **Qt Widgets** module.

---

## 📦 Installation & Compilation

### 🐧 Linux (Ubuntu/Debian)

#### 1. Install Dependencies

You need the build tools, CMake, and Qt6 libraries.

```bash
sudo apt update
sudo apt install build-essential cmake
sudo apt install qt6-base-dev qt6-declarative-dev libqt6widgets6 libgl1-mesa-dev

```

#### 2. Build the Project

You can build the project using the provided `Makefile` from the root directory:

```bash
make
```

Alternatively, you can build it manually using CMake:

```bash
mkdir build
cd build
cmake ../src
make
```

#### 3. Run the Application

You can run the application using the `Makefile`:

```bash
make run
```

Or manually from the build directory:

```bash
cd build
./ArabicMorphology
```

> **Note:** The `racines.txt` and `schemes.txt` files are automatically copied to the `build` directory during compilation.

#### 4. Visualizer (Optional)

If you want to run the project with the AVL Tree visualizer:

1. Install the visualizer dependencies:

   ```bash
   make install-vis
   ```

2. Build the project with visualizer support:

   ```bash
   make visualizer
   ```

3. Start the visualizer server:

   ```bash
   make start-server &
   ```

4. Open your browser at `http://localhost:3000`
5. Run the application:

   ```bash
   make run
   ```

---

### 🪟 Windows

#### Option A: Using Qt Creator (Recommended)

1. Open **Qt Creator**.
2. Go to **File > Open File or Project** and select `src/CMakeLists.txt`.
3. Configure the project using your installed Kit (e.g., MinGW or MSVC).
4. Click the **Run** button (Green Arrow).

#### Option B: Command Line (PowerShell / CMD)

You must use the **Qt command prompt** (e.g., *"Qt 6.x.x (MinGW/MSVC) Command Prompt"*) to ensure environment variables are set correctly.

1. Navigate to the project folder.
2. Run the build commands:

```powershell
mkdir build
cd build
cmake ../src
cmake --build . --config Release

```

1. **Fixing Missing DLL Errors:**
If you try to run the `.exe` directly from the folder and get "Missing Qt6Core.dll" errors, run the deployment tool:

```powershell
windeployqt.exe Release\ArabicMorphology.exe

```

1. Run the executable located in the `Release` folder.

---

## 📂 Complete Project Structure

### 🧠 Core Logic & Data Structures

| File | Description |
| --- | --- |
| **`AVLTree.h`** | Generic self-balancing Binary Search Tree implementation for storing Roots. Includes node rotation logic. |
| **`schemeHashTable.h`** | Hash Table implementation with triple-indexing (Name, Pattern, Length) for fast Scheme retrieval. |
| **`morphologyEngine.h`** | Core static class handling the logic for *Generation* (Root+Pattern -> Word) and *Extraction* (Word -> Root). |
| **`corpusAnalyzer.h`** | Logic for parsing text files, counting word frequencies, and auto-learning new roots. |
| **`StringUtils.h`** | Utilities for handling UTF-8 Arabic characters (resolves the 2-byte char issue in C++). |

### 🖥️ User Interface (Qt6)

| File | Description |
| --- | --- |
| **`main.cpp`** | Application entry point. Initializes Qt and loads initial data. |
| **`mainwindow.h` / `.cpp**` | Main container for the GUI. Handles the menu bar, language switching, and the tab widget. |
| **`morphologyTab.h`** | UI tab for generating words and reverse-extracting roots. |
| **`rootsTab.h`** | UI tab for viewing/editing roots. Contains the `ZoomableView` logic to render the AVL Tree graphically. |
| **`schemesTab.h`** | UI tab for managing and viewing the hash table of morphological patterns. |
| **`corpusTab.h`** | UI tab for loading text files, running the analyzer, and displaying statistics/new roots. |

### ⚙️ Configuration

| File | Description |
| --- | --- |
| **`CMakeLists.txt`** | Build configuration file. Sets C++17, links Qt6 Widgets, and enables `AUTOMOC` for Qt meta-object compilation. |

---

## 🧩 Algorithms Overview

### 1. Root Storage (AVL Tree)

* **Why:** Arabic roots are numerous. We need  search time to keep the interface responsive.
* **How:** The tree auto-balances using rotations (Left/Right) whenever a root is inserted.

### 2. Pattern Matching (Hash Table)

* **Why:** Patterns need to be accessed instantly  during generation.
* **Optimization:** We maintain a `lengthTable` array. When analyzing a word of length 6, we only check patterns that produce 6-letter words, significantly reducing processing time.

### 3. Generation Logic

The engine replaces numerical placeholders in patterns with root letters:

* Pattern: `m a 1 2 u 3` (maF3ouL)
* Root: `k t b`
* Result: `m a k t u b` (maktoub)
* *Includes handling for Shadda (gemination).*

## ⚠️ Troubleshooting

**1. "Could not find a package configuration file provided by 'Qt6'..."**

* **Solution:** Make sure you have installed `qt6-base-dev` (Linux) or that your `CMAKE_PREFIX_PATH` includes the path to your Qt installation (Windows).

**2. "GL/gl.h: No such file or directory" (Linux)**

* **Solution:** You are missing OpenGL development files. Run:

```bash
sudo apt install libgl1-mesa-dev

```

**3. Arabic text appears as question marks (???)**

* **Solution:** The application uses UTF-8. Ensure your source files (`racines.txt`) are saved with **UTF-8 encoding**.

---

**Developed for the 2025-2026 Academic Year.**
