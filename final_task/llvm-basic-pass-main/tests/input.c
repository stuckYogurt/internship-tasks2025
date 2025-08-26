//=============================================================================
// FILE:
//      input.c
//
// DESCRIPTION:
//      Sample input file
//
// License: MIT
//=============================================================================
int
foo(int a) {
  return a * 2;
}

int poo() {
  for (int i = 0; i < 4; i++) {
    do {
      foo(i);
    } while (i < 5);
  }
  return 2;
}


int
main(int argc, char *argv[]) {
  int a = 123;
  int ret = 0;

  ret += foo(a);
  ret += poo();

  return ret;
}
