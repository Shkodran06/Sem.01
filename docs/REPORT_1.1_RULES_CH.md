# Sem.01 Report 1.1 — Regole software e riferimenti svizzeri

> Verifica tecnica preliminare al 5 settembre 2026. Non è una certificazione né un’autorizzazione alla posa.

## Matrice

| ID | Regola IT / DE | Nel software | Base | Tipo e valutazione |
|---|---|---|---|---|
| R01 | Priorità del segnale luminoso / Vorrang des Lichtsignals | Il segnale luminoso domina la scelta del movimento. | SSV 68.1 | **LEGGE** — corrispondenza diretta |
| R02 | Rosso arresta / Rot bedeutet Halt | Con rosso nessun veicolo entra. | SSV 68.1bis | **LEGGE** — diretta |
| R03 | Verde autorizza / Grün gibt den Verkehr frei | Entra soltanto il gruppo autorizzato. | SSV 68.2 | **LEGGE** — restano gli obblighi di precedenza |
| R04 | Giallo dopo verde / Gelb nach Grün | Verde passa sempre da giallo prima del rosso. | SSV 68.4 lett. a | **LEGGE** — diretta |
| R05 | Rosso più giallo prima del verde / Rot und Gelb vor Grün | La fase prepara il verde con rosso e giallo. | SSV 68.4 lett. b | **LEGGE** — diretta |
| R06 | Ordine delle luci / Anordnung der Lichter | Rosso alto, giallo centro, verde basso. | SSV 70.5 | **LEGGE** — diretta |
| R07 | Frecce direzionali / Richtungspfeile | Segnale e autorizzazione valgono per la direzione indicata. | SSV 68.3, 68.5, 68.9 | **LEGGE** — diretta |
| R08 | Esclusione dei conflitti / Ausschluss von Konflikten | La matrice vieta verdi incompatibili. | SSV 71.3 | **LEGGE** — la geometria va validata sul sito |
| R09 | Frecce verdi protette / Geschützte Grünpfeile | Senza giallo lampeggiante escludono pedoni e traffico conflittuale. | SSV 71.3 | **LEGGE** — diretta |
| R10 | Posizione dei semafori / Standort der Lichtsignale | Ogni gruppo è associato chiaramente alla corsia. | SSV 71.1–1bis | **LEGGE** — quote reali da progettare |
| R11 | Corsie e preselezione / Fahrstreifen und Einspurpfeile | Corsia sinistra e corsia diritto/destra; deviazione registrata. | SSV 74.1–2 | **LEGGE** — la geometria è specifica del sito |
| R12 | Linea di arresto / Haltelinie | I veicoli attendono prima della linea bianca. | SSV 75.1 | **LEGGE** — diretta |
| R13 | Verde pedonale / Fussgängergrün | Il pedone entra solo con simbolo verde. | SSV 68.7; 70.7 | **LEGGE** — diretta |
| R14 | Sgombero pedonale / Fussgängerräumung | A fine verde nessun nuovo ingresso; chi attraversa esce subito. | SSV 68.7 | **LEGGE** — durata da calcolo tecnico |
| R15 | Chiamata pedonale memorizzata / Gespeicherte Anforderung | La richiesta resta fino a una fase sicura. | Norme VSS e progetto autorizzato | **TECNICA** — nessun singolo articolo prescrive l’algoritmo |
| R16 | Traffico compatibile col pedone / Verträglicher Verkehr | Avanzano soltanto movimenti che non incontrano le strisce attive. | SSV 71.3 | **LEGGE** — serve esclusione geometrica verificata |
| R17 | Biciclette / Fahrräder | Velocità propria; eventuali luci dedicate usano il simbolo bici. | SSV 68.8; 70.7 | **LEGGE** — il tracciamento lento è scelta tecnica |
| R18 | Zona protetta / Schutzzone | Nessuna fase conflittuale parte finché la zona è occupata. | SSV 71.3; norme VSS sugli intertempi | **TECNICA** — principio legale più realizzazione tecnica |
| R19 | Sgombero adattivo / Adaptive Räumzeit | Velocità, lunghezza e coda mantengono occupata la zona. | Norme VSS; ASTRA 11001 per impianti provvisori | **TECNICA** — valori e margini specifici del sito |
| R20 | Bus camion e articolati / Busse Lastwagen Sattelschlepper | L’uscita considera la parte posteriore e la lunghezza. | Analisi dei rischi e norme tecniche | **PROGETTO** — barriera prudenziale, non algoritmo imposto dalla SSV |
| R21 | Domanda e rotazione / Anforderung und Rotation | Scelta per domanda e compatibilità, con rotazione in parità. | Norme VSS e programma autorizzato | **TECNICA** — da dimensionare sui flussi reali |
| R22 | Guasto critico / Kritischer Fehler | Stato sicuro memorizzato fino a ripristino esplicito. | Analisi di sicurezza e norme applicabili | **PROGETTO** — fail-safe non certificato dal solo software |
| R23 | Perdita sensore / Sensorausfall | Mancata conferma non equivale a zona libera. | Analisi di sicurezza e norme tecniche | **PROGETTO** — regola prudenziale fail-safe |
| R24 | Direzione imprevista / Unerwartete Richtung | Anomalia registrata e protezione del movimento conflittuale. | SSV 74.2; SVG prudenza generale | **LEGGE** — il recupero software è barriera aggiuntiva |
| R25 | Cantiere autorizzato / Bewilligte Baustelle | Modalità cantiere separata dall’incrocio. | SSV 9, 80, 81, 107 | **LEGGE** — posa e ordinanze richiedono autorità/ASTRA |
| R26 | Cantiere madre figlio / Master Slave Baustelle | Un senso verde; inversione solo dopo corridoio libero. | SSV 71.3; ASTRA 11001; VSS | **TECNICA** — architettura di progetto |
| R27 | Perdita radio / Funkausfall | Blocca nuove autorizzazioni e genera guasto. | Analisi di sicurezza e norme tecniche | **PROGETTO** — da validare su hardware |
| R28 | Giallo lampeggiante / Gelbes Blinklicht | Usato solo nei casi ammessi, non come ciclo normale. | SSV 68.6; 70.1 | **LEGGE** — diretta |
| R29 | Segnalazione del cantiere / Baustellensignalisation | La versione reale richiede segnale 1.14 e dispositivi temporanei. | SSV 9, 80, 82 | **LEGGE** — non ancora completa nella simulazione |
| R30 | Camera e radar / Kamera und Radar | Stima presenza e velocità; non serve conservare immagini identificative. | LPD/DSG; indicazioni EDÖB | **LEGGE** — suolo pubblico: base legale e autorità; minimizzazione dati |
| R31 | Parametri del sito / Standortparameter | Tempi, distanze e timeout configurabili. | Norme VSS, progetto e autorizzazione | **TECNICA** — i valori demo non valgono automaticamente su strada |
| R32 | Registro e lingue / Protokoll und Sprachen | Interfaccia IT/DE e registro rendono verificabili le decisioni. | Documentazione e collaudo | **PROGETTO** — qualità e tracciabilità, non articolo stradale |

## Conclusione

Il simulatore implementa principi centrali della SSV ma non è un controllore certificato. Prima dell’hardware servono norme VSS vigenti, parametri del luogo, analisi dei rischi, prove indipendenti e autorizzazione.

## Fonti

- [SSV ufficiale italiano](https://www.fedlex.admin.ch/eli/cc/1979/1961_1961_1961/it)
- [SSV amtlich Deutsch](https://www.fedlex.admin.ch/eli/cc/1979/1961_1961_1961/de)
- [SVG RS 741.01](https://www.fedlex.admin.ch/eli/cc/1959/679_705_685/it)
- [VRV ONC RS 741.11](https://www.fedlex.admin.ch/eli/cc/1962/1364_1409_1420/it)
- [ASTRA segnali](https://www.astra.admin.ch/it/segnali)
- [EDÖB videosorveglianza spazio pubblico](https://www.edoeb.admin.ch/it/videosorveglianza-dello-spazio-pubblico-da-parte-di-privati)
