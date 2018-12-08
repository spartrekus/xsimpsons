/* toon.c - for drawing cartoons on the root window - used by xpenguins
 * Copyright (C) 1999, 2000  Robin Hogan
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <limits.h>

#include "toon.h"

/* Handle some `virtual' window managers */
#include "vroot.h"

typedef int ErrorHandler();

typedef struct {
   int solid;
   unsigned int wid;
   XRectangle pos;
} _ToonWindowData;

Display *display;
int screen = 0;
Window root;
int display_width, display_height;
GC draw_toonGC;
Pixel black, white;
Region windows = NULL;
Window *children = NULL;
unsigned int nwindows = 0;

_ToonWindowData *windata = NULL;
ToonData *toon_data;
int error_value = 0;
/* Do the edges block movement?
 * If only the sides and the bottom block movement then edge_block = 2 */
char edge_block = 0;
char shaped_windows = 1;
/* If solid_popups is set to 0 then the toons fall behind `popup' windows.
 * This includes the KDE panel */
char solid_popups = 1;
int toon_signal = 0;
char toon_error_message[TOON_MESSAGE_LENGTH] = "";
Toon *toons_to_erase = NULL;
int ntoons_to_erase = 0;
int max_relocate_up = TOON_DEFAULTMAXRELOCATE;
int max_relocate_down = TOON_DEFAULTMAXRELOCATE;
int max_relocate_left = TOON_DEFAULTMAXRELOCATE;
int max_relocate_right = TOON_DEFAULTMAXRELOCATE;

/* INTERNAL FUNCTION PROTOTYPES */
void _ToonSignalHandler(int sig);
int _ToonError(Display *display, XErrorEvent *error);
void _ToonExitGracefully(int sig);

/* SIGNAL AND ERROR HANDLING FUNCTIONS */

/* Signal Handler: stores caught signal in toon_signal */
void _ToonSignalHandler(int sig)
{
   toon_signal=sig;
   return;
}

/* Error handler for X */
int _ToonXErrorHandler(Display *display, XErrorEvent *error)
{
    error_value = error->error_code;
    return 0;
}

/* Has a signal been caught? If so return it */
int ToonSignal()
{
   int sig = toon_signal;
   toon_signal = 0;
   return sig;
}

/* Return error message */
char * ToonErrorMessage()
{
   return toon_error_message;
}


/* STARTUP FUNCTIONS */

/* Open display, set GC etc. */
Display *ToonOpenDisplay(char *display_name)
{
   XGCValues gc_values;

   display=XOpenDisplay(display_name);
   if (display == NULL) {
      if (display_name == NULL && getenv("DISPLAY") == NULL)
         strncpy(toon_error_message,"DISPLAY environment variable not set",
               TOON_MESSAGE_LENGTH);
      else
         strncpy(toon_error_message,"Can't open display",
               TOON_MESSAGE_LENGTH);
      return(NULL);
   }
   screen = DefaultScreen(display);
   root = RootWindow(display, screen);
   black = BlackPixel(display, screen);
   white = WhitePixel(display, screen);
   display_width = DisplayWidth(display, screen);
   display_height = DisplayHeight(display, screen);

   /* Set Graphics Context */
   gc_values.function = GXcopy;
   gc_values.graphics_exposures= False;
   gc_values.fill_style = FillTiled;
   draw_toonGC = XCreateGC(display,root,
      GCFunction | GCFillStyle | GCGraphicsExposures,&gc_values);

   /* Regions */
   windows = XCreateRegion();

   /* Notify if the root window changes */
   XSelectInput(display, root, SubstructureNotifyMask);

   return display;
}

/* Configure signal handling and the way the toons behave via a bitmask */
/* Currently always returns 0 */
int ToonConfigure(unsigned long int code)
{
   if (code & TOON_EDGEBLOCK)
      edge_block=1;
   else if (code & TOON_SIDEBOTTOMBLOCK)
      edge_block=2;
   else if (code & TOON_NOEDGEBLOCK)
      edge_block=0;
   if (code & TOON_SOLIDPOPUPS)
      solid_popups=1;
   else if (code & TOON_NOSOLIDPOPUPS)
      solid_popups=0;
   if (code & TOON_SHAPEDWINDOWS)
      shaped_windows=1;
   else if (code & TOON_NOSHAPEDWINDOWS)
      shaped_windows=0;
   if (code & TOON_CATCHSIGNALS) {
      signal(SIGINT, _ToonSignalHandler);
      signal(SIGTERM, _ToonSignalHandler);
      signal(SIGHUP, _ToonSignalHandler);
   }
   else if (code & TOON_EXITGRACEFULLY) {
      signal(SIGINT, _ToonExitGracefully);
      signal(SIGTERM, _ToonExitGracefully);
      signal(SIGHUP, _ToonExitGracefully);
   }
   else if (code & TOON_NOCATCHSIGNALS) {
      signal(SIGINT, SIG_DFL);
      signal(SIGTERM, SIG_DFL);
      signal(SIGHUP, SIG_DFL);
   }
   return 0;
}

