// Modified by: Tristan Bock, Kenny Taylor, Rudy Martinez
// Last updated 11/1/18

// Purpose: CMPS 3350 Fall 2018 Project

// Original author:  Gordon Griesel
// Original date:    summer 2017
//         spring 2018
//
//Walk cycle using a sprite sheet.
//images courtesy: http://games.ucla.edu/resource/walk-cycles/
//
//This program includes:
//  multiple sprite-sheet animations
//  a level tiling system
//  parallax scrolling of backgrounds
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <math.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <GL/glx.h>
#include <iostream>
#include "log.h"
//#include "ppm.h"
#include "fonts.h"
#include "global.h"
#include "image.h"
#include "level.h"
#include "rudyM.h"
#include "sprite.h"
#include "kennyT.h"
#include "tristanB.h"

using namespace std;

//defined types
//typedef double Flt;
typedef double Vec[3];
typedef Flt	Matrix[4][4];

//macros
#define rnd() (((double)rand())/(double)RAND_MAX)
#define random(a) (rand()%a)
#define MakeVector(v, x, y, z) (v)[0]=(x),(v)[1]=(y),(v)[2]=(z)
#define VecCopy(a,b) (b)[0]=(a)[0];(b)[1]=(a)[1];(b)[2]=(a)[2]
#define VecDot(a,b)	((a)[0]*(b)[0]+(a)[1]*(b)[1]+(a)[2]*(b)[2])
#define VecSub(a,b,c) (c)[0]=(a)[0]-(b)[0]; \
                             (c)[1]=(a)[1]-(b)[1]; \
(c)[2]=(a)[2]-(b)[2]

//constants
const float timeslice = 1.0f;
const float gravity = -0.2f;
#define ALPHA 1

//function prototypes
void initOpengl();
void checkMouse(XEvent *e);
int checkKeys(XEvent *e);
void init();
void physics();
void render();
void showIntroScreen();
void show_credits();

// timer.cpp functions
extern struct timespec timeStart, timeCurrent;
extern double timeDiff(struct timespec *start, struct timespec *end);
extern void timeCopy(struct timespec *dest, struct timespec *source);

// Tristan file functions
//extern void tristanCredits(Rect*);
//extern void tristanImage(int, int, GLuint);

// Kenny file functions
extern void drawText(int,int,int,char*);
extern void showKennyCredits(Rect*);
extern void showKennyCredits(int, int);
extern void showKennyImage(int, int, GLuint);
extern void loadLevel(Level*, int);
extern void drawTexturedBackground();
extern void drawSolidTile(float,float,float,float,float,float);
extern void drawTexturedTile(int, float, float, float);
extern void loadTexture(GLuint*, Image);
extern void loadTextureAlpha(GLuint*, Image);
extern void showLeaderboard();
extern void showIntroScreen();

// Rudy file functions
extern void rudyCredits(Rect*);
extern void showRudyPicture(int, int, GLuint);

//-----------------------------------------------------------------------------
//Setup timers
class Timers {
    public:
        double physicsRate;
        double oobillion;
        struct timespec timeStart, timeEnd, timeCurrent;
        struct timespec walkTime;
        Timers() {
            physicsRate = 1.0 / 30.0;
            oobillion = 1.0 / 1e9;
        }
        double timeDiff(struct timespec *start, struct timespec *end) {
            return (double)(end->tv_sec - start->tv_sec ) +
                (double)(end->tv_nsec - start->tv_nsec) * oobillion;
        }
        void timeCopy(struct timespec *dest, struct timespec *source) {
            memcpy(dest, source, sizeof(struct timespec));
        }
        void recordTime(struct timespec *t) {
            clock_gettime(CLOCK_REALTIME, t);
        }
} timers;
//-----------------------------------------------------------------------------


Global gl;

Player* pl = new Player;

// Start on level 1
Level lev;

