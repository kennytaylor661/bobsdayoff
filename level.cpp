
#include <stdio.h>
#include <string.h>
#include "global.h"
#include "level.h"

extern Global gl;

Level::Level() {
    load((char *)"level1.txt");
}

void Level::load(char *filename)
{
    //Log("Level constructor\n");
    tilesize[0] = 32; 
    tilesize[1] = 32; 
    ftsz[0] = (Flt)tilesize[0];
    ftsz[1] = (Flt)tilesize[1];
    tile_base = 140.0;
    //read level
    //FILE *fpi = fopen("level1.txt","r");
    FILE *fpi = fopen(filename, "r");
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

    // Set player initial location of (gl.xres/2, ?)

    // Clear the vector of SomeObject objects
    someobject.clear();
    // Create a few collectable SomeObject objects
    SomeObject *so;
    so = new SomeObject(100, 100, gl.someObjectTexture);
    someobject.push_back(*so);
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