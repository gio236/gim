# GIM Text Editor

**GIM Text Editor** is a terminal-based text editor built in C++ using the `ncurses` library and a custom `ncurses-toolkit` for UI components like menus and message boxes.

## Prerequisites

To build and run GIM, you need:

1. **ncurses library** – provides terminal handling and UI capabilities. 
2. **ncurses-toolkit** – this will be installed with the script.

### Installing `ncurses`

Depending on your system, install `ncurses` using your package manager:

| Platform / Package Manager | Command |
|----------------------------|---------|
| Debian / Ubuntu            | `sudo apt-get install libncurses5-dev libncursesw5-dev` |
| Fedora / Red Hat / CentOS  | `sudo dnf install ncurses-devel` |
| Arch Linux / Manjaro       | `sudo pacman -S ncurses` |
| macOS (Homebrew)           | `brew install ncurses` |
| Windows (MSYS2 / MinGW)    | `pacman -S mingw-w64-x86_64-ncurses` |

### Building gim

for installing : 

```bash
bash installer.sh
```

then you can use it :

```bash
gim filethatyouwantopen 
```

### this is how the folder where gim should be built should be

```bash 
.
├── a.out
├── main.cpp
├── makefile
└── ncurses-toolkit
    ├── demo
    ├── include
    │   ├── menu.hpp
    │   └── message_box.hpp
    ├── LICENSE
    ├── Makefile
    ├── menu_demo
    ├── misc
    │   ├── demo.cpp
    │   ├── mbox_demo.cpp
    │   └── menu_demo.cpp
    ├── README.md
    └── src
        ├── menu.cpp
        └── message_box.cpp

```
