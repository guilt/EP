#ifndef IHEADERS
#define IHEADERS

#include "conio.h"
#include "dir.h"
#include "fstream.h"
#include "string.h"

#define BUFSIZE 4096

char buffer[BUFSIZE];
fstream infile, outfile;
long ifsize = 0, icnt = 0, incnt = 0, i;

#endif