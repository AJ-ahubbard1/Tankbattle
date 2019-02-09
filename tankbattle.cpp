// Waterwheel
//modified by: Andrew Hubbard
//date: 2/9/2019
//
//3350 Spring 2018 Lab-1
//This program demonstrates the use of OpenGL and XWindows
//
//Assignment is to modify this program.
//You will follow along with your instructor.
//
//Elements to be learned in this lab...
// .general animation framework
// .animation loop
// .object definition and movement
// .collision detection
// .mouse/keyboard interaction
// .object constructor
// .coding style
// .defined constants
// .use of static variables
// .dynamic memory allocation
// .simple opengl components
// .git
//
//elements we will add to program...
//   .Game constructor
//   .multiple bullets
//   .gravity
//   .collision detection
//   .more objects
//
#include <iostream>
using namespace std;
#include <stdio.h>
#include <cstdlib>
#include <ctime>
#include <cstring>
#include <cmath>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <GL/glx.h>
#include "fonts.h"

// Speed of tanks
const float SPEED = 2.0; 	
const float GRAVITY = -0.3;
const float PI = 3.1415926535;
const float TILT = .02;
const float MAXPOWER = 20;
const float POW = .25;
const int P1 = 0;
const int P2 = 1;
const int WIDTH = 40;
const int HEIGHT = 30;
const int KEYS = 98;
const int RADIUS = 4;
const float HITLOSS = 24;
//some structures
struct Vec {
	float x, y, z;
};

struct Shape {
	float width, height;
	float radius;
	Vec center;
};

struct Bullet {
	Shape s;
	Vec velocity;
};

struct Gun {
	Shape s;
	Vec v1, v2, v3, v4;

	// x = angle, y = power
	Vec aim;
};

struct Tank {
	Shape body;
	Gun gun;
	Vec pos;
	Shape powerbar;	
	Shape healthbar;	
};

class Global {
	public:
	int xres, yres;
	int xmouse, ymouse;
	Shape barrier;
	Shape ground;
	Tank tank[2];	
	bool loaded[2];
	bool alive[2];
	int keyhits[KEYS];
	Bullet bullet[2];
	Global() {
		xres = 1000;
		yres = 750;
		loaded[P1] = true;
		loaded[P2] = true;
		alive[P1] = true;
		alive[P2] = true;
		//define barrier shape
		barrier.width = 15;
		barrier.height = yres/4;
		barrier.center.x = xres/2;
		ground.width = xres;
		ground.height = 5;
		tank[P1].body.width = tank[P2].body.width = WIDTH;
		tank[P1].body.height = tank[P2].body.height = HEIGHT;
		tank[P1].gun.s.width = tank[P2].gun.s.width = WIDTH;
		tank[P1].gun.s.height = tank[P2].gun.s.height = 10;
		tank[P1].gun.aim.x = TILT;
		tank[P2].gun.aim.x = PI - TILT;
		tank[P1].gun.aim.y = tank[P2].gun.aim.y = 10.0;
		tank[P1].pos.x = xres/4;
		tank[P2].pos.x = (xres/4)*3;
		tank[P1].pos.y = tank[P2].pos.y = 5;
		tank[P1].powerbar.height = tank[P1].gun.aim.y * tank[P1].gun.aim.y;
		tank[P1].powerbar.width = 5.0;
		tank[P1].powerbar.center.x = 0;
		tank[P1].powerbar.center.y = yres/2;
		tank[P2].powerbar.height = tank[P2].gun.aim.y * tank[P2].gun.aim.y;
		tank[P2].powerbar.height = 100;	
		tank[P2].powerbar.width = 5.0;
		tank[P2].powerbar.center.x = xres;
		tank[P2].powerbar.center.y = yres/2;
		tank[P1].healthbar.height = 5;	
		tank[P1].healthbar.width = 120.0;
		tank[P1].healthbar.center.x = xres/4;
		tank[P1].healthbar.center.y = yres - 10;
		tank[P2].healthbar.height = 5;	
		tank[P2].healthbar.width = 120.0;
		tank[P2].healthbar.center.x = (xres*3)/4;
		tank[P2].healthbar.center.y = yres - 10;
			
	}
} g;

