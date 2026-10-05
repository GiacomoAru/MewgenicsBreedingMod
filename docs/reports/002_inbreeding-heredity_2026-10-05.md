# Report 002: assi Inbreeding ed Eredità (S5-S7)

Data: 2026-10-05. Gioco 1.1.21239, slot di test 1.

## Summary

L'hook su `breed` ora applica i due assi della mod. Un simulatore con una suite automatica (genitori finti, N = 25000 per caso, 5 coppie di genitori per caso) ha confermato i tassi di tutti i livelli. Tutte le combinazioni provate danno **12 PASS su 12**, con attive, passive e `coi` del gattino invariati. Nel gioco vero, 58 nascite osservate: nessun crash, salvataggio e riavvio ok.

## Technical details

- Inbreeding: `breed` riceve `coi` scalato (Vanilla invariato, Mite ×0.5, Nessuno 0, Duro `min(1, 2×coi)`); `kitten->coi` torna al valore vero; a livello Nessuno si tolgono dal gattino i disordini nuovi non in whitelist (e si compattano gli slot).
- Eredità: Mite e Nessuno nascondono i disordini dei genitori (stringa `"None"`, livello 1) per slot con probabilità 0.5 o 1, e sostituiscono le parti difettose ereditate (altro genitore, altrimenti parte generata dal gioco); Duro dà una seconda possibilità (disordine 15%, difetto 50%).
- Parti per unità a coppie (gambe, braccia, occhi, sopracciglia, orecchie): i due lati portano sempre lo stesso id (re_notes S3).
- Nessuna whitelist per i difetti (decisione dell'utente); whitelist solo per i disordini.
- Strumenti di verifica: `simulator.cpp` (tendine con i gatti, "Run all tests"), `snapshot.cpp`. Solo nella build di sviluppo (`CB_DEV_TOOLS`).

## Data

Suite, genitori finti, N = 25000 per caso. Percentuale di gattini con almeno un tratto del tipo indicato.

Tassi "nuovi" (genitori puliti), coi della coppia = 0.25, per livello di Inbreeding (Eredità Vanilla):

| Livello | Disordini nuovi | Difetti nuovi |
|---|---|---|
| Vanilla | 4.1% (atteso 4) | 37.5% (37.5) |
| Mite | 2.0% (2) | 18.8% (18.8) |
| Nessuno | 0.1% (0, solo whitelist) | 0.0% (0) |
| Duro | 14.0% (14) | 74.8% (75) |

Tassi "ereditati" per livello di Eredità (Inbreeding Vanilla):

| Caso | Vanilla | Mite | Nessuno | Duro |
|---|---|---|---|---|
| un genitore con Pox | 14.7% | 7.7% | 0.0% | 27.6% |
| Pox su A, Flu su B | 27.6% | 14.5% | 0.0% | 48.3% |
| un genitore con Pox+Flu | 15.1% | 11.3% | 0.0% | 28.0% |
| EternalYouth (whitelist) | 15.0% | 14.9% | 15.1% | 14.9% |
| difetto agli occhi (1 unità) | 48.9% | 24.8% | 0.0% | 75.1% |
| difetto gambe+braccia (2 unità) | 74.0% | 43.1% | 0.0% | 93.1% |

Controlli invariati in tutti i giri: attive da un genitore 21.5-21.7% (media per giro), passive 0%, stat medie 4.8-5.2 su tutte e 7, `coi` del gattino sbagliato 0 volte.

Nascite vere (58 chiamate, snapshot): chiamate 1-11 e 53-58, compatibili con Perfect genetics, danno 17 gattini senza alcun tratto negativo, anche con genitori che avevano difetti (per esempio braccia 700 e gambe 703 con coi 0.277). Le chiamate 12-52, compatibili con Hard mode, danno spesso difetti nuovi (per esempio la chiamata 13: Insomnia ereditato più Anemia nuovo) e difetti ereditati (chiamate 35, 36). L'impostazione per nascita non era registrata: è un'inferenza dai dati.

## Conclusions

- I tassi di tutti i livelli sono quelli del DESIGN, con scarto sotto i 3 punti.
- La mod non altera le abilità né le stat; l'RNG del gioco non viene toccato (la suite salva e ripristina lo stato).
- Il gioco regge le scritture dell'hook su gattini veri, con salvataggio e riavvio.
- Limiti: il caso Cyclops (head 704) è stato rieseguito solo come caso generico di difetto; l'impostazione di ogni nascita vera è dedotta.

## Next steps

- S8: build di release senza strumenti di sviluppo (`CB_DEV_TOOLS=OFF`), Cleanse tolto.
- S9: README e pacchetto zip.
- S10: editor della whitelist dei disordini.
