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

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include "applications.h"

typedef struct {
    GC gc;
} AppDataType;

static void Init(AppType * app);
static void Destroy(AppType * app);
static void Tick(AppType * app);
static void EventHandler(AppType * app, XEvent * event);

const AppStaticType Jump = {
    Init,
    Destroy,
    Tick,
    EventHandler
};

static void Init(AppType * app) {
    app->data = malloc(sizeof(AppDataType));
    AppDataType * data = (AppDataType *)app->data;
    
    data->gc = XCreateGC(app->display, app->window, 0, 0);
    XSetForeground(app->display, data->gc, FG_COLOR);
    XSetFont(app->display, data->gc, app->bigfont.id);
}

static void Destroy(AppType * app) {
    AppDataType * data = (AppDataType *)app->data;

    XFreeGC(app->display, data->gc);
    free(app->data);
    app->data = NULL;
}

static void Tick(AppType * app) {
    
}

static void EventHandler(AppType * app, XEvent * event) {
    
}