class X11_wrapper {
	private:
	Display *dpy;
	Window win;
	GLXContext glc;
	public:
	~X11_wrapper() {
		XDestroyWindow(dpy, win);
		XCloseDisplay(dpy);
	}
	X11_wrapper() {
		GLint att[] = {GLX_RGBA, GLX_DEPTH_SIZE, 24, 
			GLX_DOUBLEBUFFER, None };
		int w = g.xres, h = g.yres;
		dpy = XOpenDisplay(NULL);
		if (dpy == NULL) {
			cout << "\n\tcannot connect to X server\n" << endl;
			exit(EXIT_FAILURE);
		}
		Window root = DefaultRootWindow(dpy);
		XVisualInfo *vi = glXChooseVisual(dpy, 0, att);
		if (vi == NULL) {
			cout << "\n\tno appropriate visual found\n" << endl;
			exit(EXIT_FAILURE);
		} 
		Colormap cmap = XCreateColormap(dpy, root,vi->visual, AllocNone);
		XSetWindowAttributes swa;
		swa.colormap = cmap;
		swa.event_mask =
			ExposureMask | KeyPressMask | KeyReleaseMask |
			ButtonPress | ButtonReleaseMask |
			PointerMotionMask |
			StructureNotifyMask | SubstructureNotifyMask;
		win = XCreateWindow(dpy, root, 0, 0, w, h, 0, vi->depth,
			InputOutput, vi->visual, CWColormap | CWEventMask, &swa);
		set_title();
		glc = glXCreateContext(dpy, vi, NULL, GL_TRUE);
		glXMakeCurrent(dpy, win, glc);
	}
	void set_title() {
		//Set the window title bar.
		XMapWindow(dpy, win);
		XStoreName(dpy, win, "Tank Battle");
	}
	bool getXPending() {
		//See if there are pending events.
		return XPending(dpy);
	}
	XEvent getXNextEvent() {
		//Get a pending event.
		XEvent e;
		XNextEvent(dpy, &e);
		return e;
	}
	void swapBuffers() {
		glXSwapBuffers(dpy, win);
	}
} x11;

//Function prototypes
void init_opengl(void);
int check_keys(XEvent *e);
void movement();
void render();

//=====================================
// MAIN FUNCTION IS HERE
//=====================================
int main()
{
	srand(time(NULL));
	init_opengl();
	//Main animation loop
	int done = 0;
	while (!done) {
		//Process external events.
		while (x11.getXPending()) {
			XEvent e = x11.getXNextEvent();
			done = check_keys(&e);
		}
		movement();
		render();
		x11.swapBuffers();
	}
	cleanup_fonts();
	return 0;
}

void init_opengl(void) 
{
	//OpenGL initialization
	glViewport(0, 0, g.xres, g.yres);
	//Initialize matrices
	glMatrixMode(GL_PROJECTION); glLoadIdentity();
	glMatrixMode(GL_MODELVIEW); glLoadIdentity();
	//Set 2D mode (no perspective)
	glOrtho(0, g.xres, 0, g.yres, -1, 1);
	//Set the screen background color
	glClearColor(0.1, 0.1, 0.1, 1.0);
	glEnable(GL_TEXTURE_2D);
	initialize_fonts();
}

// p for P1 or P2
void makebullet(int p) {
	if (!g.loaded[p])
		return;
	float x, y;
	Bullet *b = &g.bullet[p];
	Tank *t = &g.tank[p];
	
	b->s.width = RADIUS;
	b->s.height = RADIUS;
	
	x = (t->gun.v4.x +  t->gun.v3.x +
	     t->gun.v2.x +  t->gun.v1.x) * .25;
	y = (t->gun.v4.y +  t->gun.v3.y +
	     t->gun.v2.y +  t->gun.v1.y) * .25;
	
	b->s.center.x = x;
	b->s.center.y = y;
	
	x = t->gun.aim.y * cos(t->gun.aim.x);
	y = t->gun.aim.y * sin(t->gun.aim.x);
	
	b->velocity.x = x;
	b->velocity.y = y;
	
//	g.loaded[p] = false;	uncomment when done troubleshooting
}