//X Windows variables
class X11_wrapper {
    private:
        Display *dpy;
        Window win;
    public:
        ~X11_wrapper() {
            XDestroyWindow(dpy, win);
            XCloseDisplay(dpy);
        }
        void setTitle() {
            //Set the window title bar.
            XMapWindow(dpy, win);
            XStoreName(dpy, win, "Bob's Day Off");
        }
        void setupScreenRes(const int w, const int h) {
            gl.xres = w;
            gl.yres = h;
        }
        X11_wrapper() {
            GLint att[] = { GLX_RGBA, GLX_DEPTH_SIZE, 24, GLX_DOUBLEBUFFER, None };
            //GLint att[] = { GLX_RGBA, GLX_DEPTH_SIZE, 24, None };
            XSetWindowAttributes swa;
            setupScreenRes(gl.xres, gl.yres);
            dpy = XOpenDisplay(NULL);
            if (dpy == NULL) {
                printf("\n\tcannot connect to X server\n\n");
                exit(EXIT_FAILURE);
            }
            Window root = DefaultRootWindow(dpy);
            XVisualInfo *vi = glXChooseVisual(dpy, 0, att);
            if (vi == NULL) {
                printf("\n\tno appropriate visual found\n\n");
                exit(EXIT_FAILURE);
            } 
            Colormap cmap = XCreateColormap(dpy, root, vi->visual, AllocNone);
            swa.colormap = cmap;
            swa.event_mask = ExposureMask | KeyPressMask | KeyReleaseMask |
                StructureNotifyMask | SubstructureNotifyMask;
            win = XCreateWindow(dpy, root, 0, 0, gl.xres, gl.yres, 0,
                    vi->depth, InputOutput, vi->visual,
                    CWColormap | CWEventMask, &swa);
            GLXContext glc = glXCreateContext(dpy, vi, NULL, GL_TRUE);
            glXMakeCurrent(dpy, win, glc);
            setTitle();
        }
        void reshapeWindow(int width, int height) {
            //window has been resized.
            setupScreenRes(width, height);
            glViewport(0, 0, (GLint)width, (GLint)height);
            glMatrixMode(GL_PROJECTION); glLoadIdentity();
            glMatrixMode(GL_MODELVIEW); glLoadIdentity();
            glOrtho(0, gl.xres, 0, gl.yres, -1, 1);
            setTitle();
        }
        void checkResize(XEvent *e) {
            //The ConfigureNotify is sent by the
            //server if the window is resized.
            if (e->type != ConfigureNotify)
                return;
            XConfigureEvent xce = e->xconfigure;
            if (xce.width != gl.xres || xce.height != gl.yres) {
                //Window size did change.
                reshapeWindow(xce.width, xce.height);
            }
        }
        bool getXPending() {
            return XPending(dpy);
        }
        XEvent getXNextEvent() {
            XEvent e;
            XNextEvent(dpy, &e);
            return e;
        }
        void swapBuffers() {
            glXSwapBuffers(dpy, win);
        }
} x11;

int main()
{
    initOpengl();
    lev.load(1);
    init();
    int done = 0;
    while (!done) {
        while (x11.getXPending()) {
            XEvent e = x11.getXNextEvent();
            x11.checkResize(&e);
            checkMouse(&e);
            done = checkKeys(&e);
        }

        // Run physics unless the game is paused
        if (!gl.paused)
            physics();

        // Check if the player is dead
        if (pl->getHP() <= 0) {
            gl.paused = 1;
            delete pl;
            pl = new Player;
            gl.endScreenFlag = 1;
        }

        // Check for dead zombies
        for (unsigned int i = 0; i < lev.zmb.size(); i++)
            if (lev.zmb[i].HP <= 0) {
                // Draw explosion
                std::pair<int,int> enemyPos = lev.zmb[i].getPos();
                gl.exp.pos[0] = enemyPos.first - gl.camera[0];
                gl.exp.pos[1] = enemyPos.second + 200;
                gl.exp.pos[2] =   0.0;
                timers.recordTime(&gl.exp.time);
                gl.exp.onoff ^= 1;
                // Delete dead zombie
                lev.zmb.erase(lev.zmb.begin() + i);
                // Add player score
                gl.score += 1000;
            }

        // Check for dead slime enemies
        for (unsigned int i = 0; i < lev.slmE.size(); i++)
            if (lev.slmE[i].HP <= 0) {
                // Draw explosion
                std::pair<int,int> enemyPos = lev.slmE[i].getPos();
                gl.exp.pos[0] = enemyPos.first - gl.camera[0];
                gl.exp.pos[1] = enemyPos.second + 200;
                gl.exp.pos[2] =   0.0;
                timers.recordTime(&gl.exp.time);
                gl.exp.onoff ^= 1;
                // Delete dead slime
                lev.slmE.erase(lev.slmE.begin() + i); 
                // Add player score
                gl.score += 1000;
            }

        // Render the screen
        if (gl.introScreenFlag) {
            showIntroScreen();
            x11.swapBuffers();
        } else if(gl.render) {
            render();
            x11.swapBuffers();
        }
    }
    cleanup_fonts();
    return 0;
}

