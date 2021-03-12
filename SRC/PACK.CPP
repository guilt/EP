#include "headers.h"

#define P_OK 0
#define P_O_NW -1
#define P_I_NR -2

int fcount = 0, ini = 0, flag = 0, dflag;
unsigned int fnlen;
long flen;

int pack(char *fn1) {
  ifsize = 0;
  dflag = 0;
  infile.open(fn1, ios::in | ios::binary);
  if (!infile) {
    return P_I_NR;
  }
  infile.seekg(0, ios::end);
  flen = infile.tellg();
  ifsize = flen;
  infile.seekg(0, ios::beg);
  fnlen = strlen(fn1);
  outfile.write((char *)&fnlen, sizeof(int));
  outfile.write(fn1, fnlen);
  outfile.write((char *)&dflag, sizeof(int));
  outfile.write((char *)&flen, sizeof(long));
  icnt = ifsize / BUFSIZE;
  incnt = ifsize % BUFSIZE;
  for (i = 0; i < icnt; i++) {
    infile.read(buffer, BUFSIZE);
    outfile.write(buffer, BUFSIZE);
  }
  infile.read(buffer, incnt);
  outfile.write(buffer, incnt);
  infile.close();
  fcount++;
  cout << "\nPacked : " << fn1 << " : " << ifsize << " Bytes !";
  return P_OK;
}

int dpack(char *fn1) {
  dflag = 1;
  fnlen = strlen(fn1);
  outfile.write((char *)&fnlen, sizeof(int));
  outfile.write(fn1, fnlen);
  outfile.write((char *)&dflag, sizeof(int));
  fcount++;
  cout << "\nMade : " << fn1 << "\\";
  return P_OK;
}

int init(char *fn) {
  outfile.open(fn, ios::out | ios::binary);
  if (!outfile) {
    return P_O_NW;
  }
  outfile << "$$$$";
  ini = 1;
  return P_OK;
}

void quit() {
  outfile.seekp(0, ios::beg);
  outfile << "PF";
  outfile.write((char *)&fcount, sizeof(int));
  outfile.close();
  ini = 0;
}

void main() {
  char c = '0';
  char fname[80], pfname[80];
  clrscr();
  cout << "\nEnter Packing Filename :";
  cin >> pfname;
  flag = init(pfname);
  if (flag == P_O_NW) {
    return;
  }
  do {
    cout << "\n1.Add File 2.Make Directory 3.Quit => Choice :";
    cin >> c;
    if (c == '1') {
      cout << "\nEnter Filename to Add :";
      cin >> fname;
      flag = pack(fname);
    } else if (c == '2') {
      cout << "\nEnter Directory to Add :";
      cin >> fname;
      flag = dpack(fname);
    }
  } while (c != '3');
  quit();
}