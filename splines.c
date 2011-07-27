/*
 *  Written using CGI 5 October 1987 by Ron Record
 *  Rewritten using X11 20 Apr 1993 by Ron Record (rr@ronrecord.com)
 */
/*************************************************************************
 *                                                                       *
 * Copyright (c) 1987-1993 Ronald Joe Record                            *
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


#include "splines.h"

#define BMAX 32
#define M_ESIZ 512
#define M_X 12
#define M_Y 8
#define M_DELTA 0.05
#define MAX_CINC 8

typedef struct {
	int x,y;
	int xdir, ydir;
	int color;
} BALL;

BALL balls[BMAX];
int xradius=5, yradius=4, erase_balls=1, erase_curve=0, c_color, ch_wheel=1;
int lines=0, curve=1, nsteps=0, showballs=0, nballs=4, nsplines=1, e_color;
int naptime=0, stopping=1, now_c=0, delay_erase=1, esiz=64, s_order=3;
int nummaps=1, delay=0, bezier=0, spinlines=0, solospin=0;
int useroot=0, Fflag=0, oflag=0, ranwalk=0, ranflag=0, spin=0, xor=0;
int width=0, height=0, maxcolor, numfreecols, usedefault=0;
int ***ctrl;
long limit=0, num_c=1;
double *theta;
int *radius;
char *outname;

/* routines declared in this file */
void event_loop(), usage(), init_contexts(), drawball(), redraw(), Clear();
void init_ctrl(), init_pts(), print_help(), drawspline(), ranparams();
void redisplay(), resize(), save(), move(), parseargs(), Getkey(); 
void freemem(), setupmem();
/* external routines used in this file */
extern int rand();
extern void Draw_spline(), Draw_bez();

void
usage()
{
    printf("Usage: splines [-abefrsuxBEFIRW][-w width][-h height][-o file]\n");
	printf("\t[-m index][-c colorwheel][-n count][-N nballs][-l limit]\n");
	printf("\t[-p nsteps][-D delay][-S naptime][-X xradius][-Y yradius]\n");
    printf("\t-a indicates display control points only\n");
    printf("\t-B indicates Bezier rather than B-spline\n");
    printf("\t-b indicates display cubic curves only\n");
    printf("\t-c selects color wheel\n");
    printf("\t-D specifies the spin delay\n");
    printf("\t-E indicates erasure of previous control points\n");
    printf("\t-e indicates erasure of previous curve\n");
    printf("\t-f indicates display control points as circles\n");
    printf("\t-I indicates infinite mode\n");
    printf("\t-l sets the limit of number of curves to be calculated\n");
    printf("\t-N sets the number of control points\n");
    printf("\t-n sets the limit of number of sequences of curves\n");
    printf("\t-p sets the number of steps used in spline calculation\n");
    printf("\t-R indicates use of the root window\n");
    printf("\t-r indicates random selection of parameters\n");
    printf("\t-S sets the delay between curves\n");
    printf("\t-s indicates spin color wheel when done computing\n");
    printf("\t-R indicates use the root window\n");
    printf("\t-m # indicates a minimum color index of # (0-255)\n");
    printf("\t-o file will save the output as 'file' in PPM format\n");
    printf("\t-W specifies a pseudo-random walk of the control points\n");
    printf("\t-w # indicates a window width of #\n");
    printf("\t-X sets the X radius of control point disks\n");
    printf("\t-x indicates use of XOR rather than COPY mode\n");
    printf("\t-Y sets the Y radius of control point disks\n");
    printf("\t-h # indicates a window height of #\n");
    printf("\t-u produces this message\n");
    printf("During display :\n");
    printf("\t'f' or 'F' will save the picture as a PPM file\n");
    printf("\t'r' or 's' will spin the color wheel forwards or backwards\n");
    printf("\t'W' will increment and 'w' decrement the color map selection\n");
    printf("\t'?' or 'h' will display the usage message\n");
    printf("\t'q' or 'Q' will quit\n");
}

void
freemem()
{
	static int i, j;

	for (i=0;i<M_ESIZ;i++) {
		for (j=0;j<nballs;j++)
			free(ctrl[i][j]);
		free(ctrl[i]);
	}
	free(ctrl);
	free(theta);
	free(radius);
}

void
setupmem()
{
	static int i, j;

	if ((ctrl = (int ***)malloc((unsigned)((M_ESIZ+1)*sizeof(int **)))) == NULL) {
		fprintf(stderr, "Error malloc'ing ctrl\n");
		exit(-1);
	}
	if ((theta = (double *)malloc((unsigned)M_ESIZ*sizeof(double))) == NULL) {
		fprintf(stderr, "Error malloc'ing theta\n");
		exit(-1);
	}
	if ((radius = (int *)malloc((unsigned)M_ESIZ*sizeof(int))) == NULL) {
		fprintf(stderr, "Error malloc'ing radius\n");
		exit(-1);
	}
	for (i=0;i<M_ESIZ;i++) {
		if ((ctrl[i]=(int **)malloc((unsigned)((nballs+1)*sizeof(int *))))==NULL){
			fprintf(stderr, "Error malloc'ing ctrl[%d]\n",i);
			exit(-1);
		}
		for (j=0;j<nballs;j++) {
			if ((ctrl[i][j] = (int *)malloc((unsigned)(2*sizeof(int))))==NULL) {
				fprintf(stderr, "Error malloc'ing ctrl[%d][%d]\n",i,j);
				exit(-1);
			}
		}
	}
}