/* Store the pixmaps to the server */
/* Returns 0 on success, otherwise the return value from the Xpm function */
int ToonInstallData(ToonData *data, int n)
{
   int i, status;
   XpmAttributes attributes;
   for (i=0;i<n;i++) {
      attributes.valuemask |= (XpmReturnPixels
                | XpmReturnExtensions | XpmExactColors | XpmCloseness);
      attributes.exactColors=False;
      attributes.closeness=40000;
      if ((status = XpmCreatePixmapFromData(display, root, (data+i)->image,
            &((data+i)->pixmap), &((data+i)->mask), &attributes))) {
         return status;
      }      
   }

   toon_data=data;
   return 0;
}


/* DRAWING FUNCTIONS */

/* Draw the toons from toon[0] to toon[n-1] */
/* Currently always returns 0 */
int ToonDraw(Toon *toon, int n)
{
   int width,height,i;
   Toon *t;
   for (i=0;i<n;i++) {
      t=toon+i;
      if (t->active) {
      width=toon_data[t->type].width;
      height=toon_data[t->type].height;

      XSetClipOrigin(display, draw_toonGC,
         t->x-width*t->frame, t->y-height*t->direction); 
      XSetClipMask(display, draw_toonGC, 
         toon_data[t->type].mask);   
      XCopyArea(display,
         toon_data[t->type].pixmap,
         root,draw_toonGC,width*t->frame,height*t->direction,
         width,height,t->x,t->y);
      XSetClipMask(display, draw_toonGC, None);
      t->x_map = t->x;
      t->y_map = t->y;
      t->width_map = width;
      t->height_map = height;
      }
   }
    return 0;
}

/* Erase toons toon[0] to toon[n-1] */
/* Currently always returns 0 */
int ToonErase(Toon *toon,int n)
{
   int i;
   Toon *t;
   for (i=0;i<n;i++) {
      t=toon+i;
      XClearArea(display, root, t->x_map, t->y_map,
            t->width_map, t->height_map, False);
   }
   return 0;
}

/* Send any buffered X calls immediately */
void ToonFlush()
{
   XFlush(display);
   return;
}

/* QUERY FUNCTIONS */

/* Return the width of the display */
int ToonDisplayWidth()
{
   return display_width;
}

/* Return the height of the display */
int ToonDisplayHeight()
{
   return display_height;
}

/* Returns 1 if the toon is blocked in the specified direction, 0 if not 
   blocked and -1 if the direction arguments was out of bounds */
int ToonBlocked(Toon *toon, int direction)
{
   if (edge_block) {
      switch (direction) {
         case TOON_LEFT:
            if ((toon->x) <= 0)
               return 1;
            break;
         case TOON_RIGHT:
            if ((toon->x)+(toon_data[toon->type].width) >= display_width)
               return 1;
            break;
         case TOON_UP:
            if ((toon->y) <= 0)
               return 1;
            break;
         case TOON_DOWN:
            if ((toon->y)+(toon_data[toon->type].height) >= display_height)
               return 1;
            break;
      }
   }
   switch (direction) {
      case TOON_HERE:
         return (XRectInRegion(windows,toon->x,toon->y,
               toon_data[toon->type].width,toon_data[toon->type].height) 
               != RectangleOut);
      case TOON_LEFT:
         return (XRectInRegion(windows,toon->x-1,toon->y,
               1,toon_data[toon->type].height) != RectangleOut);
      case TOON_RIGHT:
         return (XRectInRegion(windows,toon->x+toon_data[toon->type].width,
               toon->y,1,toon_data[toon->type].height) != RectangleOut);
      case TOON_UP:
         return (XRectInRegion(windows,toon->x,toon->y-1,
               toon_data[toon->type].width,1) != RectangleOut);
      case TOON_DOWN:
         return (XRectInRegion(windows,toon->x,
               toon->y+toon_data[toon->type].height, toon_data[toon->type].width,
               1) != RectangleOut);
      default:
         return -1;
   }
}

