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

#define FRAMES_PER_SEC      60
#define CLOCKS_PER_FRAME    (CLOCKS_PER_SEC/FRAMES_PER_SEC)
#define JUMP_TIME_TO_PEAK   (0.4F * FRAMES_PER_SEC)
#define NUM_SPECKS          8

typedef struct {
    int x;
    int y;
} SpeckType;

typedef struct {
    // written once during init
    GC gc;
    int playing;
    clock_t clock;
    int origin_x;
    int origin_y;
    int psize;
    int base_vx; // pixels per frame
    int jump_height_min;
    int jump_height_max;
    float jump_velocity; // pixels per frame
    float jump_gravity; // pixels per frame per frame
    int jump_timeout; // in frames
    int score_y;
    int eyes_y;
    int eyes_height;
    int eyes_x1;
    int eyes_x2;
    // updated during gameplay
    int time; // in frames
    int vx;
    float vy;
    float y;
    int jump_tick;
    int jump_key;
    int ground_y;
    int ground_height;
    int speck_size;
    int jump_state;
    SpeckType specks[NUM_SPECKS];
    int speck_start_idx;
    int speck_end_idx;
} AppDataType;

static void Init(AppType * app);
static void Destroy(AppType * app);
static void Tick(AppType * app);
static void EventHandler(AppType * app, XEvent * event);
static void Start(AppType * app);
static void BeginJump(AppType * app);
static void Frame(AppType * app);
static void ShowStartMessage(AppType * app);
static void DrawText(AppType * app, int x, int y, char * text);

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
    XSetFont(app->display, data->gc, app->mediumfont.id);

    data->origin_x = app->width / 8;
    data->origin_y = app->height * 2/3;
    data->psize = app->height / 16;
    data->base_vx = app->width / (3 * FRAMES_PER_SEC); // 3 seconds per screen width
    data->jump_height_min = app->height / 8;
    data->jump_height_max = app->height / 3;
    data->jump_velocity = (-2.0F * (float)data->jump_height_min) / JUMP_TIME_TO_PEAK;
    data->jump_gravity = (2.0F * (float)data->jump_height_min) / (JUMP_TIME_TO_PEAK * JUMP_TIME_TO_PEAK);
    data->jump_timeout = (int)((data->jump_height_max - data->jump_height_min) / -data->jump_velocity);
    data->speck_size = (app->height/128);
    data->ground_y = data->origin_y + data->speck_size;
    data->ground_height = (app->height*19/20) - data->ground_y;
    data->score_y = app->mediumfont.height*2;
    data->eyes_height = data->speck_size*2;
    data->eyes_y = (data->psize/2) + (data->eyes_height/2);
    data->eyes_x1 = data->origin_x - data->speck_size*2;
    data->eyes_x2 = data->origin_x - data->speck_size*4;

    data->playing = 0;

    ShowStartMessage(app);

    XSetForeground(app->display, data->gc, COMMENT_COLOR);
    XSetFont(app->display, data->gc, app->smallfont.id);
    char hints[] = "ESC: Menu";
    XDrawString(app->display, app->window, data->gc, app->smallfont.width, app->height - app->smallfont.height + app->smallfont.baseline_y, hints, strlen(hints));
    XSetForeground(app->display, data->gc, FG_COLOR);

    XFlush(app->display);
}

static void Destroy(AppType * app) {
    AppDataType * data = (AppDataType *)app->data;

    XFreeGC(app->display, data->gc);
    free(app->data);
    app->data = NULL;
}

static void Tick(AppType * app) {
    AppDataType * data = (AppDataType *)app->data;

    if (data->playing != 0) {
        if (clock() - data->clock >= CLOCKS_PER_FRAME) {
            data->clock += CLOCKS_PER_FRAME;
            Frame(app);
            XFlush(app->display);
        }
    }
}

static void EventHandler(AppType * app, XEvent * event) {
    AppDataType * data = (AppDataType *)app->data;

    if (event->type == KeyPress || event->type == KeyRelease) {
        KeySym k = XLookupKeysym((XKeyPressedEvent *)event, 0);

        if (k == ' ') {
            if (event->type == KeyPress) {
                if (data->playing == 1) {
                    data->jump_key = 1;
                    BeginJump(app);
                }
            }
            else { // KeyRelease
                if (data->playing == 0) {
                    Start(app);
                    XFlush(app->display);
                }
                else {
                    data->jump_key = 0;
                }
            }
        }
    }
}

