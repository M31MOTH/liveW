#include "xwin.h"

void initWindow(xwin *win);
void initBackground(xwin *win);

xwin *init_xwin()
{
    xwin *win = (xwin *)malloc(sizeof(struct xwin));

    if (!(win->display = XOpenDisplay(NULL))) {
		printf("Couldn't open X11 display\n");
		exit(1);
	}

    if (cfg.windowed)
        initWindow(win);
    else
        initBackground(win);

    uint32_t cardinal_alpha = (uint32_t) (cfg.transparency * (uint32_t)-1) ;
    XChangeProperty(win->display, win->window,
                    XInternAtom(win->display, "_NET_WM_WINDOW_OPACITY", 0),
                    XA_CARDINAL, 32, PropModeReplace, (uint8_t*) &cardinal_alpha,1);

    return win;
}

void initWindow(xwin *win)
{
	int attr[] = {
		GLX_RGBA,
		GLX_RED_SIZE, 1,
		GLX_GREEN_SIZE, 1,
		GLX_BLUE_SIZE, 1,
		GLX_DOUBLEBUFFER, True,
		None
	};

	win->screenNum = DefaultScreen(win->display);
	win->root = RootWindow(win->display, win->screenNum);

	win->width = cfg.width, win->height = cfg.height;

	int elemc;
	win->fbc = glXChooseFBConfig(win->display, win->screenNum, NULL, &elemc);
	if (!win->fbc) {
		printf("Couldn't get FB configs\n");
		exit(1);
	}

	win->vi = glXChooseVisual(win->display, win->screenNum, attr);

	if (!win->vi) {
		printf("Couldn't get a visual\n");
		exit(1);
	}

	// Window parameters
	win->swa.background_pixel = 0;
	win->swa.border_pixel = 0;
	win->swa.colormap = XCreateColormap(win->display, win->root, win->vi, AllocNone);
	win->swa.event_mask = StructureNotifyMask | ExposureMask | KeyPressMask;
	unsigned long mask = CWBackPixel | CWBorderPixel | CWColormap | CWEventMask;

	if (cfg.debug)
        printf("Window depth %d, %dx%d\n", win->vi->depth, win->width, win->height);

	win->window = XCreateWindow(win->display, win->root, -1, -1, win->width, win->height, 0,
			win->vi->depth, InputOutput, win->vi->visual, mask, &win->swa);
}

static Window find_subwindow(xwin *r, Window win, int w, int h)
{
    unsigned int i, j;
    Window troot, parent, *children;
    unsigned int n;

    for (i = 0; i < 10; i++) {
        XQueryTree(r->display, win, &troot, &parent, &children, &n);

        for (j = 0; j < n; j++) {
            XWindowAttributes attrs;

            if (XGetWindowAttributes(r->display, children[j], &attrs)) {
                if (attrs.map_state != 0 && (attrs.width == w && attrs.height == h)) {
                    win = children[j];
                    break;
                }
            }
        }

        XFree(children);
        if (j == n) {
            break;
        }
    }

    return win;
}

static Window find_desktop_window(xwin *r)
{
    Window root = RootWindow(r->display, r->screenNum);
    Window win = root;

    win = find_subwindow(r, root, -1, -1);

    win = find_subwindow(r, win, r->width, r->height);

    if (cfg.debug) {
        if (win != root) {
            printf("Desktop window (%lx) is subwindow of root window (%lx)\n", win, root);
        } else {
            printf("Desktop window (%lx) is root window\n", win);
        }
    }

    r->root = root;
    r->desktop = win;

    return win;
}

void initBackground(xwin *win)
{
	int attr[] = {
		GLX_RGBA,
		GLX_RED_SIZE, 1,
		GLX_GREEN_SIZE, 1,
		GLX_BLUE_SIZE, 1,
		GLX_DOUBLEBUFFER, True,
		None
	};

	win->screenNum = DefaultScreen(win->display);
	win->root = RootWindow(win->display, win->screenNum);

	win->width = DisplayWidth(win->display, win->screenNum),
	win->height = DisplayHeight(win->display, win->screenNum);

	if(!find_desktop_window(win)) {
		printf("Error: couldn't find desktop window\n");
		exit(1);
	}

	int elemc;
	win->fbc = glXChooseFBConfig(win->display, win->screenNum, NULL, &elemc);
	if (!win->fbc) {
		printf("Couldn't get FB configs\n");
		exit(1);
	}

	win->vi = glXChooseVisual(win->display, win->screenNum, attr);

	if (!win->vi) {
		printf("Couldn't get a visual\n");
		exit(1);
	}

	// Window parameters
	win->swa.background_pixmap = ParentRelative;
	win->swa.background_pixel = 0;
	win->swa.border_pixmap = 0;
	win->swa.border_pixel = 0;
	win->swa.bit_gravity = 0;
	win->swa.win_gravity = 0;
	win->swa.override_redirect = True;
	win->swa.colormap = XCreateColormap(win->display, win->root, win->vi, AllocNone);
	win->swa.event_mask = StructureNotifyMask | ExposureMask;
	unsigned long mask = CWOverrideRedirect | CWBackingStore | CWBackPixel | CWBorderPixel | CWColormap;

	if (cfg.debug)
        printf("Window depth %d, %dx%d\n", win->vi->depth, win->width, win->height);

	win->window = XCreateWindow(win->display, win->root, -1, -1, win->width, win->height, 0,
			win->vi->depth, InputOutput, win->vi->visual, mask, &win->swa);

	XLowerWindow(win->display, win->window);
}



void swapBuffers(xwin *win) {
	glXSwapBuffers(win->display, win->window);
    checkErrors("Swapping buffs");
}