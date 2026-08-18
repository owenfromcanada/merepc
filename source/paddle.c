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
#include "parameters.h"

#define FRAMES_PER_SEC      60
#define CLOCKS_PER_FRAME    (CLOCKS_PER_SEC/FRAMES_PER_SEC)
#define HITS_PER_ACCEL      7
#define MAX(a, b)           ((a) > (b) ? (a) : (b))
#define MIN(a, b)           ((a) < (b) ? (a) : (b))
#define NUM_PARAMETERS      1


typedef struct {
    GC gc;
    int field_x1;
    int field_x2;
    int field_y1;
    int field_y2;
    int msg_y;
    int pwidth;
    int pheight;
    int bsize;
    int bvel; // ball velocity in pixels per frame
    int bvelinit; // starting ball velocity in pixels per frame
    int baccel; // ball acceleration after a number of hits
    int paccel; // paddle acceleration in pixels per frame per frame
    int pvmax; // paddle max velocity in pixels per frame
    int px[2]; // paddle x position, static
    int py[2]; // current paddle y position
    int pvy[2]; // current paddle y velocity
    int pup[2]; // current paddle key up command
    int pdown[2]; // current paddle key down command
    int ball_x;
    int ball_y;
    int ball_vx;
    int ball_vy;
    int hits;
    int playing;
    clock_t clock;
    ParameterType parameters[NUM_PARAMETERS];
    int highscore;
    int parameters_dirty;
} AppDataType;

static void Init(AppType * app);
static void Destroy(AppType * app);
static void Tick(AppType * app);
static void EventHandler(AppType * app, XEvent * event);
static void DrawText(AppType * app, int x, int y, char * text);
static void ShowScore(AppType * app);
static void ShowHighScore(AppType * app);
static void ShowStartMessage(AppType * app);
static void SetField(AppType * app);
static void MovePaddle(AppType * app, int player, int pos);
static void MoveBall(AppType * app, int x, int y);

static const char PARAMETER_NAME[] = "Paddle";

const AppStaticType Paddle = {
    Init,
    Destroy,
    Tick,
    EventHandler
};

static void Init(AppType * app) {
    app->data = malloc(sizeof(AppDataType));
    AppDataType * data = (AppDataType *)app->data;

    data->parameters[0].key = "highscore";
    data->parameters[0].type = TYPE_INT;
    data->parameters[0].count = 1;
    data->parameters[0].data = &data->highscore;
    data->highscore = 0; // default value if no saved score exists
    LoadParameters(PARAMETER_NAME, NUM_PARAMETERS, data->parameters);

    data->gc = XCreateGC(app->display, app->window, 0, 0);
    XSetForeground(app->display, data->gc, FG_COLOR);
    XSetFont(app->display, data->gc, app->mediumfont.id);

    int padding = app->height / 10;
    int maxwidth = app->width - (padding*2);
    int maxheight = app->height - (padding*2);

    int fieldwidth, fieldheight;
    if (maxheight * 8 / 5 > maxwidth) {
        // more square screen ratio -- width is limiting factor
        fieldwidth = maxwidth;
        fieldheight = maxwidth * 5 / 8;
    }
    else {
        // widescreen ratio -- height is limiting factor
        fieldheight = maxheight;
        fieldwidth = maxheight * 8 / 5;
    }

    data->field_y1 = app->height/2 - fieldheight/2;
    data->field_y2 = app->height/2 + fieldheight/2;
    data->field_x1 = app->width/2 - fieldwidth/2;
    data->field_x2 = app->width/2 + fieldwidth/2;
    data->pheight = fieldheight / 4;
    data->pwidth = data->pheight / 5;
    data->bsize = fieldheight / 24;
    data->bvelinit = MAX(1, (fieldwidth/4)/FRAMES_PER_SEC);
    data->baccel = MAX(1, data->bvelinit/4);
    data->px[0] = data->field_x1 - data->pwidth;
    data->px[1] = data->field_x2;
    data->paccel = MAX(1, data->bvelinit/4);
    data->pvmax = MAX(1, data->bvelinit*5/3);
    data->msg_y = data->field_y1 - data->bsize - app->mediumfont.height/2;
    data->playing = 0;

    XFillRectangle(app->display, app->window, data->gc, data->field_x1 - data->bsize, data->field_y1 - data->bsize, data->field_x2 - data->field_x1 + (data->bsize*2), data->bsize);
    XFillRectangle(app->display, app->window, data->gc, data->field_x1 - data->bsize, data->field_y2, data->field_x2 - data->field_x1 + (data->bsize*2), data->bsize);
    ShowStartMessage(app);
    ShowHighScore(app);

    XSetForeground(app->display, data->gc, COMMENT_COLOR);
    XSetFont(app->display, data->gc, app->smallfont.id);
    char hints[] = "ESC: Menu";
    XDrawString(app->display, app->window, data->gc, app->smallfont.width, app->height - app->smallfont.height + app->smallfont.baseline_y, hints, strlen(hints));
    XSetForeground(app->display, data->gc, FG_COLOR);
    XSetFont(app->display, data->gc, app->mediumfont.id);

    XFlush(app->display);
}

