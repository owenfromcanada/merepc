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
#include <X11/cursorfont.h>
#include <X11/keysym.h>
#include "merepc.h"
#include "applications.h"
#include "parameters.h"


static int Init(AppType * app, int w, int h);
static void Run(AppType * app);
static void Destroy(AppType * app);


int main(int argc, char *argv[]) {
    AppType app;
    int w = -1;
    int h = -1;
    int count;

    if (argc > 1) {
        count = sscanf(argv[1], "%dx%d", &w, &h);
        if (count < 2) {
            printf("Usage:  merepc [WIDTHxHEIGHT]\n     where WIDTH and HEIGHT are the resolution in pixels (optional)\n");
            return 1;
        }
    }

    if (Init(&app, w, h) == 0) {
        Run(&app);
        Destroy(&app);
    }
    else {
        printf("Initialization failed\n");
        return 1;
    }

    return 0;
}

static int Init(AppType * app, int w, int h) {
    // initialize display and root window

    app->display = XOpenDisplay(NULL);
    if (app->display == NULL) {
        printf("Cannot open display");
        return 1;
    }

    Window root = XDefaultRootWindow(app->display);

    // capture input events

    //Cursor cursor = XCreateFontCursor(app->display, XC_left_ptr);
    //XDefineCursor(app->display, root, cursor);

    if (XGrabKeyboard(app->display, root, False, GrabModeAsync, GrabModeAsync, CurrentTime) != GrabSuccess) {
        printf("Cannot grab keyboard\n");
        return 1;
    }
    if (XGrabPointer(app->display, root, False, ButtonPressMask | ButtonReleaseMask | PointerMotionMask, GrabModeAsync, GrabModeAsync, None, None, CurrentTime) != GrabSuccess) {
        printf("Cannot grab pointer\n");
        return 1;
    }

    XAutoRepeatOff(app->display);

    // set up app window

    if (w <= 0 || h <= 0) {
        XWindowAttributes rootattributes;
        assert(XGetWindowAttributes(app->display, root, &rootattributes) == True);
        app->width = rootattributes.width;
        app->height = rootattributes.height;
    }
    else {
        app->width = w;
        app->height = h;
    }

    int attributemask = CWBackPixel;
    XSetWindowAttributes attributes = {};
    attributes.background_pixel = BG_COLOR;

    app->window = XCreateWindow(app->display, root, 0, 0, app->width, app->height, 0, CopyFromParent, CopyFromParent, CopyFromParent, attributemask, &attributes);

    XSelectInput(app->display, app->window, StructureNotifyMask);

    XMapWindow(app->display, app->window);
    XFlush(app->display);

    // initialize fonts

    char fontname[100];

    sprintf(fontname, "-*-roboto mono medium-medium-r-normal--%d-0-0-0-*-0-ascii-0", app->height/11);
    app->bigfont.font = XLoadQueryFont(app->display, fontname);
    if (app->bigfont.font == NULL) {
        printf("Couldn't load font\n");
        return 1;
    }
    app->bigfont.id = app->bigfont.font->fid;
    app->bigfont.width = app->bigfont.font->max_bounds.rbearing - app->bigfont.font->min_bounds.lbearing;
    app->bigfont.height = app->bigfont.font->max_bounds.ascent - app->bigfont.font->min_bounds.descent;
    app->bigfont.baseline_x = -app->bigfont.font->min_bounds.lbearing;
    app->bigfont.baseline_y = app->bigfont.font->max_bounds.ascent;

    sprintf(fontname, "-*-roboto mono medium-medium-r-normal--%d-0-0-0-*-0-ascii-0", app->height/33);
    app->mediumfont.font = XLoadQueryFont(app->display, fontname);
    if (app->mediumfont.font == NULL) {
        printf("Couldn't load font\n");
        return 1;
    }
    app->mediumfont.id = app->mediumfont.font->fid;
    app->mediumfont.width = app->mediumfont.font->max_bounds.rbearing - app->mediumfont.font->min_bounds.lbearing;
    app->mediumfont.height = app->mediumfont.font->max_bounds.ascent - app->mediumfont.font->min_bounds.descent;
    app->mediumfont.baseline_x = -app->mediumfont.font->min_bounds.lbearing;
    app->mediumfont.baseline_y = app->mediumfont.font->max_bounds.ascent;

    sprintf(fontname, "-*-roboto mono medium-medium-r-normal--%d-0-0-0-*-0-ascii-0", app->height/48);
    app->smallfont.font = XLoadQueryFont(app->display, fontname);
    if (app->smallfont.font == NULL) {
        printf("Couldn't load font\n");
        return 1;
    }
    app->smallfont.id = app->smallfont.font->fid;
    app->smallfont.width = app->smallfont.font->max_bounds.rbearing - app->smallfont.font->min_bounds.lbearing;
    app->smallfont.height = app->smallfont.font->max_bounds.ascent - app->smallfont.font->min_bounds.descent;
    app->smallfont.baseline_x = -app->smallfont.font->min_bounds.lbearing;
    app->smallfont.baseline_y = app->smallfont.font->max_bounds.ascent;

    // initialize parameters
    InitParameters();

    return 0;
}

static void Run(AppType * app) {
    app->appcalls = NULL;
    Bool quit = False;
    Bool ctrl_down = False;
    Bool shift_down = False;
    Bool f12_down = False;
    Bool init = False;
    KeySym k;
    XEvent event;
    while (quit == False) {

        while (XPending(app->display)) {
            XNextEvent(app->display, &event);

            switch (event.type) {
                case MapNotify:
                    init = True;
                    SwitchApp(app, &Menu);
                    break;
                
                case KeyPress:
                    k = XLookupKeysym((XKeyPressedEvent *)&event, 0);
                    if (k == XK_Control_L) {
                        ctrl_down = True;
                    }
                    else if (k == XK_Shift_L) {
                        shift_down = True;
                    }
                    else if (k == XK_F12) {
                        f12_down = True;
                    }

                    app->appcalls->EventHandler(app, &event);
                    break;
                
                case KeyRelease:
                    k = XLookupKeysym((XKeyPressedEvent *)&event, 0);
                    if (k == XK_Control_L) {
                        ctrl_down = False;
                    }
                    else if (k == XK_Shift_L) {
                        shift_down = False;
                    }
                    else if (k == XK_F12) {
                        f12_down = False;
                    }
                    else if (k == XK_Escape) {
                        SwitchApp(app, &Menu);
                    }

                    app->appcalls->EventHandler(app, &event);
                    break;

                case ButtonPress:
                    break;

                case ButtonRelease:
                    break;

                case MotionNotify:
                    break;
            }

            if (ctrl_down == True && shift_down == True && f12_down == True) {
                quit = True;
            }
        }

        if (init == True) {
            app->appcalls->Tick(app);
        }
    }

    SwitchApp(app, NULL);
}

static void Destroy(AppType * app) {
    XDestroyWindow(app->display, app->window);

    XFreeFont(app->display, app->smallfont.font);
    XFreeFont(app->display, app->bigfont.font);

    XCloseDisplay(app->display);
}

void SwitchApp(AppType * app, const AppStaticType * appcalls) {
    if (app->appcalls != NULL) {
        app->appcalls->Destroy(app);
        app->appcalls = NULL;
        app->data = NULL;
    }

    XClearArea(app->display, app->window, 0, 0, app->width, app->height, False);

    if (appcalls != NULL) {
        app->appcalls = appcalls;
        app->appcalls->Init(app);
    }
}