int check_keys(XEvent *e) 
{
	int key = XLookupKeysym(&e->xkey, 0);	
	/* nothing pressed
	if (e->type != KeyPress && e->type != KeyRelease) {
		for (int i = 0; i < KEYS; i++) 
			g.keyhits[i] = 0;
	}
	*/	
	if (e->type == KeyRelease) {
		g.keyhits[key%100] = 0;
		//cout << key % 100 << ": Key Released\n";
	}
	
	if (e->type == KeyPress) {
		g.keyhits[key%100] = 1;
		// cout << key % 100 << ": Key Pressed\n";
		if(key == XK_Escape)
			return 1;
	}
	return 0;
}

void movement() 
{
if(g.alive[P1]) {	
	// Spacebar
	if (g.keyhits[32])
		makebullet(P1);
	// a
	if (g.keyhits[97])
		g.tank[P1].pos.x -= SPEED;
	// d
	if (g.keyhits[0])
		g.tank[P1].pos.x += SPEED;
	// w
	if (g.keyhits[19]) {
		g.tank[P1].gun.aim.y += POW;
		if (g.tank[P1].gun.aim.y > MAXPOWER)
			g.tank[P1].gun.aim.y = MAXPOWER;
	}
	// s
	if (g.keyhits[15]) {
		g.tank[P1].gun.aim.y -= POW;
		if (g.tank[P1].gun.aim.y < 5.0)
			g.tank[P1].gun.aim.y = 5.0;
	}
	// q
	if (g.keyhits[13]) {
		g.tank[P1].gun.aim.x += TILT;
		if (g.tank[P1].gun.aim.x > PI/2)
			g.tank[P1].gun.aim.x = PI/2;
	}
	// e
	if (g.keyhits[1]) {
		g.tank[P1].gun.aim.x -= TILT;
		if (g.tank[P1].gun.aim.x < TILT)
			g.tank[P1].gun.aim.x = TILT;
	}
}
if(g.alive[P2]) {
	// numkey 0 
	if (g.keyhits[38])
		makebullet(P2);
	// left arrow
	if (g.keyhits[61])
		g.tank[P2].pos.x -= SPEED;
	// right arrow
	if (g.keyhits[63])
		g.tank[P2].pos.x += SPEED;
	// up arrow
	if (g.keyhits[62]) {
		g.tank[P2].gun.aim.y += POW;
		if (g.tank[P2].gun.aim.y > MAXPOWER)
			g.tank[P2].gun.aim.y = MAXPOWER;
	}
	// down arrow
	if (g.keyhits[64]) {
		g.tank[P2].gun.aim.y -= POW;
		if (g.tank[P2].gun.aim.y < 5.0)
			g.tank[P2].gun.aim.y = 5.0;
	}
	// 3	
	if (g.keyhits[35]) {
		g.tank[P2].gun.aim.x -= TILT;
		if (g.tank[P2].gun.aim.x < PI/2)
			g.tank[P2].gun.aim.x = PI/2;
	}
	// 1	
	if (g.keyhits[36]) {
		g.tank[P2].gun.aim.x += TILT;
		if (g.tank[P2].gun.aim.x > PI - TILT)
			g.tank[P2].gun.aim.x = PI - TILT;
	}
}
	// t troubleshoot button
	if (g.keyhits[16]) {
		cout << "\nPlayer 1\nPower: " << g.tank[P1].gun.aim.y << endl;
		cout << "Angle: " << g.tank[P1].gun.aim.x << endl;
		cout << "Player 2\nPower: " << g.tank[P2].gun.aim.y << endl;
		cout << "Angle: " << g.tank[P2].gun.aim.x << "\n\n";
	}


	// Keeps tanks in barriers
	if (g.tank[P1].pos.x > g.xres/2 - WIDTH - 5) {
		g.tank[P1].pos.x = g.xres/2 - WIDTH - 5;
		g.tank[P1].pos.z = 0;
	}
	if (g.tank[P2].pos.x < g.xres/2 + WIDTH + 5) {
		g.tank[P2].pos.x = g.xres/2 + WIDTH + 5;
		g.tank[P2].pos.z = 0;
	}
	if (g.tank[P1].pos.x < WIDTH) {
		g.tank[P1].pos.x = WIDTH;
		g.tank[P1].pos.z = 0;
	}
	if (g.tank[P2].pos.x > g.xres - WIDTH) {
		g.tank[P2].pos.x = g.xres - WIDTH;
		g.tank[P2].pos.z = 0;
	}
	// setting vertices of guns
	for (int i = 0; i < 2; i++) {
		float x, y, w, h, a, cosA, sinA, cos_A, sin_A;
		Shape *s;	
		s = &g.tank[i].gun.s;
		w = s->width * 2;
		h = s->height * 2;
		x = g.tank[i].pos.x;
		y = g.tank[i].pos.y + HEIGHT;
		a = g.tank[i].gun.aim.x;
		cosA = cos(a);
		sinA = sin(a);
		cos_A = cos(a + (PI/4));
		sin_A = sin(a + (PI/4));
		Gun *v;
		v = &g.tank[i].gun;
		v->v1.x = x;
		v->v1.y = y;
		v->v2.x = x + h*cos_A;
		v->v2.y = y + h*sin_A;
		v->v3.x = x + w*cosA + h*cos_A;	    
       		v->v3.y = y + w*sinA + h*sin_A;	    
		v->v4.x = x + w*cosA;
	       	v->v4.y = y + w*sinA;
		
		i++;	
		s = &g.tank[i].gun.s;
		w = s->width * 2;
		h = s->height * 2;
		x = g.tank[i].pos.x;
		y = g.tank[i].pos.y + HEIGHT;
		a = g.tank[i].gun.aim.x;
		cosA = cos(a);
		sinA = sin(a);
		cos_A = cos( -(PI/4) + a);
		sin_A = sin( -(PI/4) + a);
		v = &g.tank[i].gun;
		v->v1.x = x;
		v->v1.y = y;
		v->v2.x = x + h*cos_A;
		v->v2.y = y + h*sin_A;
		v->v3.x = x + w*cosA + h*cos_A;	    
       		v->v3.y = y + w*sinA + h*sin_A;	    
		v->v4.x = x + w*cosA;
	       	v->v4.y = y + w*sinA;
	}
	for (int i = 0; i < 2; i++) {
		// bullet gravity
		Bullet *b = &g.bullet[i];
		Tank *t = &g.tank[i];		
		b->s.center.x += b->velocity.x;
		b->s.center.y += b->velocity.y;
		b->velocity.y += GRAVITY;
		t->body.center.x = t->pos.x;
		t->body.center.y = t->pos.y;
		
		Shape *w = &g.barrier;
		if ((b->s.center.y < w->height) && 
		    (b->s.center.x > w->center.x - w->width) &&
		    (b->s.center.x < w->center.x + w->width)) {
			b->s.center.x = -1;
		 	b->s.center.y = -1;
			b->velocity.x = 0;
			b->velocity.y = 0;
		}
	}
	for (int i = 0; i < 2; i++) {
	for (int j = 0; j < 2; j++) {
		Bullet *b = &g.bullet[i];
		Tank *t = &g.tank[j];		
		if ( (i != j) && (b->s.center.y < t->pos.y + t->body.height) && 
 		     		 (b->s.center.y > t->pos.y - t->body.height) && 
				 (b->s.center.x > t->pos.x - t->body.width) &&
		    	         (b->s.center.x < t->pos.x + t->body.width)) {
				b->s.center.x = -1;
			 	b->s.center.y = -1;
				b->velocity.x = 0;
				b->velocity.y = 0;
				t->healthbar.width -= HITLOSS;
				if (t->healthbar.width <= 0) 
					g.alive[j] = false;
		}
	}
	}		
	
	// powerbar 
	g.tank[P1].powerbar.height = g.tank[P1].gun.aim.y * g.tank[P1].gun.aim.y;
	g.tank[P2].powerbar.height = g.tank[P2].gun.aim.y * g.tank[P2].gun.aim.y;
	
}

