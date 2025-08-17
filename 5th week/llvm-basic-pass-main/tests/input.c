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

int
bar(int a, int b) {
  if (a == 2) {
    a = 3;
  }

  do {
    if (a == 4) {
      return a;
    } else {
      continue;
    }
    b ++;
  } while (b < 5);

  return (a + foo(b) * 2);
}

int
fez(int a, int b, int c) {
  return (a + bar(a, b) * 2 + c * 3);
}

int in_branches(int x, int y) {
    int result = 0;
    
    if (x > y) {
        result = x * 2;
    } else {
        result = y + 5;
    }
    
    for (int i = 0; i < result; i++) {
        if (i % 2 == 0) {
            result--;
        }
    }
    
    return result;
}

int complex_with_goto(int n) {
    int sum = 0;
    int i = 0, j = 0;
    
    if (n <= 0) return -1;
    
    while (i < n) {
        if (i % 3 == 0) {
            for (j = n; j > 0; j--) {
                sum += (i * j);
                if (sum > 1000) goto cleanup;
            }
        } else if (i % 2 == 0) {
            do {
                sum -= i;
                j++;
            } while (j < 5 && sum > 0);
        } else {
            sum += i;
        }
        
        i++;
    }
    
cleanup:
    if (sum < 0) {
        sum = 0;
        for (i = 0; i < 10; i++) {
            sum += i;
        }
    }
    
    return sum;
}

int
main(int argc, char *argv[]) {
  int a = 123;
  int ret = 0;

  ret += foo(a);
  ret += bar(a, ret);
  ret += fez(a, ret, 123);
  ret += in_branches(2, a);
  ret -= complex_with_goto(0);

  return ret;
}