/* Returns 1 if the toon would be in an occupied area if moved by xoffset
   and yoffset, 0 otherwise */
int ToonOffsetBlocked(Toon *toon, int xoffset, int yoffset)
{
   if (edge_block) {
      if (  ((toon->x + xoffset) <= 0) || 
            ((toon->x)+(toon_data[toon->type].width + xoffset) >= display_width) ||
            ((toon->y + yoffset) <= 0 && edge_block != 2) ||
            ((toon->y)+(toon_data[toon->type].height + yoffset) >= display_height)
            ) {
         return 1;
      }
   }
   return (XRectInRegion(windows,toon->x + xoffset,toon->y + yoffset,
         toon_data[toon->type].width,toon_data[toon->type].height) 
         != RectangleOut);
}

/* Returns 1 if any change to the top-level window configuration has occurred,
   0 otherwise */
int ToonWindowsMoved()
{
   XEvent event;
   int windows_moved=0;
   while (XPending(display)) {
      XNextEvent(display, &event);
      if (event.type == ConfigureNotify || event.type == MapNotify
            || event.type == UnmapNotify) {
         windows_moved=1;
      }
   }
   return windows_moved;
}

/* ASSIGNMENT FUNCTIONS */

/* Move a toon */
void ToonMove(Toon *toon, int xoffset, int yoffset)
{
   toon->x += xoffset;
   toon->y += yoffset;
   return;
}

/* Directly assign the position of a toon */
void ToonSetPosition(Toon *toon, int x, int y)
{
   toon->x=x;
   toon->y=y;
   return;
}

/* Change a toons type and activate it */
/* Gravity determines position offset of toon if size different from
 * previous type */
void ToonSetType(Toon *toon, int type, int direction, int gravity)
{
   switch(gravity) {
      case TOON_HERE:
         toon->x += (toon_data[toon->type].width-toon_data[type].width)/2;
         toon->y += (toon_data[toon->type].height-toon_data[type].height)/2;
         break;
      case TOON_DOWN:
         toon->x += (toon_data[toon->type].width-toon_data[type].width)/2;
         toon->y += (toon_data[toon->type].height-toon_data[type].height);
         break;
      case TOON_UP:
         toon->x += (toon_data[toon->type].width-toon_data[type].width)/2;
         break;
      case TOON_LEFT:
         toon->y += (toon_data[toon->type].height-toon_data[type].height)/2;
         break;
      case TOON_RIGHT:
         toon->x += (toon_data[toon->type].width-toon_data[type].width);
         toon->y += (toon_data[toon->type].height-toon_data[type].height)/2;
         break;
   }
   toon->type = type;
   toon->direction = direction;
   toon->frame = 0;
   toon->active = 1;
   return;
}

/* Set toon velocity */
void ToonSetVelocity(Toon *toon, int u, int v)
{
   toon->u=u;
   toon->v=v;
   return;
}

/* CORE FUNCTIONS */

/* Attempt to move a toon based on its velocity */
/* `mode' can be TOON_MOVE (move unless blocked), TOON_FORCE (move
   regardless) or TOON_STILL (test the move but don't actually do it) */
/* Returns TOON_BLOCKED if blocked, TOON_OK if unblocked, or 
   TOON_PARTIALMOVE if limited movement is possible */
