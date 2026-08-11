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

#define MIN(a, b)           ((a) < (b) ? (a) : (b))

typedef struct {
    GC gc;
    int rows;
    int cols;
    int width;
    int height;
    int margin;
    int padding;
    int offs_x;
    int offs_y;
} AppDataType;

static void Init(AppType * app);
static void Destroy(AppType * app);
static void Tick(AppType * app);
static void EventHandler(AppType * app, XEvent * event);
static void DrawTypewriterIcon(AppType * app, int x, int y, int w);
static void DrawPongIcon(AppType * app, int x, int y, int w);
static void DrawLabel(AppType * app, int x, int y, char * text);

const AppStaticType Menu = {
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

    data->margin = app->height / 10;
    data->padding = app->height / 20;
    data->rows = 1;
    data->cols = 2;

    int outer_w = (app->width - ((data->cols - 1) * data->margin)) / (data->cols + 1);
    int outer_h = (app->height - ((data->rows - 1) * data->margin)) / (data->rows + 1);
    int outer_size = MIN(outer_w, outer_h);
    data->width = outer_size;
    data->height = outer_size;
    data->offs_x = (app->width - (data->width * data->cols) - (data->margin * (data->cols - 1)))/2;
    data->offs_y = (app->height - (data->height * data->rows) - (data->margin * (data->rows - 1)))/2;

    int icon_w = data->width - (data->padding*2);
    int icon_h = data->height - (data->padding*2) - app->bigfont.height;
    int icon_size = MIN(icon_w, icon_h * 20/14);
    icon_w = icon_size;
    icon_h = icon_size * 14 / 20;
    int icon_offs_x = (outer_size - icon_w)/2;
    int icon_offs_y = outer_size - (data->padding*2) - app->bigfont.height - icon_h;
    int label_x = outer_size/2;
    int label_y = outer_size - data->padding - app->bigfont.height + app->bigfont.baseline_y;

    DrawTypewriterIcon(app, data->offs_x + icon_offs_x, data->offs_y + icon_offs_y, icon_size);
    DrawLabel(app, data->offs_x + label_x, data->offs_y + label_y, "T");
    DrawPongIcon(app, data->offs_x + data->width + icon_offs_x + data->margin, data->offs_y + icon_offs_y, icon_size);
    DrawLabel(app, data->offs_x + data->width + data->margin + label_x, data->offs_y + label_y, "P");

    XSetForeground(app->display, data->gc, COMMENT_COLOR);
    XSetFont(app->display, data->gc, app->smallfont.id);
    char hints[] = "CTRL+SHIFT+F12: Quit";
    XDrawString(app->display, app->window, data->gc, app->smallfont.width, app->height - app->smallfont.height + app->smallfont.baseline_y, hints, strlen(hints));
    XSetForeground(app->display, data->gc, FG_COLOR);
}

static void Destroy(AppType * app) {
    AppDataType * data = (AppDataType *)app->data;

    XFreeGC(app->display, data->gc);
    free(app->data);
    app->data = NULL;
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
static void Tick(AppType * app) {
    // do nothing
}
#pragma GCC diagnostic pop

static void EventHandler(AppType * app, XEvent * event) {
    if (event->type == KeyPress) {
        KeySym k = XLookupKeysym((XKeyPressedEvent *)event, 0);

        if (k == 't') {
            SwitchApp(app, &Typewriter);
        }
        else if (k == 'p') {
            SwitchApp(app, &Pong);
        }
    }
}

static void DrawTypewriterIcon(AppType * app, int x, int y, int w) {
    // creates a rectangular icon (20x14 "pixels")
    // x and y specify the upper left corner
    AppDataType * data = (AppDataType *)app->data;

    int px = w/20; // "pixel" size for icon drawing
    int offs_x = x + w - (px * 20);
    int offs_y = y + w - (px * 14);

    XFillRectangle(app->display, app->window, data->gc, offs_x, offs_y, 20*px, 14*px);
    XSetForeground(app->display, data->gc, BG_COLOR);
    XFillRectangle(app->display, app->window, data->gc, offs_x + (2*px), offs_y + (2*px), 16*px, 10*px);
    XSetForeground(app->display, data->gc, FG_COLOR);
    for (int x1 = 0; x1 < 5; x1++) {
        for (int y1 = 0; y1 < 2; y1++) {
            XFillRectangle(app->display, app->window, data->gc, offs_x + (3*px) + (x1 * 3*px), offs_y + (3*px) + (y1 * 3*px), 2*px, 2*px);
        }
    }
    XFillRectangle(app->display, app->window, data->gc, offs_x + (6*px), offs_y + (9*px), 8*px, 2*px);
}

static void DrawPongIcon(AppType * app, int x, int y, int w) {
    // creates a rectangular icon (20x14 "pixels")
    // x and y specify the upper left corner
    AppDataType * data = (AppDataType *)app->data;

    int px = w/20; // "pixel" size for icon drawing
    int offs_x = x + w - (px * 20);
    int offs_y = y + w - (px * 14);

    XFillRectangle(app->display, app->window, data->gc, offs_x, offs_y, 20*px, px);
    XFillRectangle(app->display, app->window, data->gc, offs_x, offs_y + (13*px), 20*px, px);

    XFillRectangle(app->display, app->window, data->gc, offs_x, offs_y + (4*px), 2*px, 6*px);
    XFillRectangle(app->display, app->window, data->gc, offs_x + (18*px), offs_y + (5*px), 2*px, 6*px);

    XFillRectangle(app->display, app->window, data->gc, offs_x + (5*px), offs_y + (3*px), 2*px, 2*px);
}

static void DrawLabel(AppType * app, int x, int y, char * text) {
    AppDataType * data = (AppDataType *)app->data;

    XDrawString(app->display, app->window, data->gc, x - (strlen(text)*app->bigfont.width)/2 + app->bigfont.baseline_x, y + app->bigfont.baseline_y, text, strlen(text));
}