void render() 
{
	glClear(GL_COLOR_BUFFER_BIT);
	//Draw shapes...
	//
	//draw a box
	Shape *bs[8];
	bs[0] = &g.barrier; 
	bs[1] = &g.ground;
	bs[2] = &g.tank[P1].body;
	bs[3] = &g.tank[P2].body;
	bs[4] = &g.tank[P1].powerbar;
	bs[5] = &g.tank[P2].powerbar;
	bs[6] = &g.tank[P1].healthbar;
	bs[7] = &g.tank[P2].healthbar;
	float w, h; 
	for (int i = 0; i <8; i++) {	   
		if ( i == 4 || i == 5)
			glColor3ub(150,160,220);
		else
			glColor3ub(90,140,90);
		glPushMatrix();
		glTranslatef(bs[i]->center.x, bs[i]->center.y, bs[i]->center.z);    
		w = bs[i]->width;
		h = bs[i]->height;
		glBegin(GL_QUADS);
		glVertex2i(-w, -h);
		glVertex2i(-w,  h);
		glVertex2i( w,  h);
		glVertex2i( w, -h);
		glEnd();
		glPopMatrix();
	}
	
	bs[0] = &g.tank[P1].gun.s;
	bs[1] = &g.tank[P2].gun.s;
	Gun *s[2];
       	s[0] = &g.tank[P1].gun;
       	s[1] = &g.tank[P2].gun;
	for (int i = 0; i < 2; i++) {	   
		glColor3ub(90,140,90);
		glPushMatrix();
		glTranslatef(bs[i]->center.x, bs[i]->center.y, bs[i]->center.z);    
		glBegin(GL_QUADS);
		glVertex2i(s[i]->v1.x, s[i]->v1.y);
		glVertex2i(s[i]->v2.x, s[i]->v2.y);
		glVertex2i(s[i]->v3.x, s[i]->v3.y);
		glVertex2i(s[i]->v4.x, s[i]->v4.y);
		glEnd();
		glPopMatrix();
		i++;
		glColor3ub(90,140,90);
		glPushMatrix();
		glTranslatef(bs[i]->center.x, bs[i]->center.y, bs[i]->center.z);    
		glBegin(GL_QUADS);
		glVertex2i(s[i]->v1.x, s[i]->v1.y);
		glVertex2i(s[i]->v2.x, s[i]->v2.y);
		glVertex2i(s[i]->v3.x, s[i]->v3.y);
		glVertex2i(s[i]->v4.x, s[i]->v4.y);
		glEnd();
		glPopMatrix();
	}
	//Draw the bullet here
	glPushMatrix();
	glColor3ub(150,160,220);
	for (int i = 0; i < 2; i++) {
		Vec *c = &g.bullet[i].s.center;
		w = h = RADIUS;
		glBegin(GL_QUADS);
		glVertex2i(c->x-w, c->y-h);
		glVertex2i(c->x-w, c->y+h);
		glVertex2i(c->x+w, c->y+h);
		glVertex2i(c->x+w, c->y-h);
		glEnd();
	}    	
	glPopMatrix();
	/* Draw your 2D text here
	Rect r[5];
	unsigned int c = 0x00ffff44;
	r[0].bot = g.box[0].center.y-5;
	r[0].left = g.box[0].center.x-39;
	r[0].center = 0;
	ggprint8b(&r[0], 16, c, "REQUIREMENTS");
	*/
}





