#!/bin/bash

# Build first
cd build
cmake -DCMAKE_INSTALL_PREFIX=/usr/local ..
cmake --build . --parallel

# Install binary
sudo cmake --install . --prefix /usr/local

# Install icon
sudo install -D -m 644 ../resources/app_icon.png /usr/share/icons/hicolor/256x256/apps/TaskManager.png

# Create desktop file
cat <<EOF | sudo tee /usr/share/applications/TaskManager.desktop
[Desktop Entry]
Name=Task Manager
Comment=Manage your tasks and daily plans
Exec=/usr/local/bin/TaskManager
Icon=TaskManager
Terminal=false
Type=Application
Categories=Utility;
StartupWMClass=TaskManager
EOF

# Update databases
sudo update-desktop-database /usr/share/applications
sudo gtk-update-icon-cache /usr/share/icons/hicolor

# Create symlink to database in home directory
DB_PATH="$HOME/.local/share/TaskManager/taskmanager.db"
if [ ! -L "$HOME/taskmanager.db" ]; then
    ln -s "$DB_PATH" "$HOME/taskmanager.db"
fi