int ToonAdvance(Toon *toon, int mode)
{
   int newx, newy;
   int new_zone;
   unsigned int width, height;
   int move_ahead = 1;
   int result = TOON_OK;

   if (mode == TOON_STILL) move_ahead = 0;

   width=toon_data[toon->type].width;
   height=toon_data[toon->type].height;

   newx = toon->x + toon->u;
   newy = toon->y + toon->v;

   if (edge_block) {
      if (newx < 0) {
         newx = 0;
         result=TOON_PARTIALMOVE;
      }
      else if (newx + toon_data[toon->type].width > display_width) {
         newx=display_width-toon_data[toon->type].width;
         result=TOON_PARTIALMOVE;
      }
      if (newy < 0 && edge_block != 2) {
         newy=0;
         result=TOON_PARTIALMOVE;
      }
      else if (newy + toon_data[toon->type].height > display_height) {
         newy=display_height-toon_data[toon->type].height;
         result=TOON_PARTIALMOVE;
      }
      if (newx == toon->x && newy == toon->y) {
         result=TOON_BLOCKED;
      }
   }

   /* Is new toon location fully/partially filled with windows? */
   new_zone = XRectInRegion(windows,newx,newy,width,height);
   if (new_zone != RectangleOut && mode == TOON_MOVE 
         && result != TOON_BLOCKED) {
      int tryx, tryy, step=1, u=newx-toon->x, v=newy-toon->y;
      result=TOON_BLOCKED;
      move_ahead=0;
      /* How far can we move the toon? */
      if ( abs(v) < abs(u) ) {
         if (newx>toon->x) step=-1;
         for (tryx = newx+step; tryx != (toon->x); tryx += step) {
            tryy=toon->y+((tryx-toon->x)*(v))/(u);
            if (XRectInRegion(windows,tryx,tryy,width,height) == RectangleOut) {
               newx=tryx;
               newy=tryy;
               result=TOON_PARTIALMOVE;
               move_ahead=1;
               break;
            }
         }
      }
      else {
         if (newy>toon->y) step=-1;
         for (tryy=newy+step;tryy!=(toon->y);tryy=tryy+step) {
            tryx=toon->x+((tryy-toon->y)*(u))/(v);
            if (XRectInRegion(windows,tryx,tryy,width,height) == RectangleOut) {
               newx=tryx;
               newy=tryy;
               result=TOON_PARTIALMOVE;
               move_ahead=1;
               break;
            }
         }
      }
   }
   if (move_ahead) {
      toon->x=newx;
      toon->y=newy;
      if ( (++(toon->frame)) >= toon_data[toon->type].nframes) {
         toon->frame = 0;
         if ( (toon_data[toon->type].conf) & TOON_NOCYCLE) 
            toon->active = 0;
      }
   }
   return result;
}

/* Build up an X-region corresponding to the location of the windows 
   that we don't want our toons to enter */
/* Returns 0 on success, 1 if windows moved again during the execution
   of this function */
int ToonLocateWindows() {
   Window dummy;
   XWindowAttributes attributes;
   int wx;
   XRectangle *window_rect;
   int x, y;
   unsigned int height, width;
   unsigned int oldnwindows;

   XRectangle *rects = NULL;
   int nrects, rectord, irect;

   XSetErrorHandler(_ToonXErrorHandler);

   /* Rebuild window region */
   XDestroyRegion(windows);
   windows = XCreateRegion();

   /* Get children of root */
   oldnwindows=nwindows;
   XQueryTree(display, root, &dummy, &dummy, &children, &nwindows);
   if (nwindows>oldnwindows) {
      if (windata) free(windata);
      if ((windata=calloc(nwindows, sizeof(_ToonWindowData))) == NULL) {
         fprintf(stderr,"Error: Out of memory\n");
         _ToonExitGracefully(1);
      }      
   }

   /* Add windows to region */
   for (wx=0; wx<nwindows; wx++) {
      error_value = 0;

      windata[wx].wid = children[wx];
      windata[wx].solid = 0;

      XGetWindowAttributes(display, children[wx], &attributes);
      if (error_value) continue;

      /* Popup? */
      if (!solid_popups && attributes.save_under) continue;

      if (attributes.map_state == IsViewable) {
         /* Geometry of the window, borders inclusive */

         x = attributes.x;
         y = attributes.y;
         width = attributes.width + 2*attributes.border_width;
         height = attributes.height + 2*attributes.border_width;

         /* Entirely offscreen? */
         if (x >= display_width) continue;
         if (y >= display_height) continue;
         if (y <= 0) continue;
         if ((x + width) < 0) continue;

         windata[wx].solid = 1;
         window_rect = &(windata[wx].pos);
         window_rect->x = x;
         window_rect->y = y;
         window_rect->height = height;
         window_rect->width = width;
         /* The area of the windows themselves */
         if (!shaped_windows) {
            XUnionRectWithRegion(window_rect, windows, windows);
         }
         else {
            rects = XShapeGetRectangles(display, children[wx], ShapeBounding,
                  &nrects, &rectord);
            if (nrects <= 1) {
               XUnionRectWithRegion(window_rect, windows, windows);
            }
            else {
               for (irect=0;irect<nrects;irect++) {
                  rects[irect].x += x;
                  rects[irect].y += y;
                  XUnionRectWithRegion(rects+irect, windows, windows);
               }
            }
            if ((rects) && (nrects > 0)) {
               XFree(rects);
            }
         }
      }
   }
   XFree(children);
   XSetErrorHandler((ErrorHandler *) NULL);
   return 0;
}