void initOpengl(void)
{
    //OpenGL initialization
    glGenTextures(1, &gl.tristanTexture);
    glViewport(0, 0, gl.xres, gl.yres);
    //Initialize matrices
    glMatrixMode(GL_PROJECTION); glLoadIdentity();
    glMatrixMode(GL_MODELVIEW); glLoadIdentity();
    //This sets 2D mode (no perspective)
    glOrtho(0, gl.xres, 0, gl.yres, -1, 1);
    //
    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_FOG);
    glDisable(GL_CULL_FACE);
    //
    //Clear the screen
    glClearColor(1.0, 1.0, 1.0, 1.0);
    //glClear(GL_COLOR_BUFFER_BIT);
    //Do this to allow fonts
    glEnable(GL_TEXTURE_2D);
    initialize_fonts();

    // Load the intro screen background texture
    loadTexture(&gl.introScreenTexture, "./images/intro_background.jpg");

    // Load the intro screen title texture
    loadTextureAlpha(&gl.introTitleTexture, "./images/intro_title.png");

    // Load the intro press space to begin texture
    loadTextureAlpha(&gl.introPressSpaceTexture, "./images/intro_press_space.png");

    // Load the character texture
    loadTextureAlpha(&gl.walkTexture, "./images/walk.gif");

    // Load the first explosion texture
    loadTextureAlpha(&gl.exp.tex, "./images/exp.png");

    // Load the second explosion texture
    loadTextureAlpha(&gl.exp44.tex, "./images/exp44.png");

    // Load the background texture
    loadTexture(&gl.backgroundTexture, "./textures/blue-tile.jpg");

    // Load the banana texture
    loadTextureAlpha(&gl.bananaTexture, "./images/banana2.png");

    // Load tile textures
    loadTexture(&gl.tileTexture[0], "./textures/gray1.jpg");
    loadTexture(&gl.tileTexture[1], "./textures/grass_32x32.png");

    // Load Kenny's credit screen texture
    loadTexture(&gl.kennyCreditsTexture, "./images/bob.jpg");

    // Load Tristan's credit screen texture
    loadTexture(&gl.tristanTexture, "./images/resize_Cactuar.png");

    // Load Rudy's credit screen texture
    loadTexture(&gl.rudyTexture, "./images/resize_turtle.jpg");

    // Load the torch texture
    loadTextureAlpha(&gl.torchTexture, "./images/torch.jpg");

    // Load the slime texture
    loadTextureAlpha(&gl.slimeTexture, "./images/slime1.png");

    // Load the Zombie texture
    loadTextureAlpha(&gl.zombieTexture, "./images/zambie.jpg");

    // Load the Enemy Slime texture
    loadTextureAlpha(&gl.slimeEnemyTexture, "./images/slimeEnemy.png");

    // Load the end screen texture
    loadTextureAlpha(&gl.endScreenTexture, "./images/end_screen.jpg");

    // Load the end screen 'press space' texture
    loadTextureAlpha(&gl.endScreenPressSpaceTexture, "./images/end_press_space.png");

    // Load the Door Texture
    loadTexture(&gl.doorTexture, "./images/door.jpg");

    // Load the initials screen texture
    loadTexture(&gl.initialTexture, "./images/initials.png");

    // Load the high scores screen texture
    loadTexture(&gl.highScoreTexture, "./images/high_scores.png");
}

void init() {

}

void checkMouse(XEvent *e)
{
    //printf("checkMouse()...\n"); fflush(stdout);
    //Did the mouse move?
    //Was a mouse button clicked?
    static int savex = 0;
    static int savey = 0;
    //
    if (e->type != ButtonRelease && e->type != ButtonPress &&
            e->type != MotionNotify)
        return;
    if (e->type == ButtonRelease) {
        return;
    }
    if (e->type == ButtonPress) {
        if (e->xbutton.button==1) {
            //Left button is down
        }
        if (e->xbutton.button==3) {
            //Right button is down
        }
    }
    if (e->type == MotionNotify) {
        if (savex != e->xbutton.x || savey != e->xbutton.y) {
            //Mouse moved
            savex = e->xbutton.x;
            savey = e->xbutton.y;
        }
    }
}

