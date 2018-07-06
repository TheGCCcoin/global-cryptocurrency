
Debian
====================
This directory contains files used to package TheGCCcoind/TheGCCcoin-qt
for Debian-based Linux systems. If you compile TheGCCcoind/TheGCCcoin-qt yourself, there are some useful files here.

## TheGCCcoin: URI support ##


TheGCCcoin-qt.desktop  (Gnome / Open Desktop)
To install:

	sudo desktop-file-install TheGCCcoin-qt.desktop
	sudo update-desktop-database

If you build yourself, you will either need to modify the paths in
the .desktop file or copy or symlink your TheGCCcoin-qt binary to `/usr/bin`
and the `../../share/pixmaps/TheGCCcoin128.png` to `/usr/share/pixmaps`

TheGCCcoin-qt.protocol (KDE)

