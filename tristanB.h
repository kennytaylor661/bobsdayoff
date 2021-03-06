#ifndef TRISTAN_H
#define TRISTAN_H

#include "fonts.h"
#include <GL/glx.h>
#include <utility>
#include "global.h"
extern Global gl;

struct Hitbox
{
	float top, bottom, left, right;
	
	Hitbox() {top = 0; bottom = 0; left = 0; right = 0;}

	Hitbox(float t, float b, float l, float r):
		top(t), bottom(b), left(l), right(r){}

    bool isColliding(Hitbox other)
    {
        return ((left <= other.right && right >= other.left)  ||  
                (right >= other.left && left <= other.right)) &&
               ((bottom <= other.top && top >= other.bottom)  ||  
                (top >= other.bottom && bottom <= other.top));
    }
    
    void draw(int yOffset)
    {
        glPushMatrix();
        glColor3f(1.0, 0.0, 0.0); 
        glTranslated(0 - gl.camera[0], 0 + yOffset, 0); 
        glBegin(GL_POLYGON);
            glVertex2i(left, bottom);  
            glVertex2i(left, top); 
            glVertex2i(right, top); 
            glVertex2i(right,  bottom);  
        glEnd();
        glPopMatrix();
    }
};

class Player
{
	int HP = 100, wid;
    int height = 256, width = 128;
	bool grounded;
	float posX = gl.xres/2, posY = 0.0f, xvel = 0, yvel = 0;
	GLuint texid;
	Hitbox hitbox;
	int iframes = 0;
	public:
	Player()
	{
		hitbox.top = posY + height;
		hitbox.bottom = posY;
		hitbox.left = posX - width/2;
		hitbox.right = posX + width/2;
	}

	int bananaCount = 0;
	int getHP(){return this->HP;}
	std::pair<int, int> getPos();
    void resetPos();
	void moveLeft();
	void moveRight();
	void Jump();
	void Fire();
	void physics();
	void render();
	Hitbox getHitbox(){return this->hitbox;}
};

class Enemy
{
	protected:
		float posX, posY, xvel = 0, yvel = 0, yOffset, yOffsetVel = 2;
        int width, height;
        Hitbox hitbox;

	public:
		Enemy(float x, float y): posX(x), posY(y) {}
		~Enemy(){}

        int HP = 3;
		void moveLeft();
		void moveRight();
		virtual void AI(Player p) = 0;
		void physics();
		std::pair<int, int> getPos();
		virtual void render() = 0;
        Hitbox getHitbox(){return this->hitbox;}
};

class Slime : public Enemy
{
	GLuint texid = gl.slimeEnemyTexture;

	public:
	Slime(float x, float y): Enemy(x, y)
	{
        width = 80;
        height = 80;

		hitbox.top = posY + height/2;
		hitbox.bottom = posY - height/2;
		hitbox.left = posX - width/2;
		hitbox.right = posX + width/2;
	}
	//int HP = 1;
	int damage = 10;
	void AI(Player p);
	void render();
	Hitbox getHitbox(){return this->hitbox;}
};
class Zombie : public Enemy
{
	GLuint texid = gl.zombieTexture;
    int lastFacing;

	public:
	Zombie(float x, float y): Enemy(x, y)
	{
        width = 100;
        height = 200;

		hitbox.top = posY + height/2;
		hitbox.bottom = posY - height/2;
		hitbox.left = posX - width/2;
		hitbox.right = posX + width/2;
	}
	//int HP = 5;
	int damage = 10;
	void AI(Player p);
	void render();
	Hitbox getHitbox(){return this->hitbox;}
};

class Door
{
    int posX, posY, width, height;
    int dest;
    GLuint texid = gl.doorTexture;
    Hitbox hitbox;

    public:
        Door(int, int, int, int, int);
        void loadDest();
        void render();
        Hitbox getHitbox(){return this->hitbox;}
};

void tristanCredits(Rect* r);
void tristanImage(int x, int y, GLuint texid);

#endif