void screenCapture()
{
    static int fnum = 0;
    static int vid = 0;
    if (!vid) {
        system("mkdir ./vid");
        vid = 1;
    }
    unsigned char *data = (unsigned char *)malloc(gl.xres * gl.yres * 3);
    glReadPixels(0, 0, gl.xres, gl.yres, GL_RGB, GL_UNSIGNED_BYTE, data);
    char ts[32];
    sprintf(ts, "./vid/pic%03i.ppm", fnum);
    FILE *fpo = fopen(ts,"w");	
    if (fpo) {
        fprintf(fpo, "P6\n%i %i\n255\n", gl.xres, gl.yres);
        unsigned char *p = data;
        //go backwards a row at a time...
        p = p + ((gl.yres-1) * gl.xres * 3);
        unsigned char *start = p;
        for (int i=0; i<gl.yres; i++) {
            for (int j=0; j<gl.xres*3; j++) {
                fprintf(fpo, "%c",*p);
                ++p;
            }
            start = start - (gl.xres*3);
            p = start;
        }
        fclose(fpo);
        char s[256];
        sprintf(s, "convert ./vid/pic%03i.ppm ./vid/pic%03i.gif", fnum, fnum);
        system(s);
        unlink(ts);
    }
    ++fnum;
}

int checkKeys(XEvent *e)
{
    pair<int,int> playerLoc = pl->getPos();

    //keyboard input?
    static int shift=0;
    if (e->type != KeyPress && e->type != KeyRelease)
        return 0;
    int key = XLookupKeysym(&e->xkey, 0);
    gl.keys[key]=1;
    if (e->type == KeyRelease) {
        gl.keys[key]=0;
        if (key == XK_Shift_L || key == XK_Shift_R)
            shift=0;
        return 0;
    }
    gl.keys[key]=1;
    if (key == XK_Shift_L || key == XK_Shift_R) {
        shift=1;
        return 0;
    }
    (void)shift;
    switch (key) {
        case XK_a:
            if (gl.initialScreenFlag)
                pushInitials('A');
            break;
        case XK_b:
            if (gl.initialScreenFlag)
                pushInitials('B');
            break;
        case XK_c: //Credits key is C.
            if (gl.initialScreenFlag)
                pushInitials('C');
            else
                gl.creditsFlag = !gl.creditsFlag;
            break;
        case XK_d:
            if (gl.initialScreenFlag)
                pushInitials('D');
            break;
        case XK_e:
            if (gl.initialScreenFlag)
                pushInitials('E');
            else {
                // Enter the door, if player is standing in front of one
                for (unsigned int i = 0; i < lev.dor.size(); i++)
                    if (pl->getHitbox().isColliding(lev.dor[i].getHitbox()))
                        lev.dor[i].loadDest();
            }
            break;
        case XK_f:
            if (gl.initialScreenFlag)
                pushInitials('F');
            break;
        case XK_g:
            if (gl.initialScreenFlag)
                pushInitials('G');
            break;
        case XK_h:
            if (gl.initialScreenFlag)
                pushInitials('H');
            break;
        case XK_i:
            if (gl.initialScreenFlag)
                pushInitials('I');
            break;
        case XK_j:
            if (gl.initialScreenFlag)
                pushInitials('J');
            break;
        case XK_k:
            if (gl.initialScreenFlag)
                pushInitials('K');
            break;
        case XK_l:
            if (gl.initialScreenFlag)
                pushInitials('L');
            else {
                gl.fetchLeaders = 1;
                gl.leaderboardFlag = !gl.leaderboardFlag;
            }
            break;
        case XK_m:
            if (gl.initialScreenFlag)
                pushInitials('M');
            else
                gl.movie ^= 1;
            break;
        case XK_n:
            if (gl.initialScreenFlag)
                pushInitials('N');
            break;
        case XK_o:
            if (gl.initialScreenFlag)
                pushInitials('O');
            break;
        case XK_p:
            if (gl.initialScreenFlag)
                pushInitials('P');
            if(!gl.initialScreenFlag)
                gl.paused = !gl.paused;
            break;
        case XK_q:
            if (gl.initialScreenFlag)
                pushInitials('Q');
            break;
        case XK_r:
            if (gl.initialScreenFlag)
                pushInitials('R');
            break;
        case XK_s:
            if (gl.initialScreenFlag)
                pushInitials('S');
            else
                screenCapture();
            break;
        case XK_t:
            if (gl.initialScreenFlag)
                pushInitials('T');
            break;
        case XK_u:
            if (gl.initialScreenFlag)
                pushInitials('U');
            break;
        case XK_v:
            if (gl.initialScreenFlag)
                pushInitials('V');
            break;
        case XK_w:
            if (gl.initialScreenFlag)
                pushInitials('W');
            break;
        case XK_x:
            if (gl.initialScreenFlag)
                pushInitials('X');
            break;
        case XK_y:
            if (gl.initialScreenFlag)
                pushInitials('Y');
            break;
        case XK_z:
            if (gl.initialScreenFlag)
                pushInitials('Z');
            break;

        case XK_1:
            gl.render=0;
            loadLevel(&lev, 1);
            gl.render=1;
            break;
        case XK_2:
            gl.render = 0;
            loadLevel(&lev, 2); 
            gl.render = 1;
            break;
        case XK_3:
            gl.render = 0;
            loadLevel(&lev, 3);
            gl.render = 1;
            break;
        case XK_Left:
            break;
        case XK_Right:
            break;
        case XK_Up:
            break;
        case XK_Down:
            break;
        case XK_equal:
            gl.delay -= 0.005;
            if (gl.delay < 0.005)
                gl.delay = 0.005;
            break;
        case XK_minus:
            gl.delay += 0.005;
            break;
        case XK_Escape:
            return 1;
            break;
        case XK_Return:
            cout << "bullet created at (" << playerLoc.first << ", " << playerLoc.second << ")" << endl;
            lev.bullet.push_back(*(new Bullet(playerLoc.first, playerLoc.second, 10 * gl.lastFacing)));
            break;
        case XK_space:
            if (!gl.leaderboardFlag && !gl.endScreenFlag)
                pl->Jump();
            // Leave the intro screen
            if (gl.introScreenFlag) {
                gl.introScreenFlag = !gl.introScreenFlag;
                gl.paused = 0;
            }
            // Leave the end screen
            if (gl.endScreenFlag) {
                gl.endScreenFlag = !gl.endScreenFlag;
                gl.initialScreenFlag = 1;
            }
            if (gl.leaderboardFlag) {
                gl.leaderboardFlag = 0;
                loadLevel(&lev, 1);
                gl.score = 0;
                gl.paused = 0;
                gl.render = 1;
            }
            break;
    }
    return 0;
}

