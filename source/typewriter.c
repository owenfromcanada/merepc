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
#include <time.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include "applications.h"


#define BLINKTIME           (CLOCKS_PER_SEC/2)
#define CHARATCURSOR(data)  (data->buffer[(data->cursor_r * data->cols) + data->cursor_c])
#define NUM_COLORS          (sizeof(COLORS)/sizeof(COLORS[0]))


typedef struct {
    GC gc;
    int padding;
    int rows;
    int cols;
    int offs_x;
    int offs_y;
    int cursor_c;
    int cursor_r;
    int cursor_state;
    int color_idx;
    clock_t clock;
    char * buffer;
} AppDataType;


static void Init(AppType * app);
static void Destroy(AppType * app);
static void Tick(AppType * app);
static void EventHandler(AppType * app, XEvent * event);
static void Clear(AppType * app);
static void CursorToEol(AppType * app);
static void Draw(AppType * app, int c, int r);
static void DrawBorder(AppType * data);


static const unsigned long COLORS[] = {
    FG_COLOR,
    0xff70d6ffUL,
    0xffff70a6UL,
    0xffff9770UL,
    0xfff4e285UL,
};


const AppStaticType Typewriter = {
    Init,
    Destroy,
    Tick,
    EventHandler
};


static void Init(AppType * app) {
    app->data = malloc(sizeof(AppDataType));
    AppDataType * data = (AppDataType *)app->data;

    data->gc = XCreateGC(app->display, app->window, 0, 0);
    XSetFont(app->display, data->gc, app->bigfont.id);

    data->padding = app->height/20;
    data->rows = ((app->height - data->padding*2) / app->bigfont.height) - 1;
    data->offs_y = ((app->height - (data->rows * app->bigfont.height)) / 2);
    data->cols = ((app->width - data->padding*2) / app->bigfont.width) - 2;
    data->offs_x = ((app->width - (data->cols * app->bigfont.width)) / 2) - app->bigfont.baseline_x;

    data->buffer = (char *)malloc(data->rows * data->cols);
    
    Clear(app);
    XFlush(app->display);
}

static void Destroy(AppType * app) {
    AppDataType * data = (AppDataType *)app->data;

    free(data->buffer);
    XFreeGC(app->display, data->gc);
    free(app->data);
    app->data = NULL;
}

static void Tick(AppType * app) {
    AppDataType * data = (AppDataType *)app->data;

    if (clock() - data->clock >= BLINKTIME) {
        data->clock += BLINKTIME;
        data->cursor_state ^= 1;
        Draw(app, data->cursor_c, data->cursor_r);
        XFlush(app->display);
    }
}

void EventHandler(AppType * app, XEvent * event) {
    AppDataType * data = (AppDataType *)app->data;

    if (event->type == KeyPress) {
        int r = data->cursor_r;
        int c = data->cursor_c;

        KeySym k = XLookupKeysym((XKeyPressedEvent *)event, 0);

        if (k >= 0x20 && k <= 0x7E) {
            if (k >= 0x61 && k <= 0x7A) {
                k -= 0x20;
            }

            CHARATCURSOR(data) = (char)k;

            if (c < data->cols - 1) {
                data->cursor_c++;
            }
            else if (r < data->rows - 1) {
                data->cursor_c = 0;
                data->cursor_r++;
            }
        }
        else if (k == XK_Return) {
            if (r < data->rows - 1) {
                data->cursor_r++;
                data->cursor_c = 0;
            }
        }
        else if (k == XK_BackSpace) {
            if (c > 0) {
                data->cursor_c--;
            }
            else if (r > 0) {
                data->cursor_r--;
                CursorToEol(app);
            }

            CHARATCURSOR(data) = ' ';
        }
        else if (k == XK_Home || k == XK_Begin) {
            data->cursor_c = 0;
        }
        else if (k == XK_End) {
            CursorToEol(app);
        }
        else if (k == XK_Page_Up) {
            data->cursor_r = 0;
        }
        else if (k == XK_Page_Down) {
            data->cursor_r = data->rows - 1;
        }
        else if (k == XK_Left) {
            if (c > 0) {
                data->cursor_c--;
            }
        }
        else if (k == XK_Right) {
            if (c < data->cols - 1) {
                data->cursor_c++;
            }
        }
        else if (k == XK_Up) {
            if (r > 0) {
                data->cursor_r--;
            }
        }
        else if (k == XK_Down) {
            if (r < data->rows - 1) {
                data->cursor_r++;
            }
        }
        else if (k == XK_Delete) {
            Clear(app);
        }
        else if (k == XK_Tab) {
            data->color_idx = (data->color_idx + 1) % NUM_COLORS;
            XSetForeground(app->display, data->gc, COLORS[data->color_idx]);
            DrawBorder(app);
        }

        data->cursor_state = 1;
        data->clock = clock();

        Draw(app, c, r);
        if (data->cursor_c != c || data->cursor_r != r) {
            Draw(app, data->cursor_c, data->cursor_r);
        }

        XFlush(app->display);
    }
}