void
drawspline(col, n)
int col, n;
{
	static int i, j, k, m;
	XSegment points[BMAX];
	extern XSegment clipline();
	extern double sin(), cos();

	if (lines) {
		for (i=0; i<(nballs/2); i++) {
			j = 2*i;
			points[i].x1 = ctrl[n][j][0];
			points[i].y1 = ctrl[n][j][1];
			points[i].x2 = ctrl[n][j+1][0];
			points[i].y2 = ctrl[n][j+1][1];
		}
		XDrawSegments(dpy,pixmap,Data_GC[xor][col],points,nballs/2);
		XDrawSegments(dpy,canvas,Data_GC[xor][col],points,nballs/2);
	}
	else if (spinlines) {
		m = solospin ? 1 : nballs;
		for (i=0; i<m; i++) {
			j = radius[n] * sin(theta[n]);
			k = radius[n] * cos(theta[n]);
			points[i].x1 = j + ctrl[n][i][0];
			points[i].y1 = k + ctrl[n][i][1];
			points[i].x2 = ctrl[n][i][0] - j;
			points[i].y2 = ctrl[n][i][1] - k;
			points[i] = clipline(points[i],0,width,0,height);
		}
		XDrawSegments(dpy,pixmap,Data_GC[xor][col],points,m);
		XDrawSegments(dpy,canvas,Data_GC[xor][col],points,m);
		theta[(n+1)%esiz] = theta[n] + M_DELTA;
		if (theta[(n+1)%esiz] > M_2PI)
			theta[(n+1)%esiz] = theta[(n+1)%esiz] - M_2PI;
		if (n < (esiz/2))
			radius[(n+1)%esiz] = Max(radius[n] + 1, 1);
		else
			radius[(n+1)%esiz] = Max(radius[n] - 1, 1);
	}
	else {
		j = (nballs / nsplines);
		for (i=0; i<nsplines; i++) {
			if (bezier)
				Draw_bez(dpy,canvas,pixmap,Data_GC[xor][col],
							&ctrl[n][i*j],j-1,nsteps);
			else
				Draw_spline(dpy,canvas,pixmap,Data_GC[xor][col],
							&ctrl[n][i*j],j-1,nsteps,s_order);
		}
	}
}

void
ranparams()
{
	delay_erase = rand() % 2;
	curve = rand() % 2;
	showballs = rand() % 2;
	if ((!curve) && (!showballs))
		if (rand() % 2)
			curve = 1;
		else
			showballs = 1;
	if (delay_erase && curve) {
		showballs = xor = 0;
	}
	else {
		xor = rand() % 2;
	}
	erase_curve = rand() % 2;
	erase_balls = rand() % 2;
	spin = rand() % 2;
	Fflag = rand() % 2;
	bezier = rand() % 2;
	lines = (!(rand() % 6));
	spinlines = (!(rand() % 6));
	nballs = rand() % BMAX;
	if (nballs < 3)
		nballs = 3;
	nsplines = (rand() % (nballs/3)) + 1;
	if (nsplines < 1)
		nsplines = 1;
	if (nsplines > (nballs/3))
		nsplines = nballs/3;
	/* work-around bug whereby if nballs>13 per Bezier spline, */
	/* the spline is always anchored at the origin */
	if (bezier && ((nballs/nsplines) > 13))
		bezier = 0;
	nsteps = ((nballs/nsplines) * 2) + (rand() % 50);
	if (rand() % 4)
		naptime = 0;
	else
		naptime = rand() % (450 / nballs);
	numwheels = rand() % (MAXWHEELS + 1);
}

void
init_contexts()
{
    static int i, j;
	XGCValues values;

    /*
     * create default, writable, graphics contexts for the canvas.
	 * the first index indicates whether the context draws in XOR mode.
	 * the second index indicates what the foreground color is.
     */
	values.background = BlackPixel(dpy, screen);
	for (j=0; j<2; j++) {
		if (j)
			values.function = GXxor;
		else
			values.function = GXcopy;
    	Data_GC[j][0] = XCreateGC(dpy, DefaultRootWindow(dpy),
        	(unsigned long) NULL, (XGCValues *) NULL);
    	/* set the background to black */
    	XSetBackground(dpy,Data_GC[j][0],BlackPixel(dpy, screen));
    	/* set the foreground of the 0th context to black */
    	XSetForeground(dpy, Data_GC[j][0], BlackPixel(dpy, screen));
    	Data_GC[j][1] = XCreateGC(dpy, DefaultRootWindow(dpy),
        	(unsigned long) NULL, (XGCValues *) NULL);
    	/* set the background to black */
    	XSetBackground(dpy,Data_GC[j][1],BlackPixel(dpy, screen));
    	/* set the foreground of the 1st context to white */
    	XSetForeground(dpy, Data_GC[j][1], WhitePixel(dpy,  screen));
    	for (i=2; i<maxcolor; i++) {
			values.foreground = i;
        	Data_GC[j][i] = XCreateGC(dpy, DefaultRootWindow(dpy),
							GCForeground | GCBackground | GCFunction, &values);
    	}
	}
}
	
