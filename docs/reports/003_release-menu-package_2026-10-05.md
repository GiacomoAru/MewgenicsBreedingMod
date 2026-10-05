# Report 003: pulizia per la release, menù e pacchetto (S8-S9)

Data: 2026-10-05. Gioco 1.1.21239, slot di test 1.

## Summary

La mod è pronta come pacchetto: `outputs/CleanBreeding-0.1.0.zip` con sola DLL di release, `config.ini` (Vanilla) e `description.json`. Tolti il Cleanse e tutte le whitelist; il menù F8 ha i nomi e i testi nuovi, un font più grande e una finestra larga e centrata; gli strumenti di sviluppo non sono nella release. La suite sui preset dà 12 casi su 12 PASS a ogni livello. L'installazione pulita dallo zip e il menù sono stati confermati dall'utente.

## Technical details

- **Cleanse:** rimosso (`CleanseMode`, `[cleanse]`, radio e pulsante).
- **Whitelist:** rimossa per difetti e disordini. Ogni disordine e ogni difetto conta come negativo, Eternal Youth e Cyclops compresi.
- **Build:** opzione CMake `CB_DEV_TOOLS` (ON sviluppo, OFF release). Con OFF non si compilano simulatore, suite, `snapshot.cpp`, header "Developer tools", helper `[debug]`, `last_breed.txt`, `sim_reports.txt`. `scripts\build.ps1 [-Release]`, `scripts\install.ps1 [-Release]`, `scripts\package.ps1`; la configurazione di sviluppo sta in `configs/config.dev.ini`. Nella build di sviluppo l'header compare solo con `[debug] developer_tools=1`.
- **Menù:** assi "Inbreeding penalties" e "Inherited flaws", livelli Off / Reduced / Normal / Increased (valori nel `.ini` invariati: 2 / 1 / 0 / 3), preset Vanilla / Gentle / Carefree / Clean / Hardcore, righe di spiegazione e tooltip, testo ×1.6, tooltip a destra del cursore. Il menù compare anche con una versione del gioco non supportata, con la sola riga rossa "Inactive".
- **Pacchetto:** controllo che la cartella contenga esattamente tre file; la DLL di release non contiene le stringhe "Developer tools", "Simulate", "Run all tests", "Cleanse", "test_resources", "developer_tools", "whitelist".

## Data

Suite (genitori finti, N = 25000 per caso), dopo la rimozione della whitelist:

| Preset (Inbreeding penalties / Inherited flaws) | Esito | EternalYouth ereditato | Cyclops ereditato | Parti nuove, coi 0.25 |
|---|---|---|---|---|
| Gentle (Reduced / Reduced) | 12/12 PASS | 7.5% | 24.3% | 18.5% |
| Off / Normal | 12/12 PASS | 15.3% | 49.0% | 0.0% |
| Clean (Off / Off) | 12/12 PASS | 0.0% | 0.0% | 0.0% |
| Hardcore (Increased / Increased) | 12/12 PASS | 27.7% | 74.9% | 74.9% |

## Conclusions

- Tutte le richieste della decisione dell'utente sono implementate: nessun Cleanse, nessuna whitelist, strumenti di sviluppo fuori dalla release.
- La release non contiene codice di test; il `config.ini` iniziale è Vanilla.
- Limiti: la mod gira solo sulla versione 1.1.21239 (hash e signature); un aggiornamento del gioco richiede di ritrovare le signature. Gemelli (due chiamate per un parto) non osservati.

## Next steps

- Dopo un aggiornamento del gioco: trovare le signature con `misc/find_rvas.py` del template.
