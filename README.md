# Sem.01 — LumaCross Control

Simulatore bilingue italiano/tedesco di una logica semaforica multi-sensore per un incrocio o una strettoia di cantiere.

## Funzioni

- domanda veicolare rilevata separatamente sui quattro accessi;
- sequenza `tutto rosso → rosso+giallo → verde → giallo → tutto rosso`;
- autorizzazione veicolare interbloccata e verifica dei movimenti compatibili con gli attraversamenti pedonali;
- interblocco della zona protetta prima di una nuova autorizzazione;
- chiamata pedonale memorizzata con sgombero controllato e mantenimento dei soli movimenti veicolari geometricamente non conflittuali, con sequenza rosso+giallo → verde → giallo → rosso anche per tali movimenti;
- biciclette con velocità di attraversamento differenziata;
- auto, biciclette, autobus, camion e autoarticolati con sagoma e lunghezza differenziate;
- conferma di uscita basata anche sulla parte posteriore del veicolo;
- perdita o bassa affidabilità del tracciamento trattata come zona non confermata libera;
- arresto d'emergenza in stato fail-safe;
- guasti critici memorizzati fino a conferma e ripristino espliciti;
- modalità cantiere madre–figlio con interblocco della strettoia e perdita radio fail-safe;
- interfaccia italiana e tedesca;
- test automatici della macchina a stati e delle invarianti di sicurezza.

## Avvio

```bash
pnpm install
pnpm dev
```

## Verifica

```bash
pnpm test
pnpm build
pnpm lint
```

## Riferimenti normativi

La logica dimostrativa è stata impostata facendo riferimento in particolare agli articoli 68, 70, 71, 74 e 75 della Signalisationsverordnung svizzera (SSV, RS 741.21): significato e sequenza delle luci, esclusione dei conflitti, dispositivi pedonali, frecce di corsia e linee di arresto.

Testo ufficiale: <https://www.fedlex.admin.ch/eli/cc/1979/1961_1961_1961/de>

## Limite d'impiego

Questo repository è un simulatore e prototipo software. Non è un controllore certificato e non deve comandare impianti stradali reali. La messa in servizio richiede almeno progettazione e validazione secondo le norme tecniche VSS applicabili, analisi di sicurezza indipendente, hardware fail-safe, prove sul campo e autorizzazione dell'autorità competente.

## Stato di completamento

Il nucleo TypeScript di sicurezza è coperto da test automatici ed è incorporato nella visualizzazione durante la build. La modalità Vita reale usa la stessa macchina a stati, gli stessi tempi e la matrice dei conflitti verificata; anche guasto critico e cantiere madre–figlio usano i rispettivi controllori testati. Il simulatore C++/Qt viene compilato e verificato separatamente e applica la stessa sequenza luminosa, lo stesso criterio di occupazione fino alla coda dei veicoli pesanti e la stessa esclusione dei conflitti pedonali.

Il progetto non dichiara conformità o prontezza per l'esercizio reale. Prima dell'hardware restano obbligatorie la revisione finale della parità C++/web, prove integrate estese e la validazione indipendente dei parametri specifici del luogo.
