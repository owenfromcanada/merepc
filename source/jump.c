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
#define NUM_STAGES          5
#define NUM_OBSTACLES       6

typedef struct {
    int x;
    int y;
} SpeckType;

typedef struct {
    int x;
    int y;
    int w;
    int h;
} ObstacleType;

typedef struct {
    // written once during init
    GC gc;
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
    int speck_size;
    int obstacle_size;
    int ground_y;
    int ground_height;
    int sky_y;
    int sky_height;
    int obstacle_spacing_min[NUM_STAGES];
    int obstacle_spacing_jitter[NUM_STAGES];
    // updated during gameplay
    int score_subcount; // frames
    int score; // seconds
    int vx;
    float vy;
    float y;
    int jump_tick;
    int jump_key;
    int jump_state;
    SpeckType specks[NUM_SPECKS];
    int speck_start_idx;
    int speck_end_idx;
    ObstacleType obstacles[NUM_OBSTACLES];
    int obstacle_start_idx;
    int obstacle_end_idx;
    int stage;
    int start_lock;
    int playing;

} AppDataType;

static void Init(AppType * app);
static void Destroy(AppType * app);
static void Tick(AppType * app);
static void EventHandler(AppType * app, XEvent * event);
static void Start(AppType * app);
static void Frame(AppType * app);
static void ShowStartMessage(AppType * app);
static void DrawText(AppType * app, int x, int y, char * text);

const AppStaticType Jump = {
    Init,
    Destroy,
    Tick,
    EventHandler
};

static const int STAGE_TIME[NUM_STAGES] = {2, 20, 60, 120, 0}; // in seconds
static const int MAX_OBSTACLE_WIDTH[NUM_STAGES] = {1, 1, 1, 2, 3};
static const int MAX_OBSTACLE_HEIGHT[NUM_STAGES] = {1, 1, 2, 2, 3};
static const int MIN_OBSTACLE_SPACING[NUM_STAGES] = {16, 14, 11, 10, 9}; // in units of player width
static const int MAX_OBSTACLE_SPACING[NUM_STAGES] = {24, 22, 20, 18, 16}; // in units of player width

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
    data->sky_y = data->score_y + app->mediumfont.height;
    data->sky_height = data->origin_y - data->sky_y;
    data->eyes_height = data->speck_size*2;
    data->eyes_y = (data->psize/2) + (data->eyes_height/2);
    data->eyes_x1 = data->origin_x - data->speck_size*2;
    data->eyes_x2 = data->origin_x - data->speck_size*4;
    data->obstacle_size = data->psize;
    for (int k = 0; k < NUM_STAGES; k++) {
        data->obstacle_spacing_min[k] = MIN_OBSTACLE_SPACING[k] * data->psize;
        data->obstacle_spacing_jitter[k] = (MAX_OBSTACLE_SPACING[k] - MIN_OBSTACLE_SPACING[k]) * data->psize;
    }

    data->start_lock = 0;
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
                data->jump_key = 1;
                if (data->playing == 1) { 
                    data->start_lock = 1;
                }
            }
            else {
                data->jump_key = 0;
                if (data->playing == 0 && data->start_lock == 0) {
                    Start(app);
                    XFlush(app->display);
                }
                data->start_lock = 0;
            }
        }
    }
}

static void Start(AppType * app) {
    AppDataType * data = (AppDataType *)app->data;

    XClearArea(app->display, app->window, 0, 0, app->width, app->height * 19/20, False);
    XFillRectangle(app->display, app->window, data->gc, 0, data->origin_y, app->width, data->ground_y - data->origin_y);

    data->score_subcount = 0;
    data->score = 0;
    data->vx = data->base_vx;
    data->vy = 0.0F;
    data->y = 0.0F;
    data->jump_tick = 0;
    data->jump_state = 3;
    data->speck_start_idx = 0;
    data->speck_end_idx = 0;
    data->obstacle_start_idx = 0;
    data->obstacle_end_idx = 1;
    data->obstacles[0].x = app->width * 2;
    data->obstacles[0].y = data->origin_y - data->obstacle_size;
    data->obstacles[0].w = data->obstacle_size;
    data->obstacles[0].h = data->obstacle_size;
    data->stage = 0;
    data->playing = 1;
    data->clock = clock();
}