static void Clear(AppType * app) {
    AppDataType * data = (AppDataType *)app->data;

    memset(data->buffer, ' ', data->rows * data->cols);
    XClearArea(app->display, app->window, 0, 0, app->width, app->height, False);

    XSetForeground(app->display, data->gc, COMMENT_COLOR);
    XSetFont(app->display, data->gc, app->smallfont.id);
    char hints[] = "ESC: Menu        TAB: Color        DEL: Clear";
    XDrawString(app->display, app->window, data->gc, app->smallfont.width, app->height - app->smallfont.height + app->smallfont.baseline_y, hints, strlen(hints));
    XSetFont(app->display, data->gc, app->bigfont.id);

    data->color_idx = 0;
    XSetForeground(app->display, data->gc, COLORS[data->color_idx]);
    DrawBorder(app);

    data->cursor_c = 0;
    data->cursor_r = 0;
    data->cursor_state = 1;
    data->clock = clock();
}

static void CursorToEol(AppType * app) {
    AppDataType * data = (AppDataType *)app->data;

    data->cursor_c = data->cols - 1;
    while (CHARATCURSOR(data) == ' ' && data->cursor_c > 0) {
        data->cursor_c--;
    }
    if (CHARATCURSOR(data) != ' ' && data->cursor_c < data->cols - 1) {
        data->cursor_c++;
    }
}

static void Draw(AppType * app, int c, int r) {
    AppDataType * data = (AppDataType *)app->data;

    int x = data->offs_x + (app->bigfont.width * c);
    int y = data->offs_y + (app->bigfont.height * r);
    XClearArea(app->display, app->window, x, y, app->bigfont.width, app->bigfont.height, False);
    XDrawString(app->display, app->window, data->gc, x + app->bigfont.baseline_x, y + app->bigfont.baseline_y, &(data->buffer[(r*data->cols) + c]), 1);
    if (data->cursor_c == c && data->cursor_r == r && data->cursor_state != 0) {
        XFillRectangle(app->display, app->window, data->gc, x, y + app->bigfont.baseline_y - (app->bigfont.height/4), app->bigfont.width, app->bigfont.height*9/32);
    }
}

static void DrawBorder(AppType * app) {
    AppDataType * data = (AppDataType *)app->data;

    int t = app->bigfont.width/4;

    XFillRectangle(app->display, app->window, data->gc, data->padding, data->padding, app->width - (data->padding*2), t);
    XFillRectangle(app->display, app->window, data->gc, data->padding, app->height - data->padding - t, app->width - (data->padding*2), t);
    XFillRectangle(app->display, app->window, data->gc, data->padding, data->padding, t, app->height - (data->padding*2));
    XFillRectangle(app->display, app->window, data->gc, app->width - data->padding - t, data->padding, t, app->height - (data->padding*2));
}
