
Debian
====================
This directory contains files used to package keplerd/kepler-qt
for Debian-based Linux systems. If you compile keplerd/kepler-qt yourself, there are some useful files here.

## kepler: URI support ##


kepler-qt.desktop  (Gnome / Open Desktop)
To install:

	sudo desktop-file-install kepler-qt.desktop
	sudo update-desktop-database

If you build yourself, you will either need to modify the paths in
the .desktop file or copy or symlink your kepler-qt binary to `/usr/bin`
and the `../../share/pixmaps/kepler128.png` to `/usr/share/pixmaps`

kepler-qt.protocol (KDE)

