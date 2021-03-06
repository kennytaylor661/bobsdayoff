
#include <stdio.h>
#include <string.h>
#include "global.h"
#include "level.h"
#include <iostream>
using namespace std;

extern Global gl;
extern Player *pl;

Level::Level() {
    load(1);
}

void Level::load(int levelNum)
{
    FILE *fpi;

    //Log("Level constructor\n");
    tilesize[0] = 32; 
    tilesize[1] = 32; 
    ftsz[0] = (Flt)tilesize[0];
    ftsz[1] = (Flt)tilesize[1];
    tile_base = 140.0;
    //read level
    switch(levelNum) {
        case 1:
            fpi = fopen("level1.txt", "r");
            break;
        case 2:
            fpi = fopen("level2.txt", "r");
            break;
        case 3:
            fpi = fopen("level3.txt", "r");
            break;
    };
    if (fpi) {
        nrows=0;
        char line[200];
        while (fgets(line, 200, fpi) != NULL) {
            // Ignore comments
            if (line[0] != '#') {
                removeCrLf(line);
                int slen = strlen(line);
                ncols = slen;
                //Log("line: %s\n", line);
                for (int j=0; j<slen; j++) {
                    arr[nrows][j] = line[j];
                }
                ++nrows;
            }
        }
        fclose(fpi);
        //printf("nrows of background data: %i\n", nrows);
    }
    for (int i=0; i<nrows; i++) {
        for (int j=0; j<ncols; j++) {
            printf("%c", arr[i][j]);
        }
        printf("\n");
    }

    // Put player back at the start of the level
    gl.camera[0] = gl.camera[1] = 0.0;
    gl.backgroundXoffset = 0.0;
    pl->resetPos();

    // Clear the vector of Banana Objects
    ban.clear();

    // Create new banana objects
    Banana *b;
    srand(time(NULL));
    for(int i = 0; i < 20; i++) {
        b = new Banana((rand() % 80) * 50, (rand() % 200) + 300, gl.bananaTexture);
        ban.push_back(*b);
    }

    // Clear the vector of zombie objects
    zmb.clear();

    // Create new zombie enemy
    zmb.push_back(*(new Zombie(100, 400)));
    zmb.push_back(*(new Zombie(2000, 400)));
    zmb.push_back(*(new Zombie(3300, 400)));

    // Clear the vector of slime enemies
    slmE.clear();

    // Create new slime enemy
    slmE.push_back(*(new Slime(700, 100)));
    slmE.push_back(*(new Slime(1400, 100)));
    slmE.push_back(*(new Slime(3200, 100)));

    // Clear the vector of doors
    dor.clear();

    // Place doors
    switch(levelNum) {
        case 1:
            // Place door at end leading to level 2
            dor.push_back(*(new Door(3800, 64, 128, 256, 2)));
            break;
        case 2:
            // Place door at beginning leading to level 1
            dor.push_back(*(new Door(400, 64, 128, 256, 1)));
            // Place a door at end leading to level 3
            dor.push_back(*(new Door(3800, 64, 128, 256, 3)));
            break;
        case 3:
            // Place door at beginning leading to level 2
            dor.push_back(*(new Door(400, 64, 128, 256, 2)));
            break;
    };
}

void Level::removeCrLf(char *str) {
    //remove carriage return and linefeed from a Cstring
    char *p = str;
    while (*p) {
        if (*p == 10 || *p == 13) {
            *p = '\0';
            break;
        }
        ++p;
    }
}
