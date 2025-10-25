# Loop-invariant code motion (LICM)
U ovoj optimizaciji cilj je da pronađemo instrukcije koje možemo bezbedno da izvršimo jednom pre ulaska u petlju i ne izvršavamo u svakoj iteraciji ponovo. Ovo je uprošćena verzija pravog LICM prolaza koji koristi LLVM kompajler i značajno je restriktivnija ali uspeva da optimizuje neke osnovne primere. U našoj implementaciji smatraćemo da instrukcija može da se podigne van petlje ukoliko je sledeće ispunjeno:
- **Svi operandi instrukcije su invarijantni** (ne menjaju svoju vrednost) u petlji
- **Možemo bezbedno da spekulativno izvršimo ovu instrukciju.** Kako bismo izbegli proveravanje da li se petlja izvrši barem jednom neke instrukcije možemo da izvršimo "spekulativno". Izvršiti instrukciju spekulativno znači izvršiti je iako se ona u početnom programu možda ne bi izvršila. Da bismo ovo smeli da uradimo moramo da osiguramo da nećemo promeniti ponašanje. Na primer instrukciju deljenja ne smemo da izvršimo spekulativno ako nismo sigurni da je delilac različit od nule.
- **Instrukcija nema bočnih efekata.** Slično kao i u prethodnoj proveri - ako podignemo instrukciju van petlje ona će se sigurno izvršiti barem jednom, a moguće je da se telo petlje ne bi izvršilo ni jednom. Zato ćemo izbeći instrukcije koje mogu da imaju bočne efekte.

Razmotrimo sledeći primer:
```c
int sum(int *A, int N) {
  int s = 0;
  int x = 5;
  for (int i = 0; i &lt; N; ++i) {
    int z = x * 2;
    s += A[i] + z;
  }
  return s;
}
```
Kada prevedemo kod u IR: 
```sh
clang -O0 -S -Xclang -disable-O0-optnone -emit-llvm test.c -o test.ll
```
Telo petlje počinje sa:
```llvm
...
%14 = load i32, i32* %6, align 4, !dbg !35
%15 = mul nsw i32 %14, 2, !dbg !36
store i32 %15, i32* %8, align 4, !dbg !34
%16 = load i32*, i32** %3, align 8, !dbg !37
%17 = load i32, i32* %7, align 4, !dbg !38
%18 = sext i32 %17 to i64, !dbg !37
...
```
Prve tri instrukcije odgovaraju c kodu z = x*2. Njih želimo da podignemo van petlje.Prva instrukcija je load instrukcija. Ona je bezbedna da se izvrši spekulativno, njen argument je %6 koji se ne menja unutar petlje (odgovara promenljivoj x u c kodu) i instrukcija nema bočnih efekata. Međutim ovo nije dovoljno proveriti za load instrukcije.
Posmatrajmo instrukciju:
```llvm
%17 = load i32, i32* %7, align 4
```
Njen argument %7 odgovara adresi na kojoj se nalazi vrednost brojača petlje (u c kodu promenljiva i). Naivnom proverom zaključili bismo da je ovu instrukciju takođe moguće podići van petlje. Ovo bismo zaključili jer je zaista tačno da %7 ne menja svoju vrednost tokom petlje. Međutim vrednost na toj adresi menja svoju vrednost u svakoj iteraciji. **Dakle, za load instrukcije potrebno je dodatno proveriti da li još neka instrukcija u petlji piše na tu adresu u tom slučaju ne smemo da podignemo load instrukciju van petlje.**
Sa ovom dodatnom proverom zaključujemo da je instrukciju
```llvm
%14 = load i32, i32* %6, align 4, !dbg !35
```
moguće podići van petlje, a narednu instrukciju nije:
```llvm
%17 = load i32, i32* %7, align 4
```
Kada podignemo prvu instrukciju van petlje onda će svi argumenti naredne instrukcije za množenje biti invarijantni pa ćemo smeti i nju da podignemo itd.
## Primer izvršavanja
Ako na prethodnom C kodu pokrenemo ovu optimizaciju:
```sh
opt -load-pass-plugin=./build/licm/LicmPass.so -passes="licm-pass" test.ll -S -o out.ll
```
Ukoliko takođe pokrenemo prolaz instnamer kako bismo dobili konzistentne nazive instrukcija i labeli dobićemo naredni kod:
```diff
  store i32 5, i32* %i4, align 4
  store i32 0, i32* %i5, align 4
+ %i9 = load i32, i32* %i2, align 4
+ %i12 = load i32, i32* %i4, align 4
+ %i13 = mul nsw i32 %i12, 2
+ %i14 = load i32*, i32** %i, align 8
  br label %bb7

bb7:                                              ; preds = %bb23, %bb
  %i8 = load i32, i32* %i5, align 4
- %i9 = load i32, i32* %i2, align 4
  %i10 = icmp slt i32 %i8, %i9
  br i1 %i10, label %bb11, label %bb26

bb11:                                             ; preds = %bb7
- %i12 = load i32, i32* %i4, align 4
- %i13 = mul nsw i32 %i12, 2
  store i32 %i13, i32* %i6, align 4
- %i14 = load i32*, i32** %i, align 8
  %i15 = load i32, i32* %i5, align 4
  %i16 = sext i32 %i15 to i64
  %i17 = getelementptr inbounds i32, i32* %i14, i64 %i16
  %i18 = load i32, i32* %i17, align 4
  %i19 = load i32, i32* %i6, align 4
  %i20 = add nsw i32 %i18, %i19
  %i21 = load i32, i32* %i3, align 4
  %i22 = add nsw i32 %i21, %i20
  store i32 %i22, i32* %i3, align 4
  br label %bb23

bb23:                                             ; preds = %bb11
  %i24 = load i32, i32* %i5, align 4
  %i25 = add nsw i32 %i24, 1
  store i32 %i25, i32* %i5, align 4
  br label %bb7, !llvm.loop !6

bb26:                                             ; preds = %bb7
  %i27 = load i32, i32* %i3, align 4
  ret i32 %i27
}
```