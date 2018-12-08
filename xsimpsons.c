/* XPENGUINS - cool little penguins that walk along the tops of your windows
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
 *
 *
 * XPENGUINS is a kind of a cross between lemmings and xsnow - penguins
 * fall from the top of the screen and walk across the tops of windows.
 * The images are take from `Pingus', a lemmings-type game for Linux.
 * All the nasty X calls are in `toon.c', so xpenguins.c should be easily
 * modifiable to animate any small furry (or feathery) animal of your choice.
 * 
 * Robin Hogan <R.J.Hogan@reading.ac.uk>
 * Project started 22 November 1999
 * Last update 26 February 2000
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

/* C structures defined here */
#include "toon.h"

/*
 * The penguin shapes are defined here - includes links to all the xpm images.
 * If you want to have something else falling from the top of your screen (the
 * possibilities are endless!) then point the following line somewhere else. 
 */
#include "penguins/def.h"

#define MAX_PENGUINS 256
#define DEFAULT_DELAY 50
#define JUMP_DISTANCE 8
#define RandInt(maxint) ((int) ((maxint)*((float) rand()/(RAND_MAX+1.0))))

#define XPENGUINS_VERSION "1.2"
#define XPENGUINS_AUTHOR "Robin Hogan"
#define XPENGUINS_DATE "22 May 2000"

/* A few macros... */

#define InitPenguin(penguin) \
   ToonSetType(penguin, PENGUIN_FALLER, PENGUIN_FORWARD, \
         TOON_UNASSOCIATED); \
   ToonSetPosition(penguin, RandInt(ToonDisplayWidth() \
         - PENGUIN_DEFAULTWIDTH), 1-PENGUIN_DEFAULTHEIGHT); \
   ToonSetAssociation(penguin, TOON_UNASSOCIATED); \
   ToonSetVelocity(penguin, RandInt(2)*2-1, 3)

#define MakeClimber(penguin) \
   ToonSetType(penguin, PENGUIN_CLIMBER, (penguin)->direction, \
         TOON_DOWN); \
   ToonSetAssociation(penguin, (penguin)->direction); \
   ToonSetVelocity((penguin),0,-4)

#define MakeWalker(penguin) \
   ToonSetType(penguin, PENGUIN_WALKER, \
         (penguin)->direction,TOON_DOWN); \
   ToonSetAssociation(penguin, TOON_DOWN); \
   ToonSetVelocity(penguin, 4*((2*(penguin)->direction)-1), 0)

#define MakeFaller(penguin) \
   ToonSetVelocity(penguin, ((penguin)->direction)*2-1, 3); \
   ToonSetType(penguin, PENGUIN_FALLER, \
         PENGUIN_FORWARD,TOON_DOWN); \
   ToonSetAssociation(penguin, TOON_UNASSOCIATED)

void ShowUsage(char **argv) {
   fprintf(stdout,"Usage: %s [options]\n",argv[0]);
   fprintf(stdout,"Options:\n");
   fprintf(stdout,"  -display <display>        Send the penguins to <display>'\n");
   fprintf(stdout,"  -delay <millisecs>        Set delay between frames (default %d)\n",
         DEFAULT_DELAY);
   fprintf(stdout,"  -n, -penguins <n>         Create <n> penguins (max %d)\n",
         MAX_PENGUINS);
   fprintf(stdout,"  -ignorepopups             Penguins ignore `popup' windows\n");
   fprintf(stdout,"  -rectwin                  Regard shaped windows as rectangular\n");
   fprintf(stdout,"  -q, -quiet, --quiet       Suppress message on exit\n");
   fprintf(stdout,"  -v, -version, --version   Show version information\n");
   fprintf(stdout,"  -h, -?, -help, --help     Show this message\n");
};


/* Global variables */
int finished=0;
int new_positions=0;
int verbose=1;