Flt VecNormalize(Vec vec)
{
    Flt len, tlen;
    Flt xlen = vec[0];
    Flt ylen = vec[1];
    Flt zlen = vec[2];
    len = xlen*xlen + ylen*ylen + zlen*zlen;
    if (len == 0.0) {
        MakeVector(vec, 0.0, 0.0, 1.0);
        return 1.0;
    }
    len = sqrt(len);
    tlen = 1.0 / len;
    vec[0] = xlen * tlen;
    vec[1] = ylen * tlen;
    vec[2] = zlen * tlen;
    return(len);
}

void physics(void)
{
    struct timespec ts, te;
    // Record the start time
    clock_gettime(CLOCK_REALTIME, &ts);

    // ========================================
    // Move the background and foreground tiles 
    // ========================================
    if (gl.walk || gl.keys[XK_Right] || gl.keys[XK_Left]
            || gl.keys[XK_a] || gl.keys[XK_d]) {
        //man is walking...
        //when time is up, advance the frame.
        timers.recordTime(&timers.timeCurrent);
        double timeSpan = timers.timeDiff(&timers.walkTime, &timers.timeCurrent);
        if (timeSpan > gl.delay) {
            //advance
            ++gl.walkFrame;
            if (gl.walkFrame >= 16)
                gl.walkFrame -= 16;
            timers.recordTime(&timers.walkTime);
        }

        if (gl.keys[XK_Left] || gl.keys[XK_a]) {
            pl->moveLeft();

        } else if (gl.camera[0] <= (lev.ncols * lev.tilesize[0] - gl.xres/2)) {
            pl->moveRight();		
        }

        // Move the explosions
        if (gl.exp.onoff) {
            gl.exp.pos[0] -= 2.0 * (0.05 / gl.delay);
        }	
        if (gl.exp44.onoff) {
            gl.exp44.pos[0] -= 2.0 * (0.05 / gl.delay);
        }
    }

    // ==========================
    // Handle explosion animation
    // ==========================

    if (gl.exp.onoff) {
        //explosion is happening
        timers.recordTime(&timers.timeCurrent);
        double timeSpan = timers.timeDiff(&gl.exp.time, &timers.timeCurrent);
        if (timeSpan > gl.exp.delay) {
            //advance explosion frame
            ++gl.exp.frame;
            if (gl.exp.frame >= 23) {
                //explosion is done.
                gl.exp.onoff = 0;
                gl.exp.frame = 0;
            } else {
                timers.recordTime(&gl.exp.time);
            }
        }
    }
    if (gl.exp44.onoff) {
        //explosion is happening
        timers.recordTime(&timers.timeCurrent);
        double timeSpan = timers.timeDiff(&gl.exp44.time, &timers.timeCurrent);
        if (timeSpan > gl.exp44.delay) {
            //advance explosion frame
            ++gl.exp44.frame;
            if (gl.exp44.frame >= 16) {
                //explosion is done.
                gl.exp44.onoff = 0;
                gl.exp44.frame = 0;
            } else {
                timers.recordTime(&gl.exp44.time);
            }
        }
    }

    //====================================
    //Adjust position of ball.
    //Height of highest tile when ball is?
    //====================================
    Flt dd = lev.ftsz[0];
    int col = (int)((gl.camera[0]+gl.ball_pos[0]) / dd);
    col = col % lev.ncols;
    int hgt = 0;
    for (int i=0; i<lev.nrows; i++) {
        if (lev.arr[i][col] != ' ') {
            hgt = (lev.nrows-i) * lev.tilesize[1];
            break;
        }
    }
    if (gl.ball_pos[1] < (Flt)hgt) {
        gl.ball_pos[1] = (Flt)hgt;
        MakeVector(gl.ball_vel, 0, 0, 0);
    } else {
        gl.ball_vel[1] -= 0.9;
    }
    gl.ball_pos[1] += gl.ball_vel[1];

    // =========================
    // Handle the Player
    // =========================
    pl->physics();

    // =========================
    // Handle the banana objects
    // =========================
    for (unsigned int i = 0; i < lev.ban.size(); i++)
        lev.ban[i].physics();

    // =========================
    // Handle the zombie objects
    // =========================
    for (unsigned int i = 0; i < lev.zmb.size(); i++)
        lev.zmb[i].physics();

    // =========================
    // Handle the SlimeE objects
    // =========================
    for (unsigned int i = 0; i < lev.slmE.size(); i++)
        lev.slmE[i].physics();

    // =========================
    // Handle the bullet objects
    // =========================
    for (unsigned int i = 0; i < lev.bullet.size(); i++)
        lev.bullet[i].physics();

    // Record the physics time
    clock_gettime(CLOCK_REALTIME, &te);
    gl.physicsTime = timeDiff(&ts, &te);
}