static void Frame(AppType * app) {
    AppDataType * data = (AppDataType *)app->data;
    
    int redraw_score = 0;

    // ******************* Game Logic *******************

    // update score
    data->score_subcount++;
    if (data->score_subcount >= FRAMES_PER_SEC) {
        data->score_subcount = 0;
        if (data->score < 999999999) {
            data->score++;
            redraw_score = 1;
        }

        // update stage
        if (data->stage < NUM_STAGES - 1 && data->score >= STAGE_TIME[data->stage]) {
            data->stage++;
        }
    }

    // determine vertical velocity
    if (data->jump_state == 0 && data->jump_key == 1) {
        // initialize jump
        data->vy = data->jump_velocity;
        data->jump_tick = 0;
        data->jump_state = 1;
    }
    else if (data->jump_state == 1) {
        // continue jump if button remains pressed
        data->jump_tick++;
        if (data->jump_tick >= data->jump_timeout || data->jump_key == 0) {
            data->jump_state = 2;
        }
    }
    else if (data->jump_state == 2) {
        // gravity takes over
        data->vy += data->jump_gravity;
    }

    // update avatar position
    if (data->jump_state != 0) {
        // move avatar
        data->y += data->vy;

        // check for landing
        if (data->y >= 0.0F) {
            data->y = 0.0F;
            data->jump_state = 0;
        }
    }

    // update obstacles
    int count = data->obstacle_end_idx - data->obstacle_start_idx + (data->obstacle_end_idx < data->obstacle_start_idx ? NUM_OBSTACLES : 0);
    if (count < NUM_OBSTACLES - 1) {
        // add obstacles
        data->obstacles[data->obstacle_end_idx].w = ((rand() % MAX_OBSTACLE_WIDTH[data->stage]) + 1) * data->obstacle_size;
        data->obstacles[data->obstacle_end_idx].h = ((rand() % MAX_OBSTACLE_HEIGHT[data->stage]) + 1) * data->obstacle_size;
        data->obstacles[data->obstacle_end_idx].y = data->origin_y - data->obstacles[data->obstacle_end_idx].h;

        int last_x = 0;
        if (count > 0) {
            int last = (data->obstacle_end_idx == 0 ? NUM_OBSTACLES - 1 : data->obstacle_end_idx - 1);
            last_x = data->obstacles[last].x;
        }
        data->obstacles[data->obstacle_end_idx].x = last_x + data->obstacle_spacing_min[data->stage] + (rand() % data->obstacle_spacing_jitter[data->stage]);
        data->obstacle_end_idx = (data->obstacle_end_idx + 1) % NUM_OBSTACLES;
    }
    for (int k = data->obstacle_start_idx; k != data->obstacle_end_idx; k = (k + 1) % NUM_OBSTACLES) {
        // move obstacles
        data->obstacles[k].x -= data->vx;

        // check for collision
        if ( (data->obstacles[k].x < data->origin_x) &&
             (data->obstacles[k].x + data->obstacles[k].w > data->origin_x - data->psize) &&
             (data->obstacles[k].y < data->origin_y + (int)data->y) ) {
            data->playing = 0;
        }
        // check for obstacles going off screen
        else if (data->obstacles[k].x < -data->obstacles[k].w) {
            data->obstacle_start_idx = (data->obstacle_start_idx + 1) % NUM_OBSTACLES;
        }
    }

    // update specks
    count = data->speck_end_idx - data->speck_start_idx + (data->speck_end_idx < data->speck_start_idx ? NUM_SPECKS : 0);
    if (count < NUM_SPECKS - 1) {
        // add specks
        if ((rand() % 20) < 1) {
            data->specks[data->speck_end_idx].y = (rand() % (data->ground_height - (data->speck_size * 2))) + data->origin_y + data->speck_size;
            data->specks[data->speck_end_idx].x = app->width;
            data->speck_end_idx = (data->speck_end_idx + 1) % NUM_SPECKS;
        }
    }
    for (int k = data->speck_start_idx; k != data->speck_end_idx; k = (k + 1) % NUM_SPECKS) {
        // move specks
        data->specks[k].x -= data->vx;

        // check for specks going off screen
        if (data->specks[k].x < -data->speck_size) {
            data->speck_start_idx = (data->speck_start_idx + 1) % NUM_SPECKS;
        }
    }

    // ******************* Draw Frame *******************

    // draw score
    if (redraw_score == 1) {
        char buffer[10];
        sprintf(buffer, "%d", data->score);
        DrawText(app, app->width/2, data->score_y, buffer);
    }

    // clear sky
    XClearArea(app->display, app->window, 0, data->sky_y, app->width, data->sky_height, False);

    // draw obstacles
    for (int k = data->obstacle_start_idx; k != data->obstacle_end_idx; k = (k + 1) % NUM_OBSTACLES) {
        if (data->obstacles[k].x < app->width) {
            XFillRectangle(app->display, app->window, data->gc, data->obstacles[k].x, data->obstacles[k].y, data->obstacles[k].w, data->obstacles[k].h);
        }
    }

    // clear ground
    XClearArea(app->display, app->window, 0, data->ground_y, app->width, data->ground_height, False);

    // draw specks
    XSetForeground(app->display, data->gc, COMMENT_COLOR);
    for (int k = data->speck_start_idx; k != data->speck_end_idx; k = (k + 1) % NUM_SPECKS) {
        XFillRectangle(app->display, app->window, data->gc, data->specks[k].x, data->specks[k].y, data->speck_size, data->speck_size);
    }
    XSetForeground(app->display, data->gc, FG_COLOR);

    // draw avatar
    XFillRectangle(app->display, app->window, data->gc, data->origin_x - data->psize, data->origin_y + (int)data->y - data->psize, data->psize, data->psize);

    // draw eyes
    int eyes_y = data->origin_y + (int)data->y - data->eyes_y + (data->jump_state == 0 ? 0 : (data->vy > 0 ? data->speck_size : -data->speck_size));
    XClearArea(app->display, app->window, data->eyes_x1, eyes_y, data->speck_size, data->eyes_height, False);
    XClearArea(app->display, app->window, data->eyes_x2, eyes_y, data->speck_size, data->eyes_height, False);

    // check for end of game
    if (data->playing == 0) {
        ShowStartMessage(app);
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
