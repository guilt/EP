#ifndef ICOPY
#define ICOPY

#include "headers.h"

#define C_OK 0
#define C_O_NW -1
#define C_I_NR -2

int copy(char *fn1, char *fn2) {
  infile.open(fn1, ios::in | ios::binary);
  if (!infile) {
    return C_I_NR;
  }
  outfile.open(fn2, ios::out | ios::binary);
  if (!outfile) {
    infile.close();
    return C_O_NW;
  }
  infile.seekg(0, ios::end);
  ifsize = infile.tellg();
  icnt = ifsize / BUFSIZE;
  incnt = ifsize % BUFSIZE;
  infile.seekg(0, ios::beg);
  for (i = 0; i < icnt; i++) {
    infile.read(buffer, BUFSIZE);
    outfile.write(buffer, BUFSIZE);
  }
  infile.read(buffer, incnt);
  outfile.write(buffer, incnt);
  outfile.close();
  infile.close();
  return C_OK;
}

#endif
