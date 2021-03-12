#include "copy.h"
#include "extract.h"

void main(int argc, char *argv[]) {
  if (argc == 2) {
    int f = extract(argv[1]);
    if (f == E_OK) {
      cout << "\nDelivered Properly !";
    }
    if (f != E_OK) {
      cout << "\nExtraction Error !";
    }
  } else {
    cout << "\nFormat is : " << argv[0] << " PackedFilename";
  }
  getch();
}
