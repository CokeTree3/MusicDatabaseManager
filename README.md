# Music Database Manager

**Note! -** This project is still in very early development, so many bugs are to be expected and much of the project might change and be redeveloped. 

A simple Qt based program to synchronize a local music library with a server, with multiplatform support. 

With music streaming services becoming more and more expensive(While paying the artists barely anything), I stopped using them and switched to just downloading the music myself. While much more reliable and cheaper(in the long term), this created the issue of having to manually copy everything I download to all my devices, and keeping every device's local files up to date when I download new tracks on one device.

So I decided to create my own music synchronization system to more or less automate this task. While there definitely are many similar programs out there, I wanted to make my own to suit all my needs and to practise full program development, as this is my first proper programming project.

## Project

The main goal of this project is to allow music library synchronization between a server library and client libraries independent of the platform, as well as a simple GUI display of the current library. The aim is to keep all functionality on all the main operating systems.

## Supported Platforms

Currently supported platforms are:
- Windows
- Linux
- Android

Primary focus is on Linux support, but Windows will also get fair amount of testing and support
Android support is also guaranteed for core functionality for now. In the long term I would like to move the android development to Kotlin as it's a much more integrated development platform for the Android OS, but for now I try to keep the main app functions working with Qt support for Android NDK. 

## Installation

The binary can be run as is, no installation required(on Linux/Windows)

If available, download the compiled binaries from the latest release [here](https://github.com/CokeTree3/MusicDatabaseManager/releases)
If not available, proceed to the **Building** section.

## Requirements

On Linux, running the GUI requires Qt5 to be installed.
Windows executables downloaded from the Releases section has all the necessary libraries built in and doesn't require any other installs.


## Usage

This program allows synchronizing music files between two systems over the network, by connecting from one system to a host system, and choosing which files to download.

The program operates with a music library in which the root directory contains subdirectories for each artist and in each of them, there are directories for each album/single/any other music release type and in these there are the actual music tracks. This library system will be automatically created when initialised and synchronized with an empty local library, but can also already exist and only the missing tracks will be added. 

Note that the synchronization will also remove any extra tracks/albums/artists that are present in the local library when synchronized if they are not present in the server library, but this can be manually disabled.

During synchronization a selection window allows to deselect specific tracks, albums and artists which to not synchronize with the server. 

If already exists locally, the library needs to follow a structure of *root* - *Artist Name* - *Album Name* - *Tracks*, where the Artists and Albums are directories. The Album directory can optionally contain a *jpg* or *png* file that will be loaded as the album cover. If not provided, the cover image will be taken from the embedded cover of the first track in the album, and if this is not available, the default image will be used(from /assets directory)


## Building

Currently the project uses the QMake building system from Qt, but I might switch over to CMake soon.

At least C++ version C++17 is required, as well as g++ for Linux and MingW for Windows. 

Building the binaries requires these libraries to be installed:
- Qt(5+)
- [JSON for Modern C++](https://github.com/nlohmann/json)
- [asio C++ library](https://think-async.com/Asio/AsioStandalone.html)

Building for android can be done using Android NDK

When targeting platforms other than Linux, the .pro QMake file requires uncommenting some lines for the specific platform.




The app has both client and server modes built in. The default is the client mode, but with a switch in the *File* menu, the server mode can be enabled and the local library synced to any connecting device.


## Future Plans

Since the project is sill in very early development and for now is very simple and basic, much of the current functionality might change and many of the features mentioned below might also change.

Some features I plan to add:

* Improved UI
* A CLI server binary, with no Qt dependency
* Local track importing(managing artist and album directories)
* Android Studio/Kotlin based Android app
* Server funcionality on Android
* Playlist file syncronization
* \+ more 

## Screenshots

To be added!