void
Clear()
{
    XFillRectangle(dpy, pixmap, Data_GC[0][0], 0, 0, width, height);
    XCopyArea(dpy, pixmap, canvas, Data_GC[0][0], 0, 0, width, height, 0, 0);
}

void 
init_ball(k)
int k;
{
	balls[k].x = (int)((rand()%(width-2))+1);
	balls[k].y = (int)((rand()%(height-2))+1);
	balls[k].xdir = (rand()&0xff) > 128 ? rand()%M_X+1 :-(rand()%M_X+1);
	balls[k].ydir = (rand()&0xff) > 128 ? rand()%M_Y+1 :-(rand()%M_Y+1);
	balls[k].color = (rand() % numfreecols) + STARTCOLOR;
}

void
init_ctrl()
{
	int i;

	for (i=0;i<nballs;i++) {
		ctrl[now_c][i][0] = balls[i].x;
		ctrl[now_c][i][1] = balls[i].y;
	}
}

void
init_pts()
{
	int i;

	for (i=0;i<nballs;i++) {
		init_ball(i);
		if (showballs)
			drawball(i, balls[i].color);
	}
}

initialize()
{
    Clear();
	num_c = 1;
	now_c = 0;
	theta[0] = 0.0;
	radius[0] = 1;
    init_pts();
	init_ctrl();
	c_color = STARTCOLOR;
	if (xor)
		e_color = c_color;
	else
		e_color = 0;
	if (curve)	/* Draw the initial spline curve */
		drawspline(c_color, now_c);
}

#define x_str 10

void
print_help() 
{
    static char str[80];
    static int y_str, spacing;
    static int ascent, descent, dir;
    static XCharStruct overall;
    static GC gc;

    gc = Data_GC[0][1];
    XClearWindow(dpy, help);
    y_str = 20;
    sprintf(str,"During run-time, interactive control can be exerted via : ");
    XDrawImageString(dpy,help,gc,x_str,y_str,str,strlen(str));
    XQueryTextExtents(dpy,(XID)XGContextFromGC(gc),"Hey!",
			4,&dir,&ascent,&descent,&overall);
    spacing = ascent + descent + 5;
    y_str += spacing;
    sprintf(str,"        < decreases delay between curves, > increases it");
    XDrawImageString(dpy,help,gc,x_str,y_str,str,strlen(str));
    y_str += spacing;
    sprintf(str,"        - lowers the value of mincolindex, + raises it");
    XDrawImageString(dpy,help,gc,x_str,y_str,str,strlen(str));
    y_str += spacing;
    sprintf(str,"        b or B toggles between Bezier and B-spline");
    XDrawImageString(dpy,help,gc,x_str,y_str,str,strlen(str));
    y_str += spacing;
    sprintf(str,"        c or C clears the window or creates a new control pt");
    XDrawImageString(dpy,help,gc,x_str,y_str,str,strlen(str));
    y_str += spacing;
    sprintf(str,"        d or D decreases or increases the spin delay");
    XDrawImageString(dpy,help,gc,x_str,y_str,str,strlen(str));
    y_str += spacing;
    sprintf(str,"        e or E toggles erasing of curves or control pts");
    XDrawImageString(dpy,help,gc,x_str,y_str,str,strlen(str));
    y_str += spacing;
    sprintf(str,"        f or F saves splines to a PPM file");
    XDrawImageString(dpy,help,gc,x_str,y_str,str,strlen(str));
    y_str += spacing;
    sprintf(str,"        g or G toggles display of ctrl pts as disks");
    XDrawImageString(dpy,help,gc,x_str,y_str,str,strlen(str));
    y_str += spacing;
    sprintf(str,"        h or H or ? displays this message");
    XDrawImageString(dpy,help,gc,x_str,y_str,str,strlen(str));
    y_str += spacing;
    sprintf(str,"        i or I toggles infinite mode");
    XDrawImageString(dpy,help,gc,x_str,y_str,str,strlen(str));
    y_str += spacing;
    sprintf(str,"        k or K toggles display of control pts or curves");
    XDrawImageString(dpy,help,gc,x_str,y_str,str,strlen(str));
    y_str += spacing;
    sprintf(str,"        L deletes a control point");
    XDrawImageString(dpy,help,gc,x_str,y_str,str,strlen(str));
    y_str += spacing;
    sprintf(str,"        n goes on to the next splines");
    XDrawImageString(dpy,help,gc,x_str,y_str,str,strlen(str));
    y_str += spacing;
    sprintf(str,"        N creates a new replacement splines");
    XDrawImageString(dpy,help,gc,x_str,y_str,str,strlen(str));
    y_str += spacing;
    sprintf(str,"        p or P decreases or increases the number of steps");
    XDrawImageString(dpy,help,gc,x_str,y_str,str,strlen(str));
    y_str += spacing;
    sprintf(str,"        R toggles a random walk of the control points");
    XDrawImageString(dpy,help,gc,x_str,y_str,str,strlen(str));
    y_str += spacing;
    sprintf(str,"        S stops the spinning of the color wheel");
    XDrawImageString(dpy,help,gc,x_str,y_str,str,strlen(str));
    y_str += spacing;
    sprintf(str,"        r or s spins the colorwheel");
    XDrawImageString(dpy,help,gc,x_str,y_str,str,strlen(str));
    y_str += spacing;
    sprintf(str,"        w decrements, W increments the color wheel index");
    XDrawImageString(dpy,help,gc,x_str,y_str,str,strlen(str));
    y_str += spacing;
    sprintf(str,"        X creates an additional control point");
    XDrawImageString(dpy,help,gc,x_str,y_str,str,strlen(str));
    y_str += spacing;
    sprintf(str,"        x toggles between XOR and COPY mode");
    XDrawImageString(dpy,help,gc,x_str,y_str,str,strlen(str));
    y_str += spacing;
    sprintf(str,"        q or Q exits");
    XDrawImageString(dpy,help,gc,x_str,y_str,str,strlen(str));
    y_str += spacing;
    sprintf(str,"Press 'h', 'H', or '?' to unmap the help window");
    XDrawImageString(dpy,help,gc,x_str,y_str,str,strlen(str));
}