static void Destroy(AppType * app) {
    AppDataType * data = (AppDataType *)app->data;

    if (data->parameters_dirty == 1) {
        SaveParameters(PARAMETER_NAME, NUM_PARAMETERS, data->parameters);
    }

    XFreeGC(app->display, data->gc);
    free(app->data);
    app->data = NULL;
}

static void Tick(AppType * app) {
    AppDataType * data = (AppDataType *)app->data;

    int bx, by, py;

    if (data->playing != 0) {
        if (clock() - data->clock >= CLOCKS_PER_FRAME) {
            data->clock += CLOCKS_PER_FRAME;

            // move paddles
            for (int p = 0; p < 2; p++) {

                int dv = data->pdown[p] - data->pup[p];
                if (dv > 0) {
                    data->pvy[p] = MIN(data->pvy[p] + data->paccel, data->pvmax);
                }
                else if (dv < 0) {
                    data->pvy[p] = MAX(data->pvy[p] - data->paccel, -data->pvmax);
                }
                else if (data->pvy[p] > 0) {
                    data->pvy[p] = MAX(data->pvy[p] - data->paccel, 0);
                }
                else if (data->pvy[p] < 0) {
                    data->pvy[p] = MIN(data->pvy[p] + data->paccel, 0);
                }

                // check if ball is in paddle path
                if (data->ball_x < data->px[p] + data->pwidth && data->ball_x + data->bsize > data->px[p]) {
                    continue;
                }

                py = data->py[p] + data->pvy[p];
                py = MIN(py, data->field_y2 - data->pheight);
                py = MAX(py, data->field_y1);

                if (py != data->py[p]) {
                    MovePaddle(app, p, py);
                }
            }

            // move ball
            by = data->ball_y + (data->ball_vy * data->bvel / 2);
            if (by >= data->field_y2 - data->bsize) {
                by -= 2*(by - (data->field_y2 - data->bsize));
                data->ball_vy = -1;
            }
            else if (by <= data->field_y1) {
                by += 2*(data->field_y1 - by);
                data->ball_vy = 1;
            }

            int hit = 0;
            bx = data->ball_x + (data->ball_vx * data->bvel);
            if (bx < data->field_x1) {
                if (data->py[0] < by + data->bsize && data->py[0] + data->pheight > by) {
                    // hit
                    bx += 2*(data->field_x1 - bx);
                    data->ball_vx = 1;
                    hit = 1;
                }
                else if (bx < data->field_x1 - data->bsize) {
                    data->playing = 0;
                }
            }
            else if (bx + data->bsize > data->field_x2) {
                if (data->py[1] < by + data->bsize && data->py[1] + data->pheight > by) {
                    // hit
                    bx -= 2*(bx + data->bsize - data->field_x2);
                    data->ball_vx = -1;
                    hit = 1;
                }
                else if (bx > data->field_x2) {
                    data->playing = 0;
                }
            }

            if (hit != 0) {
                data->hits++;
                if (data->hits % HITS_PER_ACCEL == 0) {
                    data->bvel += data->baccel;
                }
                ShowScore(app);
            }

            if (data->playing == 0) {
                if (data->hits > data->highscore) {
                    data->highscore = data->hits;
                    data->parameters_dirty = 1;
                    ShowHighScore(app);
                }
                ShowStartMessage(app);
            }

            MoveBall(app, bx, by);

            XFlush(app->display);
        }
    }
}

static void EventHandler(AppType * app, XEvent * event) {
    AppDataType * data = (AppDataType *)app->data;

    if (event->type == KeyPress || event->type == KeyRelease) {
        KeySym k = XLookupKeysym((XKeyPressedEvent *)event, 0);

        if (k == 'a') {
            data->pup[0] = (event->type == KeyPress ? 1 : 0);
        }
        else if (k == 'z') {
            data->pdown[0] = (event->type == KeyPress ? 1 : 0);
        }
        else if (k == '\'') {
            data->pup[1] = (event->type == KeyPress ? 1 : 0);
        }
        else if (k == '/') {
            data->pdown[1] = (event->type == KeyPress ? 1 : 0);
        }
        else if (k == ' ') {
            if (event->type == KeyRelease && data->playing == 0) {
                SetField(app);
                data->playing = 1;
                XFlush(app->display);
            }
        }
    }
}

static void DrawText(AppType * app, int x, int y, char * text) {
    AppDataType * data = (AppDataType *)app->data;

    XDrawString(app->display, app->window, data->gc, x - (strlen(text)*app->mediumfont.width)/2 + app->mediumfont.baseline_x, y - app->mediumfont.height/2 + app->mediumfont.baseline_y, text, strlen(text));
}