/* Wait for a specified number of microseconds */
int ToonSleep(unsigned long usecs) {
   struct timeval t;
   t.tv_usec = usecs%(unsigned long)1000000;
   t.tv_sec = usecs/(unsigned long)1000000;
   select(0, (void *)0, (void *)0, (void *)0, &t);
   return 0;
}


/* FINISHING UP */

/* Close link to X server and free client-side window information */
int ToonCloseDisplay()
{
   XDestroyRegion(windows);
   XClearWindow(display,root);
   XCloseDisplay(display);
   if (windata) {
      free(windata);
      windata=NULL;
   }
   return 0;
}

/* Clear root window, close display and exit  */
void _ToonExitGracefully(int sig)
{
   ToonConfigure(TOON_NOCATCHSIGNALS);
   ToonCloseDisplay();
   exit(0);
}


/* HANDLING TOON ASSOCIATIONS WITH MOVING WINDOWS */

/* Set a toons association direction - e.g. TOON_DOWN if the toon
   is walking along the tops the window, TOON_UNASSOCIATED if
   the toon is in free space */
void ToonSetAssociation(Toon *toon, int direction)
{
   toon->associate = direction;
   return;
}

void ToonSetMaximumRelocate(int up, int down, int left, int right) {
   max_relocate_up = up;
   max_relocate_down = down;
   max_relocate_left = left;
   max_relocate_right = right;
   return;
}

/* The first thing to be done when the windows move is to work out 
   which windows the associated toons were associated with just before
   the windows moved */
/* Currently this function always returns 0 */
int ToonCalculateAssociations(Toon *toon, int n)
{
   int i, wx;
   int x, y, width, height;

   for (i=0; i<n; i++) {
      if (toon[i].associate != TOON_UNASSOCIATED && toon[i].active) {
         /* determine the position of a line of pixels that the associated
            window should at least partially enclose */
         switch (toon[i].associate) {
            case TOON_DOWN:
               x = toon[i].x;
               y = toon[i].y + toon_data[toon->type].height;
               width = toon_data[toon->type].width;
               height = 1;
               break;
            case TOON_UP:
               x = toon[i].x;
               y = toon[i].y - 1;
               width = toon_data[toon->type].width;
               height = 1;
               break;
            case TOON_LEFT:
               x = toon[i].x - 1;
               y = toon[i].y;
               width = 1;
               height = toon_data[toon->type].height;
               break;
            case TOON_RIGHT:
               x = toon[i].x + toon_data[toon->type].width;
               y = toon[i].y;
               width = 1;
               height = toon_data[toon->type].height;
               break;
            default:
               fprintf(stderr,"Error: Illegal direction: %d\n",toon[i].associate);
               _ToonExitGracefully(1);
         }

         toon[i].wid = 0;
         for (wx=0; wx<nwindows; wx++) {
            if (windata[wx].solid &&
                  windata[wx].pos.x < x+width && 
                  windata[wx].pos.x + windata[wx].pos.width > x &&
                  windata[wx].pos.y < y+height &&
                  windata[wx].pos.y + windata[wx].pos.height > y) {
               toon[i].wid = windata[wx].wid;
               toon[i].xoffset = toon[i].x - windata[wx].pos.x;
               toon[i].yoffset = toon[i].y - windata[wx].pos.y;
               break;
            }
         }
      }
   }
   return 0;
}

/* After calling ToonLocateWindows() we relocate all the toons that were
   associated with particular windows */
/* Currently this function always returns 0 */
int ToonRelocateAssociated(Toon *toon, int n)
{
   int i, wx, dx, dy;
   for (i=0; i<n; i++) {
      if (toon[i].associate != TOON_UNASSOCIATED && toon[i].wid != 0
            && toon[i].active) {
         for (wx=0; wx<nwindows; wx++) {
            if (toon[i].wid == windata[wx].wid && windata[wx].solid) {
               dx = toon[i].xoffset + windata[wx].pos.x - toon[i].x;
               dy = toon[i].yoffset + windata[wx].pos.y - toon[i].y;
               if (dx < max_relocate_right && -dx < max_relocate_left
                     && dy < max_relocate_down && -dy < max_relocate_up) {
                  if (!ToonOffsetBlocked(toon+i, dx, dy)) {
                     toon[i].x += dx;
                     toon[i].y += dy;
                  }
               }
               break;
            }
         }
      } 
   }
   return 0;
}