void
redisplay (event)
XExposeEvent    *event;
{
    if ((event->window == help) && (!useroot))
        print_help();
    else {
        /*
        * Extract the exposed area from the event and copy
        * from the saved pixmap to the window.
        */
        XCopyArea(dpy, pixmap, canvas, Data_GC[0][0], event->x, event->y, 
            event->width, event->height, event->x, event->y);
    }
}

void
resize()
{
    Window r;
    int j; 
    int x, y;
    unsigned int bw, d, new_w, new_h;

    XGetGeometry(dpy,canvas,&r,&x,&y,&new_w,&new_h,&bw,&d);
    if (((int)new_w == width) && ((int)new_h == height))
        return;
    width = (int)new_w; height = (int)new_h;
    if (pixmap)
        XFreePixmap(dpy, pixmap);
    pixmap = XCreatePixmap(dpy, DefaultRootWindow(dpy), 
            width, height, DefaultDepth(dpy, screen));
	initialize();
}

void
Cleanup() {
	XCloseDisplay(dpy);
}

/* Store splines growth in PPM format */
void
save()
{
    FILE *outfile;
    unsigned char c;
    XImage *ximage;
    static int i,j;
    struct Colormap {
        unsigned char red;
        unsigned char green;
        unsigned char blue;
    };
    struct Colormap *colormap=NULL;

    if ((colormap=
        (struct Colormap *)malloc((unsigned)sizeof(struct Colormap)*maxcolor))
            == NULL) {
        fprintf(stderr,"Error malloc'ing colormap array\n");
		Cleanup();
        exit(-1);
    }
    outfile = fopen(outname,"w");
    if(!outfile) {
        perror(outname);
		Cleanup();
        exit(-1);
    }

    ximage=XGetImage(dpy, pixmap, 0, 0, width, height, AllPlanes, ZPixmap);

    for (i=0;i<maxcolor;i++) {
        colormap[i].red=(unsigned char)(Colors[i].red >> 8);
        colormap[i].green=(unsigned char)(Colors[i].green >> 8);
        colormap[i].blue =(unsigned char)(Colors[i].blue >> 8);
    }
    fprintf(outfile,"P%d %d %d\n",6,width,height);
    fprintf(outfile,"%d\n",maxcolor-1);

    for (j=0;j<height;j++)
        for (i=0;i<width;i++) {
            c = (unsigned char)XGetPixel(ximage,i,j);
            fwrite((char *)&colormap[c],sizeof colormap[0],1,outfile);
        }
    fclose(outfile);
    free(colormap);
}

void
redraw()
{
	static int i;

	Clear();
	if (curve) /* Redraw the erased spline curve */
		drawspline(c_color, now_c);
	if (showballs) /* Redraw the erased balls */
		for (i=0; i<nballs; i++)
			drawball(i, balls[i].color);
}