void render()
{
    struct timespec ts;
    // Record the start time
    clock_gettime(CLOCK_REALTIME, &ts);

    // =======================
    // Clear the screen
    // =======================
    glClearColor(0.1, 0.1, 0.1, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);
    float cx = gl.xres/2.0;
    float cy = gl.yres/2.0;

    // ===========================================
    // Draw textured background (256x256 tiles)
    // ===========================================
    drawTexturedBackground();

    // =======================
    // Render the ground
    // =======================
    glBegin(GL_QUADS);
    glColor3f(0.2, 0.2, 0.2);
    glVertex2i(0,       220);
    glVertex2i(gl.xres, 220);
    glColor3f(0.4, 0.4, 0.4);
    glVertex2i(gl.xres,   0);
    glVertex2i(0,         0);
    glEnd();

    // =================================
    // Draw the foreground tile system
    // =================================
    int tx = lev.tilesize[0];
    //int ty = lev.tilesize[1];
    Flt dd = lev.ftsz[0];
    Flt offy = lev.tile_base;
    int ncols_to_render = gl.xres / lev.tilesize[0] + 2;
    int col = (int)(gl.camera[0] / dd);
    col = col % lev.ncols;
    //Partial tile offset must be determined here.
    //The leftmost tile might be partially off-screen.
    //cdd: camera position in terms of tiles.
    Flt cdd = gl.camera[0] / dd;
    //flo: just the integer portion
    Flt flo = floor(cdd);
    //dec: just the decimal portion
    Flt dec = (cdd - flo);
    //offx: the offset to the left of the screen to start drawing tiles
    Flt offx = -dec * dd;
    //Log("gl.camera[0]: %lf   offx: %lf\n",gl.camera[0],offx);
    for (int j=0; j<ncols_to_render; j++) {
        int row = lev.nrows-1;
        for (int i=0; i<lev.nrows; i++) {
            // Draw white tile
            if (lev.arr[row][col] == 'w')
                drawSolidTile((Flt)j*dd+offx, (Flt)i*lev.ftsz[1]+offy, tx, 0.8, 0.8, 0.6);
            // Draw red tile
            else if (lev.arr[row][col] == 'r')
                drawSolidTile((Flt)j*dd+offx, (Flt)i*lev.ftsz[1]+offy, tx, 0.9, 0.2, 0.2);
            // Draw blue tile
            else if (lev.arr[row][col] == 'b')
                drawSolidTile((Flt)j*dd+offx, (Flt)i*lev.ftsz[1]+offy, tx, 0.2, 0.2, 0.9);
            // Draw green tile
            else if (lev.arr[row][col] == 'g')
                drawSolidTile((Flt)j*dd+offx, (Flt)i*lev.ftsz[1]+offy, tx, 0.0, 0.6, 0.0);
            // Draw purple tile
            else if (lev.arr[row][col] == 'p')
                drawSolidTile((Flt)j*dd+offx, (Flt)i*lev.ftsz[1]+offy, tx, 0.5, 0.0, 0.5);
            // Draw yellow tile
            else if (lev.arr[row][col] == 'y')
                drawSolidTile((Flt)j*dd+offx, (Flt)i*lev.ftsz[1]+offy, tx, 1.0, 1.0, 0.0);
            // Draw dark gray tile
            else if (lev.arr[row][col] == 'z')
                drawSolidTile((Flt)j*dd+offx, (Flt)i*lev.ftsz[1]+offy, tx, 0.4, 0.4, 0.4);
            // Draw black tile
            else if (lev.arr[row][col] == 'k')
                drawSolidTile((Flt)j*dd+offx, (Flt)i*lev.ftsz[1]+offy, tx, 0.1, 0.1, 0.1);
            // Draw textured tiles
            else if(lev.arr[row][col] >= '0' && lev.arr[row][col] <= '9')
                drawTexturedTile((int)lev.arr[row][col] - (int)'0', (Flt)j*dd+offx, (Flt)i*lev.ftsz[1]+offy, tx);
            --row;

        }
        col = (col+1) % lev.ncols;

    }

    // =================
    // Draw Door objects
    // =================
    for (unsigned int i = 0; i < lev.dor.size(); i++)
        lev.dor[i].render();

    // =====================
    // Draw wall decorations
    // =====================
    drawWallDecorations(dd, ncols_to_render, offx, offy);

    // ========================
    // Draw the ball (disabled)
    // ========================
    //glColor3f(1.0, 1.0, 0.1);
    //glPushMatrix();
    //glTranslated(gl.ball_pos[0], lev.tile_base+gl.ball_pos[1], 0);
    //glBegin(GL_QUADS);
    //	glVertex2i(-10, 0);
    //	glVertex2i(-10, 20);
    //	glVertex2i( 10, 20);
    //	glVertex2i( 10, 0);
    //glEnd();
    //glPopMatrix();

    // =========================
    // Draw shadow
    // =========================
    //#define SHOW_FAKE_SHADOW
#ifdef SHOW_FAKE_SHADOW
    glColor3f(0.25, 0.25, 0.25);
    glBegin(GL_QUADS);
    glVertex2i(cx-60, 150);
    glVertex2i(cx+50, 150);
    glVertex2i(cx+50, 130);
    glVertex2i(cx-60, 130);
    glEnd();
#endif

    // ===================
    // Draw Banana objects
    // ===================
    for (unsigned int i = 0; i < lev.ban.size(); i++)
        lev.ban[i].render();

    // ===================
    // Draw SlimeE objects
    // ===================
    for (unsigned int i = 0; i < lev.slmE.size(); i++)
        lev.slmE[i].render();

    // ===================
    // Draw Zombie objects
    // ===================
    for (unsigned int i = 0; i < lev.zmb.size(); i++)
        lev.zmb[i].render();

    // =================
    // Draw Door objects
    // =================
    //for (unsigned int i = 0; i < lev.dor.size(); i++)
    //    lev.dor[i].render();

    // ================
    // Draw bullets
    // ================
    for (unsigned int i = 0; i < lev.bullet.size(); i++)
        lev.bullet[i].render();

    // =====================
    // Draw character sprite
    // =====================
    pl->render();

    // =================
    // Render explosions
    // =================
    float h, w;
    if (gl.exp.onoff) {
        h = 80.0;
        w = 80.0;
        glPushMatrix();
        glColor3f(1.0, 1.0, 1.0);
        glBindTexture(GL_TEXTURE_2D, gl.exp.tex);
        glEnable(GL_ALPHA_TEST);
        glAlphaFunc(GL_GREATER, 0.0f);
        glColor4ub(255,255,255,255);
        glTranslated(gl.exp.pos[0], gl.exp.pos[1], gl.exp.pos[2]);
        int ix = gl.exp.frame % 5;
        int iy = gl.exp.frame / 5;
        float tx = (float)ix / 5.0;
        float ty = (float)iy / 5.0;
        glBegin(GL_QUADS);
//        glTexCoord2f(tx,     ty+0.2); glVertex2i(cx-w, cy-h);
//        glTexCoord2f(tx,     ty);     glVertex2i(cx-w, cy+h);
//        glTexCoord2f(tx+0.2, ty);     glVertex2i(cx+w, cy+h);
//        glTexCoord2f(tx+0.2, ty+0.2); glVertex2i(cx+w, cy-h);
        glTexCoord2f(tx,     ty+0.2); glVertex2i(-w, -h);
        glTexCoord2f(tx,     ty);     glVertex2i(-w, h);
        glTexCoord2f(tx+0.2, ty);     glVertex2i(w, h);
        glTexCoord2f(tx+0.2, ty+0.2); glVertex2i(w, -h);

        glEnd();
        glPopMatrix();
        glBindTexture(GL_TEXTURE_2D, 0);
        glDisable(GL_ALPHA_TEST);
    }
    //
    if (gl.exp44.onoff) {
        h = 80.0;
        w = 80.0;
        glPushMatrix();
        glColor3f(1.0, 1.0, 1.0);
        glBindTexture(GL_TEXTURE_2D, gl.exp44.tex);
        glEnable(GL_ALPHA_TEST);
        glAlphaFunc(GL_GREATER, 0.0f);
        glColor4ub(255,255,255,255);
        glTranslated(gl.exp44.pos[0], gl.exp44.pos[1], gl.exp44.pos[2]);
        int ix = gl.exp44.frame % 4;
        int iy = gl.exp44.frame / 4;
        float tx = (float)ix / 4.0;
        float ty = (float)iy / 4.0;
        glBegin(GL_QUADS);
        glTexCoord2f(tx,      ty+0.25); glVertex2i(cx-w, cy-h);
        glTexCoord2f(tx,      ty);      glVertex2i(cx-w, cy+h);
        glTexCoord2f(tx+0.25, ty);      glVertex2i(cx+w, cy+h);
        glTexCoord2f(tx+0.25, ty+0.25); glVertex2i(cx+w, cy-h);
        glEnd();
        glPopMatrix();
        glBindTexture(GL_TEXTURE_2D, 0);
        glDisable(GL_ALPHA_TEST);
    }

    // ===============
    // Draw the UI/HUD
    // ===============
    drawHUD(ts);

    // =======================
    // Draw the credits screen
    // =======================
    if (gl.creditsFlag)
        show_credits();

    // ===========================
    // Draw the leaderboard screen
    // ===========================
    if (gl.leaderboardFlag)
        showLeaderboard();

    // ===========================
    // Draw the end screen
    // ===========================
    if (gl.endScreenFlag)
        showEndScreen();

    // ===========================
    // Draw the initials screen
    // ===========================
    if (gl.initialScreenFlag)
        showInitialScreen();

    if (gl.movie) {
        screenCapture();
    }
}

