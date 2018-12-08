/* toon.h - functions for drawing cartoons on the root window - used by xpenguins
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

#include <X11/Intrinsic.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <X11/xpm.h>
#include <X11/extensions/shape.h>

#define TOON_UNASSOCIATED -2
#define TOON_HERE -1
#define TOON_LEFT 0
#define TOON_RIGHT 1
#define TOON_UP 2
#define TOON_DOWN 3

#define TOON_FORCE 1
#define TOON_MOVE 0
#define TOON_STILL -1

#define TOON_OK 1
#define TOON_PARTIALMOVE 0
#define TOON_BLOCKED -1
#define TOON_SQUASHED -2

#define TOON_DEFAULTS 0L
#define TOON_NOEDGEBLOCK (1L<<0)
#define TOON_EDGEBLOCK (1L<<1)
#define TOON_SIDEBOTTOMBLOCK (1L<<2)
#define TOON_NOSOLIDPOPUPS (1L<<4)
#define TOON_SOLIDPOPUPS (1L<<5)
#define TOON_NOSHAPEDWINDOWS (1L<<6)
#define TOON_SHAPEDWINDOWS (1L<<7)

#define TOON_NOCYCLE (1L<<8)

#define TOON_NOCATCHSIGNALS (1L<<16)
#define TOON_CATCHSIGNALS (1L<<17)
#define TOON_EXITGRACEFULLY (1L<<18)

#define TOON_MESSAGE_LENGTH 64
#define TOON_DEFAULTMAXRELOCATE 8

/*** STRUCTURES ***/

typedef struct {
   char
      **image;
   int
      nframes,ndirections, /* number of frames and directions in image */
      width,height; /* width and height of an individual frame/direction */
   long int
      conf; /* bitmask of toon properties such as cycling etc. */
   Pixmap
      pixmap, mask; /* pointers to X structures */
} ToonData;


typedef struct {
   int
      x,y,u,v, /* new position and velocity */
      active,type,frame,direction,
      x_map,y_map,width_map,height_map,
            /* properties of the image mapped on the screen */
      associate, /* toon is associated with a window */
      xoffset, yoffset; /* location relative to window origin */
   unsigned int wid; /* window associated with */   
} Toon;

/*** FUNCTION PROTOTYPES ***/

/* SIGNAL AND ERROR HANDLING FUNCTIONS */
int ToonSignal();
char *ToonErrorMessage();

/* STARTUP FUNCTIONS */
Display *ToonOpenDisplay(char *display_name);
int ToonConfigure(unsigned long int code);
int ToonInstallData(ToonData *toon_data, int n);

/* DRAWING FUNCTIONS */
int ToonDraw(Toon *toon,int n);
int ToonErase(Toon *toon,int n);
void ToonFlush();

/* QUERY FUNCTIONS */
int ToonDisplayWidth();
int ToonDisplayHeight();
int ToonBlocked(Toon *toon, int direction);
int ToonOffsetBlocked(Toon *toon, int xoffset, int yoffset);
int ToonWindowsMoved();

/* ASSIGNMENT FUNCTIONS */
void ToonMove(Toon *toon, int xoffset, int yoffset);
void ToonSetPosition(Toon *toon, int x, int y);
void ToonSetType(Toon *toon, int type, int direction, int gravity);
void ToonSetVelocity(Toon *toon, int u, int v);

/* CORE FUNCTIONS */
int ToonAdvance(Toon *toon, int mode);
int ToonLocateWindows();
int ToonSleep(unsigned long usecs);

/* FINISHING UP */
int ToonCloseDisplay();

/* HANDLING TOON ASSOCIATIONS WITH MOVING WINDOWS */
void ToonSetAssociation(Toon *toon, int direction);
void ToonSetMaximumRelocate(int up, int down, int left, int right);
int ToonRelocateAssociated(Toon *toon, int n);
int ToonCalculateAssociations(Toon *toon, int n);

