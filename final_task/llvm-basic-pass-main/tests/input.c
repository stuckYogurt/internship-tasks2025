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
  int a = 6, b = 7;
  for (int i = 0; i < 4; i++) {
    int j = i;
    do {

      foo(a*b+a);
      j++;
    } while (j < 5);
  }
  return 2;
}

int test_multiple_invariants(int n) {
    int sum = 0;
    for (int i = 0; i < n; i++) {
        int a = 5 + 3;
        int b = 10 * 2;
        int c = a + b;
        sum += c;
    }
    return sum;
}

int test_control_flow_invariants(int n) {
    int result = 0;
    int invariant_calc = (100 - 50) / 2;  // Should be hoisted: 25
    
    for (int i = 0; i < n; i++) {
        if (i % 2 == 0) {
            continue;
        }
        
        int temp = invariant_calc * 2;  // Should be hoisted: 25*2=50
        result += temp;
        
        if (result > 1000) {
            break;
        }
    }
    return result;
}


int
main(int argc, char *argv[]) {
  int a = 123;
  int ret = 0;

  ret += foo(a);
  ret += poo();

  return ret;
}