/*** MAIN PROGRAM ***/
int main (int argc, char **argv) {
   unsigned long sleep_usec=DEFAULT_DELAY*1000;
   unsigned long configure_mask = TOON_SIDEBOTTOMBLOCK 
         | TOON_CATCHSIGNALS;
   char *display_name=NULL;
   int windows_moved,status,npenguins=8,i,n,direction;
   Toon penguin[MAX_PENGUINS];
   char prefd[MAX_PENGUINS]; /* preferred direction; -1 means none */
   char prefclimb[MAX_PENGUINS]; /* climbs when possible */
   char hold_on[MAX_PENGUINS];
   /* Handle command-line arguments */
   for (n=1;n<argc;n++) {
      if (strcmp(argv[n],"-n") == 0 || strcmp(argv[n],"-penguins") == 0) {
         if (argc > ++n) {
            npenguins=atoi(argv[n]);
            if (npenguins > MAX_PENGUINS) {
               fprintf(stderr,"Warning: only %d penguins created\n",
                     npenguins=MAX_PENGUINS);
            }
            else if (npenguins <= 0) {
               fprintf(stderr,"Warning: no penguins created\n");
                     npenguins=0;
            }
         }
         else {
            fprintf(stderr,"Error: number of penguins not specified\n");
            ShowUsage(argv);
            exit(1);
         }
      }
      else if (strcmp(argv[n],"-delay") == 0) {
         if (argc > ++n) {
            sleep_usec=1000*atoi(argv[n]);
         }
         else {
            fprintf(stderr,"Error: delay in milliseconds not specified\n");
            ShowUsage(argv);
            exit(1);
         }
      }
      else if (strcmp(argv[n],"-display") == 0) {
         if (argc > ++n) {
            display_name=argv[n];
         }
         else {
            fprintf(stderr,"Error: display not specified\n");
            ShowUsage(argv);
            exit(1);
         }
      }
      else if (strcmp(argv[n],"-ignorepopups") == 0 ) {
         configure_mask |= TOON_NOSOLIDPOPUPS;
      }
      else if (strcmp(argv[n],"-rectwin") == 0 ) {
         configure_mask |= TOON_NOSHAPEDWINDOWS;
      }
      else if (strcmp(argv[n],"-h") == 0 || strcmp(argv[n],"-help") == 0 || 
               strcmp(argv[n],"-?") == 0 || strcmp(argv[n],"--help") == 0) {
         fprintf(stdout,"XPenguins %s (%s) by %s\n",
               XPENGUINS_VERSION, XPENGUINS_DATE, XPENGUINS_AUTHOR);
         ShowUsage(argv);
         exit(0);
      }
      else if (strcmp(argv[n],"-v") == 0 || strcmp(argv[n],"-version") == 0 || 
               strcmp(argv[n],"--version") == 0) {
         fprintf(stdout,"XPenguins %s\n", XPENGUINS_VERSION);
         exit(0);
      }
      else if (strcmp(argv[n],"-q") == 0 || strcmp(argv[n],"-quiet") == 0 || 
               strcmp(argv[n],"--quiet") == 0) {
         verbose=0;
      }
      else {
         fprintf(stderr,"Warning: `%s' not understood - use `-h' for a list of options\n",argv[n]);
      }
   }

   /* reset random-number generator */
   srand(time((long *) NULL));

   /* contact X server and set up some basic X stuff */
   if (ToonOpenDisplay(display_name) == NULL) {
      fprintf(stderr,"Error: %s\n", ToonErrorMessage());
      exit(1);
   };
   /* Set up various preferences: Edge of screen is solid, and if a signal is caught
    * then exit the main event loop */
   ToonConfigure(configure_mask);
   /* Set the distance the window can move (up, down, left, right) and penguin
    * can still cling on */
   ToonSetMaximumRelocate(16,16,16,16);
   /* Send the pixmaps to the X server  - penguin_data should have been 
    * defined in penguins/def.h */
   ToonInstallData(penguin_data,PENGUIN_TYPES);

   /* initialise penguins */
   for (i=0;i<npenguins;i++) {
      prefd[i] = -1;
      prefclimb[i] = 0;
      hold_on[i] = 0;
      InitPenguin(penguin+i);
   }

   /* Find out where the windows are - should be done just before beginning the 
    * event loop */
   ToonLocateWindows();
   /* Event loop */
   while (!finished) {
      /* check if windows have moved, and flush the display */
      if ( (windows_moved = ToonWindowsMoved()) ) {
         /* if so, check for squashed toons */
         ToonCalculateAssociations(penguin,npenguins);
         windows_moved = ToonLocateWindows();
         ToonRelocateAssociated(penguin,npenguins);
      }
      for (i=0;i<npenguins;i++) {
         if (!penguin[i].active) {
            InitPenguin(penguin+i);
            continue;
         }
         else {
            if (ToonBlocked(penguin+i,TOON_HERE)) {
               ToonSetType(penguin+i,PENGUIN_EXPLOSION,
                     PENGUIN_FORWARD,TOON_HERE);
               ToonSetAssociation(penguin+i, TOON_UNASSOCIATED);
            }

            status=ToonAdvance(penguin+i,TOON_MOVE);
            switch (penguin[i].type) {
               case PENGUIN_FALLER:
                  if (status != TOON_OK) {
                     if (ToonBlocked(penguin+i,TOON_DOWN)) {
                        if (prefd[i]>-1)
                           penguin[i].direction=prefd[i];
                        else
                           penguin[i].direction=RandInt(2);
                        MakeWalker(penguin+i);
                        prefd[i]=-1;
                     }
                     else {
                        if (RandInt(2)) {
                           ToonSetVelocity(penguin+i,-penguin[i].u,3);
                        }
                        else {
                           penguin[i].direction = penguin[i].u>0;
                           MakeClimber(penguin+i);
                        }
                     }
                  }
                  break;

               case PENGUIN_TUMBLER:
                  if (status != TOON_OK) {
                     if (prefd[i]>-1)
                        penguin[i].direction=prefd[i];
                     else
                        penguin[i].direction=RandInt(2);
                     MakeWalker(penguin+i);
                     prefd[i]=-1;
                  }
                  else if (penguin[i].v < 8) {
                     penguin[i].v +=1;
                  }
                  break;

               case PENGUIN_WALKER:
                  if (status != TOON_OK) {
                     if (status == TOON_BLOCKED) {
                        /* Try to step up... */
                        int u = penguin[i].u;
                        if (!ToonOffsetBlocked(penguin+i, u, -JUMP_DISTANCE)) {
                           ToonMove(penguin+i, u, -JUMP_DISTANCE);
                           ToonSetVelocity(penguin+i, 0, JUMP_DISTANCE-1);
                           ToonAdvance(penguin+i, TOON_MOVE);
                           ToonSetVelocity(penguin+i, u, 0);
                        }
                        else {
                           /* Blocked! We can turn round, fly or climb... */
                           switch (RandInt(8)*(1-prefclimb[i])) {
                              case 0:
                                 MakeClimber(penguin+i);
                                 break;
                              case 1:
                                 ToonSetType(penguin+i,PENGUIN_FLOATER,
                                       PENGUIN_FORWARD,TOON_DOWN);
                                 ToonSetAssociation(penguin+i, TOON_UNASSOCIATED);
                                 ToonSetVelocity(penguin+i,RandInt(5)
                                       * (-penguin[i].u/4),-3);
                                 break;
                              default:
                                 penguin[i].direction = (!penguin[i].direction);
                                 MakeWalker(penguin+i);
                           }
                        }
                     }
                  }
                  else if (!ToonBlocked(penguin+i,TOON_DOWN)) {
                     /* Try to step down... */
                     ToonSetVelocity(penguin+i, 0, JUMP_DISTANCE);
                     status=ToonAdvance(penguin+i,TOON_MOVE);
                     if (status == TOON_OK) {
                        prefd[i]=penguin[i].direction;
                        ToonSetType(penguin+i, PENGUIN_TUMBLER,
                              PENGUIN_FORWARD,TOON_DOWN);
                        ToonSetAssociation(penguin+i, TOON_UNASSOCIATED);
                        ToonSetVelocity(penguin+i, 0, 1);
                        prefclimb[i]=0;
                     }
                     else {
                        ToonSetVelocity(penguin+i, 4*((2*penguin[i].direction)-1), 0);
                     }
                  }
                  break;

               case PENGUIN_CLIMBER:
                  direction = penguin[i].direction;
                  if (penguin[i].y < 0) {
                     penguin[i].direction = (!direction);
                     MakeFaller(penguin+i);
                     prefclimb[i]=0;
                  }
                  else if (status == TOON_BLOCKED) {
                     /* Try to step out... */
                     int v = penguin[i].v;
                     int xoffset = (1-direction*2) * JUMP_DISTANCE;
                     if (!ToonOffsetBlocked(penguin+i, xoffset, v)) {
                        ToonMove(penguin+i, xoffset, v);
                        ToonSetVelocity(penguin+i, -xoffset-(1-direction*2), 0);
                        ToonAdvance(penguin+i, TOON_MOVE);
                        ToonSetVelocity(penguin+i, 0, v);
                     }
                     else {
                        penguin[i].direction = (!direction);
                        MakeFaller(penguin+i);
                        prefclimb[i]=0;
                     }
                  }
                  else if (!ToonBlocked(penguin+i,direction)) {
                     if (ToonOffsetBlocked(penguin+i, ((2*direction)-1)
                           * JUMP_DISTANCE, 0)) {
                        ToonSetVelocity(penguin+i, ((2*direction)-1)
                           * (JUMP_DISTANCE-1), 0);
                        ToonAdvance(penguin+i, TOON_MOVE);
                        ToonSetVelocity(penguin+i, 0, -4);
                     }
                     else {
                        MakeWalker(penguin+i);
                        ToonSetPosition(penguin+i, penguin[i].x+(2*direction)-1,
                              penguin[i].y);
                        prefd[i]=direction;
                        prefclimb[i]=1;
                     }
                  }
                  break;

               case PENGUIN_FLOATER:
                  if (penguin[i].y < 0) {
                     penguin[i].direction = (penguin[i].u>0);
                     MakeFaller(penguin+i);
                  }
                  else if (status != TOON_OK) {
                     if (ToonBlocked(penguin+i,TOON_UP)) {
                       penguin[i].direction = (penguin[i].u>0);
                       MakeFaller(penguin+i);
                     }
                     else {
                        ToonSetVelocity(penguin+i,-penguin[i].u, -3);
                     }
                  }
                  break;

               case PENGUIN_EXPLOSION:
                  if (!hold_on[i]) {
                     hold_on[i] = 1;
                  }
                  else {
                     penguin[i].active=0;
                     hold_on[i]=0;
                  }
             }
         }
      }
      /* First erase them all, then draw them all - should reduce flickering */
      ToonErase(penguin,npenguins);
      ToonDraw(penguin,npenguins);
      ToonFlush();
      /* pause */
      ToonSleep(sleep_usec);
      /* Has an interupt signal been received? If so, quit gracefully */
      finished=ToonSignal();
   }

   /* Exit sequence... */
   /* Any more signals (TERM, HUP or INT) and the penguins are 
    * erased immediately */
   ToonConfigure(TOON_EXITGRACEFULLY);
   if (1) {
      /* Nice exit sequence... */
      if (verbose) fprintf(stderr,"Interupt received: exploding penguins");
      for (i=0;i<npenguins;i++) {
         if (penguin[i].active) {
            ToonSetType(penguin+i,PENGUIN_BOMBER,
                  PENGUIN_FORWARD,TOON_DOWN);
         }
      }
      for (n=0;n<penguin_data[PENGUIN_BOMBER].nframes;n++) {
         ToonErase(penguin,npenguins);
         ToonDraw(penguin,npenguins);
         ToonFlush();
         for (i=0;i<npenguins;i++) {
            ToonAdvance(penguin+i,TOON_FORCE);
         }
         ToonSleep(sleep_usec);
         if (verbose) fprintf(stderr,".");
      }
      if (verbose) fprintf(stderr,"done\n");
   }
   ToonErase(penguin,npenguins);
   ToonCloseDisplay();
   exit(0);
}

