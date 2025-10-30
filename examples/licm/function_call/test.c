int add(int a, int b) { return a + b; }
int f(int N) {
  int x = 5;
  for (int i = 0; i < N; ++i) {
    int z = add(x, x);
  }
  return x;
}