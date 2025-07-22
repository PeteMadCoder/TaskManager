cmake_minimum_required(VERSION 3.16)
project(TaskManager)

# Find Qt packages
find_package(Qt5 REQUIRED COMPONENTS Core Sql Widgets)

# Enable automatic moc, uic, and rcc
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Include headers directory
include_directories(${CMAKE_SOURCE_DIR}/include)

# Add resource file
set(RESOURCE_FILES
    resources.qrc
)

# Add executable
add_executable(TaskManager
    src/main.cpp
    src/database.cpp
    src/mainwindow.cpp
    include/mainwindow.h
    ${RESOURCE_FILES}
)

# Link Qt modules
target_link_libraries(TaskManager
    Qt5::Core
    Qt5::Sql
    Qt5::Widgets
)

install(CODE "
    file(MAKE_DIRECTORY \"\$ENV{HOME}/.local/share/TaskManager\")
")

# Include directories
target_include_directories(TaskManager PRIVATE include)

install(TARGETS TaskManager RUNTIME DESTINATION bin)