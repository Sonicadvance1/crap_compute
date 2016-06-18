#include <stdio.h>
#include <waffle-1/waffle.h>
#include <waffle-1/waffle_x11_egl.h>
#include <waffle-1/waffle_glx.h>
#include <X11/Xlib.h>

#include "Context.h"

namespace Context
{
	waffle_display* dpy;
	waffle_window* win;
	waffle_context* ctx;

	void SetWindowTitle()
	{
		waffle_native_window* native_win = waffle_window_get_native(win);
		if (native_win->x11_egl && false)
		{
			printf("X11 EGL\n");
			XStoreName(native_win->x11_egl->display.xlib_display,
			           native_win->x11_egl->xlib_window,
				     "OpenGL1234");
		}
		else if (native_win->glx)
		{
			printf("GLX\n");
			XStoreName(native_win->glx->xlib_display,
			           native_win->glx->xlib_window,
				     "OpenGL1234");

		}
	}

	void Create()
	{
		int32_t init_attribs[] = {
			WAFFLE_PLATFORM, WAFFLE_PLATFORM_GLX,
			WAFFLE_NONE,
		};

		int32_t config_attribs[] = {
			WAFFLE_CONTEXT_API, WAFFLE_CONTEXT_OPENGL_ES3,
			WAFFLE_RED_SIZE, 8,
			WAFFLE_GREEN_SIZE, 8,
			WAFFLE_BLUE_SIZE, 8,
			WAFFLE_ALPHA_SIZE, 8,
			WAFFLE_DOUBLE_BUFFERED, 1,
			WAFFLE_NONE,
		};

		// Init library
		if (!waffle_init(init_attribs))
			printf("Couldn't initialize waffle!\n");

		// Open display
		dpy = waffle_display_connect(nullptr);

		if (!waffle_display_supports_context_api(dpy, WAFFLE_CONTEXT_OPENGL_ES3))
			printf("Display doesn't support ES 3!\n");

		// Get the config we want
		waffle_config* cfg = waffle_config_choose(dpy, config_attribs);
		if (!cfg)
			printf("Couldn't get waffle config!\n");

		// Create our window
		win = waffle_window_create(cfg, 256, 256);

		SetWindowTitle();

		if (!win)
			printf("Couldn't create waffle window!\n");

		waffle_window_show(win);

		// Create OpenGL context
		ctx = waffle_context_create(cfg, nullptr);

		if (!ctx)
			printf("Couldn't create waffle context!\n");

		waffle_config_destroy(cfg);

		// Make Current
		waffle_make_current(dpy, win, ctx);

		printf("Test: %p\n", waffle_get_proc_address("glGetString"));
	}

	void Shutdown()
	{
		waffle_context_destroy(ctx);
		waffle_window_destroy(win);

		waffle_display_disconnect(dpy);
	}

	void Swap()
	{
		waffle_window_swap_buffers(win);
	}
}