static void ShowScore(AppType * app) {
    AppDataType * data = (AppDataType *)app->data;

    char buffer[10];
    XClearArea(app->display, app->window, 0, 0, app->width, data->field_y1 - data->bsize, False);
    sprintf(buffer, "%d", data->hits);
    DrawText(app, app->width/2, data->msg_y, buffer);
}

static void ShowHighScore(AppType * app) {
    AppDataType * data = (AppDataType *)app->data;

    char buffer[30];
    sprintf(buffer, "High Score: %d", data->highscore);

    int w = strlen(buffer) * app->mediumfont.width;

    XClearArea(app->display, app->window, app->width/2 - w/2, app->height * 19/20, w, app->height/20, False);
    XSetForeground(app->display, data->gc, COMMENT_COLOR);
    XSetFont(app->display, data->gc, app->smallfont.id);
    XDrawString(app->display, app->window, data->gc, app->width/2 - w/2 + app->mediumfont.baseline_x, app->height - app->smallfont.height + app->smallfont.baseline_y, buffer, strlen(buffer));
    XSetForeground(app->display, data->gc, FG_COLOR);
    XSetFont(app->display, data->gc, app->mediumfont.id);
}

static void ShowStartMessage(AppType * app) {
    AppDataType * data = (AppDataType *)app->data;

    int x1, x2, y1, y2;
    x1 = data->field_x1 + app->mediumfont.width*4;
    x2 = data->field_x2 - app->mediumfont.width*4;
    y1 = app->height/2 - app->mediumfont.height*2 - app->mediumfont.height/16;
    y2 = app->height/2 + app->mediumfont.height*2 - app->mediumfont.height/16;
    DrawText(app, app->width/2, app->height/2, "Press Space to Start");
    DrawText(app, x1, app->height/2 - app->mediumfont.height, "A");
    DrawText(app, x1, app->height/2 + app->mediumfont.height, "Z");
    DrawText(app, x2, app->height/2 - app->mediumfont.height, "'");
    DrawText(app, x2, app->height/2 + app->mediumfont.height, "/");

    XSegment segments[] = {
        {x1, y1, x1 + app->mediumfont.width, y1 + app->mediumfont.width},
        {x1, y1, x1 - app->mediumfont.width, y1 + app->mediumfont.width},
        {x1, y2, x1 + app->mediumfont.width, y2 - app->mediumfont.width},
        {x1, y2, x1 - app->mediumfont.width, y2 - app->mediumfont.width},
        {x2, y1, x2 + app->mediumfont.width, y1 + app->mediumfont.width},
        {x2, y1, x2 - app->mediumfont.width, y1 + app->mediumfont.width},
        {x2, y2, x2 + app->mediumfont.width, y2 - app->mediumfont.width},
        {x2, y2, x2 - app->mediumfont.width, y2 - app->mediumfont.width},
    };
    XDrawSegments(app->display, app->window, data->gc, segments, 8);
}

static void SetField(AppType * app) {
    AppDataType * data = (AppDataType *)app->data;

    XClearArea(app->display, app->window, 0, data->field_y1, app->width, data->field_y2 - data->field_y1, False);
    data->py[0] = (data->field_y1 + data->field_y2 - data->pheight)/2;
    data->py[1] = data->py[0];
    data->ball_x = app->width/2 - data->bsize/2;
    data->ball_y = app->height/2 - data->bsize/2;
    data->ball_vx = (rand() % 2 == 0 ? 1 : -1);
    data->ball_vy = (rand() % 2 == 0 ? 1 : -1);
    MovePaddle(app, 0, data->py[0]);
    MovePaddle(app, 1, data->py[1]);
    data->bvel = data->bvelinit;
    data->hits = 0;
    ShowScore(app);
    for (int p = 0; p < 2; p++) {
        data->pup[p] = 0;
        data->pdown[p] = 0;
        data->pvy[p] = 0;
    }
    data->clock = clock();
}

static void MovePaddle(AppType * app, int player, int y) {
    AppDataType * data = (AppDataType *)app->data;

    XClearArea(app->display, app->window, data->px[player], data->py[player], data->pwidth, data->pheight, False);
    data->py[player] = y;
    XFillRectangle(app->display, app->window, data->gc, data->px[player], data->py[player], data->pwidth, data->pheight);
}

static void MoveBall(AppType * app, int x, int y) {
    AppDataType * data = (AppDataType *)app->data;

    XClearArea(app->display, app->window, data->ball_x, data->ball_y, data->bsize, data->bsize, False);
    data->ball_x = x;
    data->ball_y = y;
    XFillRectangle(app->display, app->window, data->gc, data->ball_x, data->ball_y, data->bsize, data->bsize);
}
