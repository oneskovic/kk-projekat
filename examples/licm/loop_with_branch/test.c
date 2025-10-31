int f(int N, int a) {
  int s = 0;
  for (int i = 0; i < N; ++i) {
    if (i % 2 == 0) {
      int q = a * 3;
      s += i * q;
    } else {
      s += i;
    }
  }
  return s;
}