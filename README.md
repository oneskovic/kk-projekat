# Implementacija nekih jednostavnih optimizacija
U okviru ovog projekta nekoliko jednostavnih optimizacija je implementirano pomoću LLVM prolaza koristeći novi LLVM pass manager.
Implementirane su sledeće optimizacije:

## Loop-invariant code motion
U ovoj optimizaciji se konzervativno "podižu" instrukcije iz petlje koje mogu bezbedno da se izvrše pre ulaska u petlju i nema potrebe da se ponavljaju u svakom prolasku kroz petlju. Primer:
<table>
<th>Pre (originalni kod)</th>
<th>Posle (nakon LICM)</th>
<tr>
<td>
<pre><code>int sum(int *A, int N) {
  int s = 0;
  int x = 5;
  for (int i = 0; i &lt; N; ++i) {
    int z = x * 2;
    s += A[i] + z;
  }
  return s;
}</code></pre>
</td>
<td>
<pre><code>int sum(int *A, int N) {
  int s = 0;
  int x = 5;
  int z = x * 2;
  for (int i = 0; i &lt; N; ++i) {
    s += A[i] + z;
  }
  return s;
}</code></pre>
</td>
</tr>
</table>