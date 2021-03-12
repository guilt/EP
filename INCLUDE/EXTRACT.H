#ifndef IEXTRACT
#define IEXTRACT

#include "headers.h"

#define E_OK 0
#define E_O_NW -1
#define E_I_NR -2
#define E_F_NF -3
#define E_I_BF -4

// Packed File .
int extract(char *fn1) {
  unsigned int j, k, fnlen, dflag;
  char *infname;
  long flen;
  infile.open(fn1, ios::in | ios::binary);
  if (!infile) {
    return E_I_NR;
  }
  if (infile.get() != 'P' || infile.get() != 'F') {
    infile.close();
    return E_I_BF;
  }
  infile.read((char *)&j, sizeof(int));
  cout << "\nExtracting From : " << fn1;
  cout << "\nTotal :" << j;
  for (k = 0; k < j; k++) {
    infile.read((char *)&fnlen, sizeof(int));
    infile.read(infname, fnlen);
    infname[fnlen] = '\0';
    infile.read((char *)&dflag, sizeof(int));
    if (dflag == 0) {
      infile.read((char *)&flen, sizeof(long));
      outfile.open(infname, ios::out | ios::binary);
      if (!outfile) {
        infile.close();
        return E_O_NW;
      }
      ifsize = flen;
      icnt = ifsize / BUFSIZE;
      incnt = ifsize % BUFSIZE;
      for (i = 0; i < icnt; i++) {
        infile.read(buffer, BUFSIZE);
        outfile.write(buffer, BUFSIZE);
      }
      infile.read(buffer, incnt);
      outfile.write(buffer, incnt);
      outfile.close();
      cout << "\nExtracted : " << infname << " : " << ifsize << " Bytes !";
    } else if (dflag == 1) {
      mkdir(infname);
      cout << "\nMade : " << infname << "\\";
    }
  }
  infile.close();
  return E_OK;
}

int specextract(char *fn1, char *fn2) {
  unsigned int j, k, fnlen, dflag;
  char *infname;
  long flen;
  infile.open(fn1, ios::in | ios::binary);
  if (!infile) {
    return E_I_NR;
  }
  if (infile.get() != 'P' || infile.get() != 'F') {
    infile.close();
    return E_I_BF;
  }
  cout << "\nSearching : " << fn1;
  infile.read((char *)&j, sizeof(int));
  for (k = 0; k < j; k++) {
    infile.read((char *)&fnlen, sizeof(int));
    infile.read(infname, fnlen);
    infname[fnlen] = '\0';
    infile.read((char *)&dflag, sizeof(int));
    if (dflag == 0) {
      infile.read((char *)&flen, sizeof(long));
      if (strcmp(infname, fn2) == 0) {
        outfile.open(infname, ios::out | ios::binary);
        if (!outfile) {
          infile.close();
          return E_O_NW;
        }
        ifsize = flen;
        icnt = ifsize / BUFSIZE;
        incnt = ifsize % BUFSIZE;
        for (i = 0; i < icnt; i++) {
          infile.read(buffer, BUFSIZE);
          outfile.write(buffer, BUFSIZE);
        }
        infile.read(buffer, incnt);
        outfile.write(buffer, incnt);
        cout << "\nExtracted : " << infname << " : " << ifsize << " Bytes !";
        outfile.close();
      } else if (dflag == 1) {
        mkdir(infname);
        cout << "\nMade : " << infname << "\\";
      }
      infile.close();
      return E_OK;
    } else {
      infile.seekg(infile.tellg() + flen, ios::beg);
    }
  }
  return E_F_NF;
}

#endif