void
Getkey(event)
XKeyEvent *event;
{
    char key;
	static int spinning=0, spindir=0, i;
    static XWindowAttributes attr;
	extern void init_color(), write_cmap();

    if (XLookupString(event, (char *)&key, sizeof(key), (KeySym *)0,
       (XComposeStatus *) 0) > 0)
            switch (key) {
            	case '\015': /*write out current colormap to $HOME/.<prog>map*/
        			write_cmap(dpy,cmap,Colors,maxcolor,"splines","Splines");
					break;
                case '<': naptime -= 20;
					if (naptime < 0)
						naptime = 0;
					break;
                case '>': naptime += 20;
					break;
				case '-': /* decrease nsteps */
					nsteps -= 10;
					if (nsteps < 1)
						nsteps = 1;
					redraw();
					break;
				case '+': /* Increase nsteps */
					nsteps += 10;
					redraw();
					break;
                case ']': mincolindex += INDEXINC;
                    if (mincolindex > maxcolor)
                        mincolindex = 1;
					if (!usedefault)
                        init_color(dpy,canvas,cmap,Colors,STARTCOLOR,
						mincolindex,maxcolor,numwheels,"splines","Splines",0);
                    break;
                case '[': mincolindex -= INDEXINC;
                    if (mincolindex < 1)
                        mincolindex = maxcolor - 1;
					if (!usedefault)
                        init_color(dpy,canvas,cmap,Colors,STARTCOLOR,
						mincolindex,maxcolor,numwheels,"splines","Splines",0);
                    break;
				case 'a':
				case 'A': ch_wheel = (!ch_wheel);
					break;
				case 'b':
				case 'B': bezier = (!bezier);
				/* work-around bug whereby if nballs>13 per Bezier spline, */
				/* the spline is always anchored at the origin */
					if (bezier && ((nballs/nsplines) > 13))
						nballs = nsplines * 13;
					redraw();
					break;
				case 'c':	/* clear the window */
					redraw();
					break;
				case 'C':	/* create a new ship, leaving the old */
					if (nballs >= BMAX - 1)
						break;
					freemem();
					nballs++;
			/* work-around bug whereby if nballs>13 per Bezier spline, */
			/* the spline is always anchored at the origin */
					if (bezier && ((nballs/nsplines) > 13))
						nballs = nsplines * 13;
					setupmem();
					if (spinlines)
						initialize();
					else {
						init_ball(nballs-1);/* initialize new ship */
						init_ctrl();
						redraw();
					}
					break;
                case 'd': delay -= 25; if (delay < 0) delay = 0; break;
                case 'D': delay += 25; break;
				case 'e': erase_curve = (!erase_curve);
					if (erase_curve)
						redraw();
					break;
				case 'E': erase_balls = (!erase_balls);
					if (erase_balls)
						redraw();
					break;
                case 'f':	/* save in PPM format file */
                case 'F': save(); break;
				case 'g': Fflag = (!Fflag);
					redraw();
					break;
				case 'G':	/* toggle drawing solo spun lines */
					solospin = (!solospin);
					redraw();
					break;
				case 'i':
				case 'I': stopping = (!stopping);
					break;
				case 'k':	/* toggle display of balls */
					showballs = (!showballs);
					redraw();
					break;
				case 'K':	/* toggle display of curves */
					curve = (!curve);
					redraw();
					break;
				case 'L':	/* toggle drawing lines */
					lines = (!lines);
					redraw();
					break;
				case '\014': /* (ctrl-L) toggle drawing spinning lines */
					spinlines = (!spinlines);
					initialize();
					break;
				case 'l':	/* delete a ball */
					if (nballs <= 3)
						break;
					freemem();
					nballs--;
					setupmem();
					redraw();
					break;
				case 'm': esiz /= 2; /* halve the length of the trail */
					if (esiz < 1)
						esiz = 1;
					initialize();
					break;
				case 'M': esiz *= 2; /* double the length of the trail */
					if (esiz > M_ESIZ)
						esiz = M_ESIZ;
					initialize();
					break;
                case 'N':	/* go on to the next splines */
                    nummaps++;	/* but don't increment the splines counter */
                case 'n':	/* go on to the next splines */
					if (ranflag) {
						freemem();
						ranparams();
						setupmem();
					}
					initialize();
                    break;
				case 'o': /* decrease order of spline */
					if (--s_order < 1)
						s_order = 1;
					redraw();
					break;
				case 'O': /* Increase order of spline */
					if (++s_order > 4)
						s_order = 4;
					redraw();
					break;
				case 'p': /* decrease nsplines */
					if (--nsplines < 1)
						nsplines = 1;
					redraw();
					break;
				case 'P': /* Increase nsplines */
					if (++nsplines > nballs/3)
						nsplines = nballs/3;
					redraw();
					break;
				case 'R': ranwalk = (!ranwalk);
					break;
                case 'r': spinning=1; spindir=1; 
					Spin(dpy, cmap, Colors, STARTCOLOR, maxcolor, delay, 1);
                    break;
                case 'S': spinning=0;
                    break;
                case 's': spinning=1; spindir=0;
					Spin(dpy, cmap, Colors, STARTCOLOR, maxcolor, delay, 0);
                    break;
                case 'T': delay_erase = (!delay_erase);
					if (delay_erase) {
						showballs = 0; xor = 0;
						e_color = 0;
					}
					now_c=0;
					redraw();
                    break;
            	case '\027': /* (ctrl-W) read palette from $HOME/.splinesmap */
                  numwheels = 0;
				  if (!usedefault)
                      init_color(dpy,canvas,cmap,Colors,STARTCOLOR,
						mincolindex,maxcolor,numwheels,"splines","Splines",0);
                  break;
                case 'W': 
                    if (numwheels < MAXWHEELS)
                        numwheels++;
                    else
                        numwheels = 0;
					if (!usedefault)
                        init_color(dpy,canvas,cmap,Colors,STARTCOLOR,
						mincolindex,maxcolor,numwheels,"splines","Splines",0);
                    break;
                case 'w': 
                    if (numwheels > 0)
                        numwheels--;
                    else
                        numwheels = MAXWHEELS;
					if (!usedefault)
                        init_color(dpy,canvas,cmap,Colors,STARTCOLOR,
						mincolindex,maxcolor,numwheels,"splines","Splines",0);
                    break;
                case '?':
                case 'h': 
					if (!useroot) {
						XGetWindowAttributes(dpy, help, &attr);
                    	if (attr.map_state != IsUnmapped)
                        	XUnmapWindow(dpy, help);
                    	else {
                        	XMapRaised(dpy, help);
                        	print_help();
                    	}
					}
                    break;
				case 'x':	/* toggle drawing in XOR mode */
					xor = (!xor);
					if (xor)
						e_color = c_color;
					else
						e_color = 0;
					redraw();
					break;
				case 'X':	/* create new splines, erasing the old */
						nummaps++;
						initialize();
						break;
                case 'Q':
                case 'q': Cleanup(); exit(0); break;
            }
			if (spinning)
				Spin(dpy, cmap, Colors, STARTCOLOR, maxcolor, delay, spindir);
}

