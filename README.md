# Implementacija nekih jednostavnih optimizacija
U okviru ovog projekta nekoliko jednostavnih optimizacija je implementirano pomoću LLVM prolaza koristeći novi LLVM pass manager.
Implementirane su sledeće optimizacije:

## Loop-invariant code motion [(detalji implementacije)](/licm/README.md)
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

## Constant propagation and folding
U ovom prolazu se računaju i konstantni izrazi koje je moguće sračunati pre početka izvršavanja i menjaju se sa tom sračunatom vrednosti koja se propagira i na druge izraze u kojima se taj zamenjeni izraz koristio.
Primer:
<table>
<th>Pre (originalni kod)</th>
<th>Posle</th>
<tr>
<td>
<pre><code>
int f(void) {
    int t1 = 2 * 5;
    int t2 = 2 * t1;
    int t3 = 5 + t2;
    return t3;
}
</code></pre>
</td>
<td>
<pre><code>
int f(void) {
    return 25;
}
</code></pre>
</td>
</tr>
</table>