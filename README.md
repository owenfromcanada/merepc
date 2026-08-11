# MerePC

## About

MerePC is software for everyone, and particularly those who are not familiar with PCs.  It was created for users to practice PC fundamentals, such as using a keyboard and mouse.

I created MerePC for my kids.  They love to type and play simple games, but their curiosity ultimately leads them to menus and screens they don't understand how to navigate.  I wanted to create a system that is simple and impossible to break.

### But What *Is* It?

MerePC is a Linux program.  It can be run on any distribution, but it particularly shines when used as the sole program on dedicated hardware, such as a Raspberry Pi or similar low-cost computer.

### Design Priorities

* Accessible for users who have never interacted with a PC before
* Approachable, with simple controls and clear interfaces
* Robust against experimentation
    * The system should always be recoverable by the user, even after mashing the keyboard
* Lean enough to run on minimal or legacy hardware

## Requirements

* X11 or Xwayland
    * No desktop environment or window manager is required, a bare install of X11 is enough

## Build

Required tools:

* GCC
* make
* X11 development libraries

Run `make` from the root directory to build the software.

## Installation

Download the latest release for your PC architecture.

If your PC already has a desktop environment, simply run the MerePC binary file.

## Usage on Dedicated Hardware

While MerePC may be run on any Linux distribution, it is most robust when run from dedicated hardware.  A low-cost computer such as a Raspberry Pi is ideal for this usage, as MerePC requires very little computing power.  No desktop environment is required.

### Example Hardware Package

* Raspberry Pi Zero or similar single-board computer
* 4GB ROM storage
* 512MB RAM
* Keyboard
* Mouse
* Monitor

## Legal

MerePC is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

MerePC is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with MerePC. If not, see [https://www.gnu.org/licenses/](https://www.gnu.org/licenses/).