void
parseargs(argc, argv)
int argc;
char *argv[];
{
   int c;
   extern int optind, getopt();
   extern char *optarg;

     outname = "splines.ppm";
     while((c=getopt(argc,argv,
		"abefrsuxBCEFGILRTWc:h:k:l:m:n:o:p:w:D:M:N:O:P:S:"))!=EOF)
    {    switch(c)
        {
		case 'a':	curve=0; showballs=1; break; /* balls only */
		case 'b':	curve=1; showballs=1; break; /* curve and balls */
        case 'c':
            numwheels = atoi(optarg);
            if (numwheels > MAXWHEELS)
                numwheels = MAXWHEELS;
            if (numwheels < 0)
                numwheels = 0;
            break;
		case 'e':
            erase_curve++;
            break;
        case 'f':
            Fflag=1;
            break;
        case 'u':
            usage();
            exit(0);
        case 'h':
            height = atoi(optarg);
            break;
        case 'l':
            limit = atol(optarg);
            break;
        case 'm':
            mincolindex = atoi(optarg);
            break;
        case 'n':
            nummaps = atoi(optarg);
            break;
        case 'o':
            ++oflag;
            outname = optarg;
            break;
        case 'p':
            nsteps = atoi(optarg);
            break;
        case 'r':
            ++ranflag;
            break;
        case 's':
            ++spin;
            break;
        case 'w':
            width = atoi(optarg);
            break;
        case 'x':	/* use xor as drawing mode */
            xor = 1;
            break;
        case 'B':
            bezier++;
            break;
        case 'C':	/* don't change color wheels automatically */
            ch_wheel = 0;
            break;
        case 'D':
            delay = atoi(optarg);
            break;
		case 'E':
            erase_balls++;
            break;
		case 'F':
            spinlines++;
            break;
		case 'G':
            solospin++;
            break;
        case 'I':
			stopping = 0;
            break;
        case 'L':
			lines = 1;
            break;
        case 'M':
            esiz = atoi(optarg);
			if (esiz > M_ESIZ)
				esiz = M_ESIZ;
			if (esiz < 1)
				esiz = 1;
            break;
        case 'N':
            nballs = atoi(optarg);
			if (nballs > BMAX)
				nballs = BMAX;
			if (nballs < 3)
				nballs = 3;
            break;
        case 'O':
            s_order = atoi(optarg);
			if (s_order < 1)
				s_order = 1;
			if (s_order > 4)
				s_order = 4;
            break;
        case 'P':
            nsplines = atoi(optarg);
			if (nsplines > BMAX)
				nsplines = BMAX;
			if (nsplines < 1)
				nsplines = 1;
            break;
        case 'R':
            useroot++;
			ch_wheel = 0;
            break;
        case 'S':
            naptime = atoi(optarg);
            break;
        case 'T':
			delay_erase=0;
            break;
		case 'W':	ranwalk++; break;
		case 'X':	xradius = atoi(optarg); break;
		case 'Y':	yradius = atoi(optarg); break;
        case '?':
            usage();
            exit(1);
            break;
        }
    }
	if (ranflag)
		ranparams();
	if (nsplines > nballs/3)
		nsplines = nballs/3;
	/* work-around bug whereby if nballs>13 per Bezier spline, */
	/* the spline is always anchored at the origin */
	if (bezier && ((nballs/nsplines) > 13))
		nballs = nsplines * 13;
}

