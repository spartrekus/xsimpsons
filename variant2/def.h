/* def.h - defines penguin image data for use by xpenguins
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

#define PENGUIN_FORWARD 0
#define PENGUIN_LEFTRIGHT 1
#define PENGUIN_LEFT 0
#define PENGUIN_RIGHT 1

#define PENGUIN_WALKER 0
#define PENGUIN_FALLER 1
#define PENGUIN_TUMBLER 2
#define PENGUIN_FLOATER 3
#define PENGUIN_CLIMBER 4
#define PENGUIN_BOMBER 5
#define PENGUIN_EXPLOSION 6
#define PENGUIN_TYPES 7

#define PENGUIN_DEFAULTWIDTH 30
#define PENGUIN_DEFAULTHEIGHT 30

#include "walker.xpm"
#include "faller.xpm"
#include "tumbler.xpm"
#include "floater.xpm"
#include "climber.xpm"
#include "bomber.xpm"
#include "explosion.xpm"

ToonData penguin_data[] = {
   { walker_xpm , 1, 2, 42, 61, TOON_DEFAULTS },
   { faller_xpm , 4, 1, 46, 62, TOON_DEFAULTS },
   { tumbler_xpm, 1, 1, 46, 62, TOON_DEFAULTS },
   { floater_xpm, 4, 1, 46, 62, TOON_DEFAULTS },
   { climber_xpm, 1, 2, 61, 43, TOON_DEFAULTS },
   { bomber_xpm , 1, 1, 64, 64, TOON_NOCYCLE },
   { explosion_xpm, 1, 1, 64, 64, TOON_NOCYCLE }
};
