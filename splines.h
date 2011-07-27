/*************************************************************************
 *                                                                       *
 * Copyright (c) 1992, 1993 Ronald Joe Record                           *
 *                                                                       *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *************************************************************************/

/*
 *	@(#) splines.h 1.2 93/10/08 MRINC
 */
/*
 *	Written by Ron Record (rr@ronrecord.com) 07 Apr 1993.
 */

#include "patchlevel.h"
#include "libXrr.h"
#include <math.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <values.h>
#include <X11/Xlib.h> 
#include <X11/StringDefs.h> 
#include <X11/keysym.h> 
#include <X11/cursorfont.h> 
#include <X11/Xutil.h>
#include <X11/Xatom.h>

#define ABS(a)	(((a)<0) ? (0-(a)) : (a) )
#define Min(x,y) ((x < y)?x:y)
#define Max(x,y) ((x > y)?x:y)
#define MAX_ATTEMPTS 100

/* Useful mathematical constants that should have been defined in math.h 
 * M_LOG2E	- log2(e)
 * M_LN2        - ln(2)
 * M_PI		- pi
 */
#ifndef M_LOG2E
#define M_LOG2E	1.4426950408889634074
#endif
#ifndef M_PI
#define M_PI	3.14159265358979323846
#endif
#ifndef M_2PI
#define M_2PI      6.2831853071795862320E0  /*Hex  2^ 2 * 1.921FB54442D18 */
#endif
#ifndef M_LN2
#define M_LN2      6.9314718055994530942E-1 /*Hex  2^-1 * 1.62E42FEFA39EF */
#endif

/* Useful machine-dependent values that should have been defined in values.h
 * LN_MAXDOUBLE	- the natural log of the largest double  -- log(MAXDOUBLE)
 * LN_MINDOUBLE	- the natural log of the smallest double -- log(MINDOUBLE)
 */
#ifndef LN_MINDOUBLE
#define LN_MINDOUBLE (M_LN2 * (DMINEXP - 1))
#endif
#ifndef LN_MAXDOUBLE
#define LN_MAXDOUBLE (M_LN2 * DMAXEXP)
#endif

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif

int STARTCOLOR=16;
int INDEXINC=20;
int mincolindex=41;

int screen;
int numwheels = MAXWHEELS;
Display*	dpy;

extern double fabs();
extern long time();
extern int optind;
extern char *optarg;

Window canvas, help;

GC Data_GC[2][MAXCOLOR];

Pixmap  pixmap;
XColor	Colors[MAXCOLOR];
Colormap cmap;