void
event_loop()
{
    int n;
    XEvent event;

    n = XEventsQueued(dpy, QueuedAfterFlush);
    while (n-- > 0) {
        XNextEvent(dpy, &event);
        switch(event.type) {
            case KeyPress:
                Getkey(&event);
                break;
            case Expose:
				if (useroot)
					redraw();
				else
                	redisplay(&event);
                break;
            case ConfigureNotify:
                resize();
                break;
        }
    }
}

void
drawball(i, col)
int i, col;
{
	if (Fflag) {
  		XFillArc(dpy,pixmap,Data_GC[1][col],
				balls[i].x-xradius,balls[i].y-yradius,
				2*xradius,2*yradius,0,23040);
  		XFillArc(dpy,canvas,Data_GC[1][col],
				balls[i].x-xradius,balls[i].y-yradius,
				2*xradius,2*yradius,0,23040);
	}
	else {
  		XDrawArc(dpy,pixmap,Data_GC[1][col],
				balls[i].x-xradius,balls[i].y-yradius,
				2*xradius,2*yradius,0,23040);
  		XDrawArc(dpy,canvas,Data_GC[1][col],
				balls[i].x-xradius,balls[i].y-yradius,
				2*xradius,2*yradius,0,23040);
	}
}

void
erase_old()
{
	static int i;

	if (spinlines)
		Clear();
	else
		for (i=0; i<esiz; i++)
			drawspline(0, i);
	now_c = 0;
}

void
move() 
{
	static int i;

	++num_c;
	if (curve && erase_curve)	/* Erase the drawn spline curve */
		drawspline(e_color, now_c);
	if (curve && delay_erase) {
		if (num_c >= esiz) /* erase esiz ago curve */
			drawspline(0, num_c % esiz);
		if (++now_c >= esiz)
			now_c = 0;
	}
	for( i=0; i<nballs; i++ )
	{
		if (showballs && erase_balls)
			drawball(i, (erase_curve || delay_erase) ? 0 : balls[i].color);
		if (ranwalk && ((rand()&0xff) > 192)) {
			balls[i].xdir = (rand()&0xff) > 128 ? rand()%M_X+1 :-(rand()%M_X+1);
			balls[i].ydir = (rand()&0xff) > 128 ? rand()%M_Y+1 :-(rand()%M_Y+1);
		}
		balls[i].x +=balls[i].xdir;
		balls[i].y +=balls[i].ydir;
		if( (balls[i].x <0) || (balls[i].x>width-1))
		{
			/* bounced off screen */
			balls[i].xdir = -balls[i].xdir;
			balls[i].x +=balls[i].xdir;
			balls[i].x +=balls[i].xdir;
		}
		if( (balls[i].y <0) || (balls[i].y >height-1))
		{
			/* bounced off screen */
			balls[i].ydir = -balls[i].ydir;
			balls[i].y +=balls[i].ydir;
			balls[i].y +=balls[i].ydir;
		}
		if( showballs )
			drawball(i, balls[i].color);
		if (curve) {
			ctrl[now_c][i][0] = balls[i].x;
			ctrl[now_c][i][1] = balls[i].y;
		}
	}
	Timer(naptime);
	if (curve) { /* Draw the new spline curve */
		if (++c_color >= maxcolor) { /* increment the color index */
			if (ch_wheel) {/*cannot be using the root window if ch_wheel set*/
			  if (ch_wheel++ == MAX_CINC) {
				ch_wheel = 1;
				if (curve && delay_erase)
					erase_old();
                if (numwheels > 0)
                    numwheels--;
                else
                    numwheels = MAXWHEELS;
				if (!usedefault)
                    init_color(dpy,canvas,cmap,Colors,STARTCOLOR,
					  mincolindex,maxcolor,numwheels,"splines","Splines",0);
			  }
		    }
			c_color = STARTCOLOR;
		}
		if (xor)
			e_color = c_color;
		drawspline(c_color, now_c);
	}
}

