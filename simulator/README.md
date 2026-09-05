# Simulatore semafori da cantiere

Prima versione della simulazione grafica in C++20 e Qt 6.

## Avvio

Prerequisiti: compilatore C++20, CMake, Ninja e Qt 6 Widgets/Test. Dopo aver
impostato `CMAKE_PREFIX_PATH` sul proprio Qt 6:

```bash
cmake -S simulator -B simulator/build -G Ninja
cmake --build simulator/build
./simulator/build/semafori-simulator
```

Test:

```bash
ctest --test-dir simulator/build --output-on-failure
```

La dimostrazione LumaCross comprende quattro accessi con due corsie configurate
in modo uniforme (C1: solo sinistra; C2: diritto+destra),
piste ciclabili, quattro telecamere-radar, sensori d'uscita,
tracking simultaneo, velocità aggiornata durante il percorso, conteggio della
zona di conflitto e gestione informativa delle traiettorie impreviste. Il verde
successivo parte soltanto dopo lo sgombero completo e il margine finale. Il
controllore è attuato dalla domanda: a zona libera serve le richieste e combina
nello stesso verde soltanto gruppi di corsie compatibili. Le chiamate pedonali
sono memorizzate sui quattro lati; eventuali movimenti veicolari mantenuti
durante il verde pedonale vengono verificati contro l'attraversamento e seguono
la sequenza rosso+giallo, verde, giallo e rosso.
