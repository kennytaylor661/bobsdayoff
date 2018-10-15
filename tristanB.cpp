//Name: Tristan Bock
//Date: 2018-09-25
#include "tristanB.h"

Enemy::Enemy(int x, int y): posX(x), posY(y){}

   void Enemy::moveLeft()
    {
        posX--;
    }

    void Enemy::moveRight()
    {
        posX++;
    }

    bool Enemy::findPlayer()
    {
        //TBD
    }

    void Enemy::takeAction()
    {
        //stuff
    }

void tristanCredits(Rect* r)
{
	ggprint8b(r, 16, 0x00ff44ff, "Tristan Bock");
}

void tristanImage(int x, int y, GLuint texid)
{
	glColor3ub(255,255,255);
	int wid = 80;
	glPushMatrix();
	glTranslatef(x, y, 0);
	glBindTexture(GL_TEXTURE_2D, texid);
	glBegin(GL_QUADS);
		glTexCoord2f(0.0f, 1.0f); glVertex2i(-wid, -wid);
		glTexCoord2f(0.0f, 0.0f); glVertex2i(-wid,  wid);
		glTexCoord2f(1.0f, 0.0f); glVertex2i( wid,  wid);
		glTexCoord2f(1.0f, 1.0f); glVertex2i( wid, -wid);
	glEnd();
	glPopMatrix();
}
