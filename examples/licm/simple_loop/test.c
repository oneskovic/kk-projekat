int sum(int *A, int N) {
  int s = 0;
  int x = 5;
  for (int i = 0; i < N; ++i) {
    int z = x * 2;
    s += A[i] + z;
  }
  return s;
}