static void Start(AppType * app) {
    AppDataType * data = (AppDataType *)app->data;

    XClearArea(app->display, app->window, 0, 0, app->width, app->height * 19/20, False);
    XFillRectangle(app->display, app->window, data->gc, 0, data->origin_y, app->width, data->ground_y - data->origin_y);

    for (int k = 0; k < NUM_SPECKS; k++) {
        data->specks[k].x = -1;
        data->specks[k].y = 0;
    }
    data->speck_start_idx = 0;
    data->speck_end_idx = 0;
    data->time = 0;
    data->y = 0.0F;
    data->vy = 0.0F;
    data->vx = data->base_vx;
    data->jump_state = 3;
    data->clock = clock();
    data->playing = 1;
}

static void BeginJump(AppType * app) {
    AppDataType * data = (AppDataType *)app->data;

    if (data->jump_state == 0) {
        data->jump_tick = 0;
        data->jump_state = 1;
    }
}

static void Frame(AppType * app) {
    AppDataType * data = (AppDataType *)app->data;

    // remove old avatar
    if (data->jump_state != 0) {
        XClearArea(app->display, app->window, data->origin_x - data->psize, data->origin_y + (int)data->y - data->psize, data->psize, data->psize, False);
    }

    // clear ground
    XClearArea(app->display, app->window, 0, data->ground_y, app->width, data->ground_height, False);

    // draw specks
    XSetForeground(app->display, data->gc, COMMENT_COLOR);
    int next = data->speck_end_idx == NUM_SPECKS - 1 ? 0 : data->speck_end_idx + 1;
    if (next != data->speck_start_idx) {
        if ((rand() % 20) < 1) {
            data->specks[next].y = (rand() % (data->ground_height - (data->speck_size * 2))) + data->origin_y + data->speck_size;
            data->specks[next].x = app->width;
            data->speck_end_idx = next;
        }
    }
    for (int k = data->speck_start_idx; k != data->speck_end_idx; k = (k + 1) % NUM_SPECKS) {
        data->specks[k].x -= data->vx;
        if (data->specks[k].x < -data->speck_size) {
            data->speck_start_idx = (data->speck_start_idx + 1) % NUM_SPECKS;
        }
        else {
            XFillRectangle(app->display, app->window, data->gc, data->specks[k].x, data->specks[k].y, data->speck_size, data->speck_size);
        }
    }
    XSetForeground(app->display, data->gc, FG_COLOR);

    // determine vertical velocity
    if (data->jump_state == 1) {
        // initialize jump
        data->vy = data->jump_velocity;
        data->jump_state = 2;
    }
    else if (data->jump_state == 2) {
        // continue jump if button remains pressed
        data->jump_tick++;
        if (data->jump_tick >= data->jump_timeout || data->jump_key == 0) {
            data->jump_state = 3;
        }
    }
    else if (data->jump_state == 3) {
        // gravity takes over
        data->vy += data->jump_gravity;
    }

    // draw avatar
    if (data->jump_state != 0) {
        data->y += data->vy;
        if (data->y >= 0.0F) {
            data->y = 0.0F;
            data->jump_state = 0;
        }
        XFillRectangle(app->display, app->window, data->gc, data->origin_x - data->psize, data->origin_y + (int)data->y - data->psize, data->psize, data->psize);
        int eyes_y = data->origin_y + (int)data->y - data->eyes_y + (data->jump_state == 0 ? 0 : (data->vy > 0 ? data->speck_size : -data->speck_size));

        XClearArea(app->display, app->window, data->eyes_x1, eyes_y, data->speck_size, data->eyes_height, False);
        XClearArea(app->display, app->window, data->eyes_x2, eyes_y, data->speck_size, data->eyes_height, False);
    }

    // show score
    if (data->time < 999999999) {
        data->time++;

        char buffer[10];
        sprintf(buffer, "%d", data->time/FRAMES_PER_SEC);
        DrawText(app, app->width/2, data->score_y, buffer);
    }
}

static void ShowStartMessage(AppType * app) {
    DrawText(app, app->width/2, app->height/2, "Press Space to Start");
}

static void DrawText(AppType * app, int x, int y, char * text) {
    AppDataType * data = (AppDataType *)app->data;

    int w = strlen(text) * app->mediumfont.width;
    int h = app->mediumfont.height;
    XClearArea(app->display, app->window, x - w/2, y - h/2, w, h, False);
    XDrawString(app->display, app->window, data->gc, x - w/2 + app->mediumfont.baseline_x, y - app->mediumfont.height/2 + app->mediumfont.baseline_y, text, strlen(text));
}