void show_credits()
{
    // Draw background rectangle (center in viewport)
    glPushMatrix();
    glColor3ub(255,255,255);
    glBegin(GL_QUADS);
    glVertex2i(gl.xres/2-300, gl.yres/2-200);
    glVertex2i(gl.xres/2+300, gl.yres/2-200);
    glVertex2i(gl.xres/2+300, gl.yres/2+200);
    glVertex2i(gl.xres/2-300, gl.yres/2+200);
    glEnd();
    glPopMatrix();

    // Draw the credits title
    drawText(gl.xres/2-20, gl.yres/2+170, 0x004040ff, (char *)"Game Credits");

    // Draw individual text
    Rect r;
    r.bot = gl.yres/2+100;
    r.left = gl.xres/2-200;
    tristanCredits(&r);

    showKennyCredits(gl.xres/2-200, gl.yres/2);

    r.bot = gl.yres/2-100;
    r.left = gl.xres/2-200;
    rudyCredits(&r);

    // Draw individual images
    showKennyImage(gl.xres/2+200, gl.yres/2, gl.kennyCreditsTexture);
    tristanImage(gl.xres/2, gl.yres/2+75, gl.tristanTexture);
    showRudyPicture(gl.xres/2, gl.yres/2-100, gl.rudyTexture);
}
