# Task Manager

A custom task manager built from scratch to fit my workflow and help me stay organized (and sane) when life gets busy.
Built with **C++** and **Qt**.

![GIF](/resources/pic.png)

---

## Project organization

The repository is organized in the following way:

1. **build/** – CMake build output directory (generated files, compiled objects, binaries).
2. **include/** – Header files defining the classes (e.g., task, database, main window, templates).
3. **src/** – Implementation source code of the classes and application logic.
4. **resources/** – Application icons and images.
5. **resources.qrc** – Qt resource collection file.
6. **CMakeLists.txt** – Build configuration for CMake.
7. **install.sh** – Installation script for Linux (optional).
8. **TaskManager.desktop** – Desktop entry file for Linux.
9. **README.md** – This file.

---

## Build instructions

Inside the folder **/build**, execute the following commands:

```bash
cmake ..
cmake --build .
./TaskManager
```

To remove the auto-generated build items (clean build):

```bash
rm -rf *
```

Also, if you're on Linux and want to install the Task Manager, run the installation script (install.sh):
```bash
./install.sh
```

---

## Features

* Simple, clean task management interface
* Customizable task templates
* Calendar-based task visualization
* Persistent storage using an internal database

---

## Screenshots

*(Add screenshots here later)*

---

## Potencial future features

* [ ] Add recurring tasks
* [ ] Add notification support
* [ ] Add task export/import (JSON/CSV)
* [ ] Polish UI/UX

---

