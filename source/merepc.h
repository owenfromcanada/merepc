/***********************************************************************************************************************
Copyright 2026 Owen Tosh.

This file is part of MerePC.

MerePC is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

MerePC is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with MerePC. If not, see
<https://www.gnu.org/licenses/>. 
***********************************************************************************************************************/

#ifndef MEREPC_H
#define MEREPC_H

#include <X11/Xlib.h>

#define FG_COLOR 0xFFEEEEEEUL
#define BG_COLOR 0xFF222222UL
#define COMMENT_COLOR 0xFF777777UL

typedef struct AppStaticStruct AppStaticType;

typedef struct {
    XFontStruct * font;
    int id;
    int width;
    int height;
    int baseline_x;
    int baseline_y;
} FontType;

typedef struct {
    const AppStaticType * appcalls;
    Display * display;
    Window window;
    FontType bigfont;
    FontType mediumfont;
    FontType smallfont;
    int width;
    int height;
    void * data; // custom data set by application
} AppType;

struct AppStaticStruct {
    void (* Init)(AppType *);
    void (* Destroy)(AppType *);
    void (* Tick)(AppType *);
    void (* EventHandler)(AppType *, XEvent *);
};

extern void SwitchApp(AppType * app, const AppStaticType * appcalls);

#endif
