cmake_minimum_required(VERSION 3.16)

# Name your project
project(ArabicMorphology VERSION 1.0 LANGUAGES CXX)

# Require C++17 standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# -------------------------------------------------------------------
# Qt6 Configuration
# AUTOMOC is critical: it processes the Q_OBJECT macro in mainwindow.h
# -------------------------------------------------------------------
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Find the Qt6 Widgets library
find_package(Qt6 COMPONENTS Widgets REQUIRED)

# -------------------------------------------------------------------
# Project Sources
# (Because you used header-only classes for your core logic,
# main.cpp is your only compiled source file!)
# -------------------------------------------------------------------
set(PROJECT_SOURCES
    main.cpp
    mainwindow.cpp
    mainwindow.h
    morphologyTab.h
    rootsTab.h
    schemesTab.h
    corpusTab.h
    AVLTree.h
    schemeHashTable.h
    morphologyEngine.h
    corpusAnalyzer.h
)

# Create the executable
add_executable(ArabicMorphology ${PROJECT_SOURCES})

# Link the executable to Qt6
target_link_libraries(ArabicMorphology PRIVATE Qt6::Widgets)