main(argc,argv)
int argc;
char *argv[];
{
    static int i, j;
	static int count;
    XSizeHints hint;
    Atom __SWM_VROOT = None;
	XVisualInfo *visual_list, visual_template;
    Window rootReturn, parentReturn, *children;
    unsigned int numChildren;
    extern void srand(), init_color();
    
    srand((unsigned)time(0));
    parseargs(argc,argv);
    dpy = XOpenDisplay("");
    screen = DefaultScreen(dpy);
    if (useroot) {
        width = XDisplayWidth(dpy, screen);
        height = XDisplayHeight(dpy, screen);
    }
	if (width == 0)
        width = XDisplayWidth(dpy, screen);
	if (height == 0)
        height = XDisplayHeight(dpy, screen);
	if (nsteps == 0)
		nsteps = 5 * (nballs/nsplines);
    maxcolor  = (int)XDisplayCells(dpy, screen);
	if (maxcolor <= 16) {
		STARTCOLOR = 2; delay = 100;
		INDEXINC = 1; mincolindex = 5;
	}
	maxcolor = Min(maxcolor, MAXCOLOR);
	numfreecols = maxcolor - STARTCOLOR;
	if (limit == 0)
		limit = width * height;
    /*
    * Create the pixmap to hold the splines growth
    */
    pixmap = XCreatePixmap(dpy, DefaultRootWindow(dpy), width, height, 
                           DefaultDepth(dpy, screen));
    /*
    * Create the window to display the fractal topographic map
    */
    hint.x = 0;
    hint.y = 0;
    hint.width = width;
    hint.height = height;
    hint.flags = PPosition | PSize;
    if (useroot) {
        canvas = DefaultRootWindow(dpy);
        /* search for virtual root (from ssetroot by Tom LaStrange) */
        __SWM_VROOT = XInternAtom(dpy, "__SWM_VROOT", False);
        XQueryTree(dpy,canvas,&rootReturn,&parentReturn,&children,&numChildren);
        for (j = 0; j < numChildren; j++) {
            Atom actual_type;
            int actual_format;
            long nitems, bytesafter;
            Window *newRoot = NULL;

            if (XGetWindowProperty (dpy, children[j], __SWM_VROOT,0,1, False, 
                XA_WINDOW, &actual_type, &actual_format, &nitems, &bytesafter,
                (unsigned char **) &newRoot) == Success && newRoot) {
                canvas = *newRoot;
                break;
            }
        }
    }
    else {
        canvas = XCreateSimpleWindow(dpy, DefaultRootWindow(dpy),
            0, 0, width, height, 5, 0, 1);
        XSetStandardProperties(dpy, canvas, "Splines by Ron Record",
            "Splines", None, argv, argc, &hint);
        XMapRaised(dpy, canvas);
    }
    XChangeProperty(dpy, canvas, XA_WM_CLASS, XA_STRING, 8, PropModeReplace, 
					"splines", strlen("splines"));
    /*
    * Create the window used to display the help info (if not running on root)
    */
	if (!useroot) {
    	hint.x = XDisplayWidth(dpy, screen) / 4;
    	hint.y = XDisplayHeight(dpy, screen) / 5;
    	hint.width = hint.x * 2;
    	hint.height = hint.y * 3;
    	help = XCreateSimpleWindow(dpy, DefaultRootWindow(dpy),
            	hint.x, hint.y, hint.width, hint.height, 5, 0, 1);
		XSetWindowBackground(dpy, help, BlackPixel(dpy, screen));
    	/* Title */
    	XSetStandardProperties(dpy,help,"Help","Help",None,argv,argc,&hint);
    	/* Try to write into a new color map */
		visual_template.class = PseudoColor;
		visual_list = XGetVisualInfo(dpy,VisualClassMask,&visual_template, &i);
		if (! visual_list)
			usedefault = 1;
		if (usedefault)
		  cmap = DefaultColormap(dpy, screen);
		else {
    	  cmap = XCreateColormap(dpy,canvas,DefaultVisual(dpy,screen),AllocAll);
    	  init_color(dpy, canvas, cmap, Colors, STARTCOLOR, mincolindex, 
			  maxcolor, numwheels,"splines", "Splines", 0);
    	  /* install new color map */
          XSetWindowColormap(dpy, canvas, cmap);
          XSetWindowColormap(dpy, help, cmap);
	    }
	}
    init_contexts();
	if (useroot)
    	XSelectInput(dpy,canvas,ExposureMask);
	else {
    	XSelectInput(dpy,canvas,KeyPressMask|ExposureMask|StructureNotifyMask);
    	XSelectInput(dpy,help,KeyPressMask|ExposureMask);
	}
	setupmem();
    for (i=0; i!=nummaps; i++) {
		initialize();
		for (;;) {
			move();
			event_loop();
			if ((num_c > limit) && stopping)
			  break;
		}
        if (oflag)
            save();
        if (spin && (!useroot) && curve && (!erase_curve))
			DemoSpin(dpy, cmap, Colors, STARTCOLOR, maxcolor, delay, 1);
		freemem();
		if (ranflag)
			ranparams();
		setupmem();
		if (xor)
			e_color = c_color;
		else
			e_color = 0;
		if (!useroot)
			if (!usedefault)
        	    init_color(dpy,canvas,cmap,Colors,STARTCOLOR,
					mincolindex,maxcolor,numwheels,"splines","Splines",0);
    }
    XSetWindowBackgroundPixmap(dpy, canvas, pixmap);
	for (i=0; i<maxcolor; i++) {
		XFreeGC(dpy, Data_GC[0][i]);
		XFreeGC(dpy, Data_GC[1][i]);
	}
	XFreePixmap(dpy, pixmap);
	XClearWindow(dpy, canvas);
	XFlush(dpy);
	Cleanup();
    exit(0);
}
