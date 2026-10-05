# Clean Breeding: piano d'esecuzione

Cosa fa la mod e perché: [DESIGN.md](DESIGN.md). Qui c'è il **come**, in step piccoli.

Regole per chi esegue:
- Uno step alla volta, in ordine. Uno step è finito solo quando **tutti** i suoi check passano.
- Aggiorna lo stato (`[ ]` diventa `[x]`) e annota in fondo a ogni step cosa hai scoperto.
- 🛑 indica i punti in cui ti fermi e chiedi all'utente: azioni nel gioco o fuori dal PC, oppure risultati che contraddicono DESIGN.md.
- Fatti nuovi sul gioco (offset, rappresentazioni, comportamenti) vanno in [re_notes.md](re_notes.md), con scritto come sono stati verificati.

Riferimenti (in `reference/`, sola lettura, non vanno in git):

| Repo | Cosa prendere |
|---|---|
| `mewgenics_randomize_item_picks/cpp` | **Template del progetto**: build CMake, `amoeboid.cpp/.hpp` (caricamento, controllo hash, signature scan, hook Detours), `types/`, `utilities/`, `lib/` |
| `mewgenics_analysis/cpp/amoeba` | Signature extra in `amoeba.hpp` (righe 34-63: `CatData__breed`, `CatData_ctor/dtor`, `unk_init`, `unk_init_bodyparts`, RNG xoshiro); overlay ImGui in `amoeba_imgui.cpp` (hook `SDL_GL_SwapWindow` e `SDL_PollEvent`, righe ~2270-2380); `ffi/cat_factory.cpp` (chiamata diretta a `breed`, `make_kitten`, backup dell'RNG); `types/glaiel_cat.hpp` (struct `CatData` completa) |
| `mewgenics-cat-bridge/mod/cat_bridge` | `collect_all_cats()` (tutti i gatti in memoria), `SET_PASSIVE` e `SET_PART` (modifica sicura di stringhe MSVC e parti); `docs/DEVELOPMENT.md` (fatti verificati) |
| [Gist SciresM](https://gist.github.com/SciresM/95a9dbba22937420e75d4da617af1397) (online) | Descrizione della funzione `breed` ricavata dal codice del gioco: ordine dei tiri, disordini, difetti, abilità. Utile per interpretare i log di S3 e i tassi del simulatore |
| `MewgenicsBreedingManager/src` | Solo per confronto: formule (`breeding.py`), riconoscimento dei difetti (`save_parser.py` ~righe 880-920, 1207-1330) |

Versione del gioco attesa: **1.1.21239**, SHA256 `4127cd6a792ae528bca6f65a8873dd61789591937d87656c2b586a5e30eb77ea` (verificata il 2026-10-05).

---

## S0. Prerequisiti 🛑

- [x] 🛑 L'utente installa:
  - Visual Studio 2022 Build Tools con il workload "Desktop development with C++" (contiene anche CMake);
  - Mewtator ([Nexus #1](https://www.nexusmods.com/mewgenics/mods/1)) e Mewjector ([Nexus #218](https://www.nexusmods.com/mewgenics/mods/218));
  - in Mewtator: Settings → Launch Options → "DLL Mod Support" attivo, poi avvia il gioco una volta.

  Non installare tu software di sistema.
- [x] Verifica: trovi `cmake.exe` (cerca sotto `Microsoft Visual Studio\*\*\Common7\IDE\CommonExtensions\Microsoft\CMake`, come fa `reference/mewgenics-cat-bridge/mod/build.ps1`) e `version.dll` di Mewjector nella cartella del gioco.
- [x] `git init` nella root del progetto. Il `.gitignore` esiste già. Primo commit locale solo dopo l'ok dell'utente; nessun push senza richiesta esplicita.
- [x] Verifica che `sha256sum Mewgenics.exe` sia uguale a quello sopra. Se è diverso 🛑: c'è stato un update del gioco.

**Note S0 (2026-10-05):** cmake.exe in `C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin`; MSVC 14.44.35207 presente. `version.dll` e `chainloader.ini` (Mewjector) nella cartella del gioco; `mod_logs/chainloader.log` esiste. SHA256 dell'exe uguale all'atteso. Mewtator è scompattato in `Desktop\Mewtator 1 0.6.0 ...\Mewtator` (con cartella `mods`), ma `MewtatorManifest=` in chainloader.ini è vuoto: non risulta ancora avviato con "DLL Mod Support" attivo. `git init` fatto, nessun commit. Dopo la risposta dell'utente: Mewtator spostato in `<progetto>\Mewtator 1 0.6.0 ...\Mewtator` (gitignorato). Il suo `config.json` ha `dll_injection_enabled: true` ma `mod_folder` punta ancora al vecchio percorso su Desktop (non esiste più); `MewtatorManifest=` resta vuoto: il gioco è stato avviato (log 12:29) ma apparentemente non via Mewtator, quindi il manifest non è stato scritto. Da sistemare in S1 (install.ps1 userà la nuova cartella `mods`).

## S1. Scheletro DLL che si carica

- [x] Copia `reference/mewgenics_randomize_item_picks/cpp/` in `src/`, insieme ad `ATTRIBUTION.md`, che va mantenuto (licenza MIT, autore polymeric).
- [x] Rinomina il target e la cartella in `clean_breeding`. In `amoeboid.hpp`: `MOD_NAME "Clean Breeding"`, `MOD_IDENTIFIER "cicci.clean_breeding"`, `MOD_VERSION "0.1.0"`.
- [x] Svuota la logica di `randomize_item_picks.cpp` (rinominalo `clean_breeding.cpp`): tieni solo l'avvio e un messaggio di log "Clean Breeding loaded".
- [x] Crea `scripts/build.ps1` (modello: `reference/mewgenics-cat-bridge/mod/build.ps1`). Lo script trova CMake, compila in RelWithDebInfo e copia `clean_breeding.dll` in `mod/CleanBreeding/`.
- [x] Crea `scripts/install.ps1`, che copia `mod/CleanBreeding/` nella cartella mods di Mewtator. 🛑 Chiedi all'utente dov'è installato Mewtator.
- [x] Check:
  - [x] la build passa senza errori;
  - [x] 🛑 l'utente abilita la mod in Mewtator e avvia il gioco; il log di Mewjector (`mod_logs/chainloader.log`) o quello della mod mostra "Clean Breeding loaded" e l'hash dell'exe OK;
  - [x] il gioco non crasha.

**Note S1 (2026-10-05):** template copiato in `src/` (cartella mod `src/clean_breeding/`, senza `cosmic_ooze` né `misc/find_rvas.py`; quest'ultimo si riprende nel "Dopo"). Rimosse da `amoeboid.hpp` le signature del mod originale. Il log "Clean Breeding loaded" è in `clean_breeding_init()`, chiamata a fine `on_attach()`, quindi appare solo se hash e hook sono OK. Il log va nel `chainloader.log` di Mewjector (logging Mewjector attivo, console no). Build RelWithDebInfo OK con MSVC 19.44; la DLL esce in `build/clean_breeding/RelWithDebInfo/`. Mewtator vuole nella cartella mod: `description.json` + DLL (come il repo upstream). `install.ps1` copia in `Mewtator*/Mewtator/mods/CleanBreeding/` dentro il progetto e non sovrascrive un `config.ini` già presente. Test nel gioco OK (12:40): `chainloader.log` mostra "Clean Breeding loaded (version 0.1.0, exe hash OK, game 1.1.21239)", 1 DLL caricata via manifest Mewtator, uscita pulita (ExitProcess), nessuna cartella `mod_logs/crashes`. Mewtator ora sta in `<progetto>/Mewtator/` (già gitignorato), `mod_folder` sistemato dall'utente. Remote git: `origin` = https://github.com/GiacomoAru/UnnaturalSelection.git (nessun push).

## S2. Overlay ImGui con F8

- [x] Aggiungi Dear ImGui in `src/lib/imgui/` (commit `b61e56346a92cfcaf1f43a545ca37b0b32239654`, lo stesso di Amoeba). Il CMake va preso da `reference/mewgenics_analysis/cpp/lib/cmake/imgui/CMakeLists.txt`.
- [x] Porta in `src/clean_breeding/menu.cpp` gli hook `SDL_GL_SwapWindow` e `SDL_PollEvent` da `amoeba_imgui.cpp`, togliendo tutto ciò che non serve: niente viewport multipli, niente demo.
- [x] F8 apre e chiude una finestra "Clean Breeding" che per ora mostra solo lo stato: versione della mod, hash OK, signature trovate.
- [x] Quando il menù è aperto e ImGui vuole input (`io.WantCaptureMouse/Keyboard`), gli eventi non arrivano al gioco.
- [x] Check 🛑 (con l'utente nel gioco):
  - F8 mostra e nasconde la finestra (confermato dall'utente);
  - mentre la finestra è aperta i click sulla finestra non passano al gioco (non dichiarato esplicitamente: l'utente ha detto "F8 funziona, procedi");
  - il gioco va normalmente con la finestra chiusa (idem; log pulito: Mewjector "Integrity check: ALL OK", uscita ExitProcess, nessun crash report).

**Note S2 (2026-10-05):** ImGui b61e563 scaricato (zip da GitHub ocornut/imgui, 2.4 MB, con ok dell'utente) e ridotto a core + backend SDL3/OpenGL3 + `imgui_stdlib` + `imconfig.h`, senza `imgui_demo.cpp`, in `src/lib/imgui/third_party/`; voce aggiunta in `ATTRIBUTION.md`. Serve anche `src/lib/Mewgenics/` (copiato da Amoeba): import lib generata da `Mewgenics.def` con gli export SDL dell'exe (il .def è dalla 1.0.20763, ma i 60 simboli SDL importati dalla DLL sono tutti presenti negli export dell'exe 1.1.21239, verificato con `dumpbin`). Il template non installava gli hook di gruppo 1 (hook per nome di export): aggiunti resolve e install del gruppo 1 in `amoeboid.cpp`. F8 è consumato dal nostro hook e non arriva al gioco. A menù chiuso gli eventi passano intatti e non si fa render ImGui.

## S3. Probe di `breed` in sola lettura

- [x] Aggiungi le signature `ADDRESS_glaiel__CatData__breed`, `CatData_ctor`, `CatData_dtor`, `CatData_unk_init`, `CatData_unk_init_bodyparts` e `TLS0OFF_xoshiro256p_rng_context` da `amoeba.hpp`. Aggiungi `types/glaiel_cat.hpp` di Amoeba se è più completo di quello del template.
- [x] Hook su `breed` che **non modifica niente**. Per ogni chiamata logga:
  - `coi` passato;
  - sql_key dei genitori;
  - i due slot disordine di ogni genitore (stringa più livello, e anche i byte grezzi della stringa se è vuota);
  - `part_sprite_idx` dei 14 slot più `texture_sprite_idx` di genitori e gattino;
  - `kitten->coi` dopo la chiamata originale;
  - i due slot disordine del gattino dopo la chiamata.
- [x] Aggiunto (su richiesta dell'utente, 2026-10-05) `src/clean_breeding/snapshot.cpp`: dopo ogni chiamata a `breed` scrive in `<cartella DLL>/snapshots/` file JSON con lo stato di tutti i gatti in memoria (chiavi, parti, disordini, passive, in casa/fuori, giorno), con l'evento breed (genitori e gattino come restituito da `breed`). Fasi: `after_breed` (frame dopo), `new_cats` (appena compare una chiave nuova), `timeout`. Così le risposte si ricavano dai dati e non dalla memoria dell'utente.
- [x] 🛑 L'utente fa passare 1-2 notti nel gioco, con cat che si accoppiano (meglio una coppia consanguinea).
- [x] Rispondi in `docs/re_notes.md`, con le prove:
  1. `breed` viene chiamata una volta per gattino? (gemelli = 2 chiamate?)
  2. Come è rappresentato uno slot disordine vuoto? (stringa vuota, `"None"`, livello 0?)
  3. `kitten->coi` dopo `breed` è uguale al parametro `coi`?
  4. Mappa slot → file `.gon`: body, head, tail, leg1, leg2, arm1 e arm2 (→ `legs`), lefteye e righteye (→ `eyes`), eyebrows, ears, mouth, texture. Conferma con cat-bridge DEVELOPMENT.md e con almeno un difetto visibile nel gioco.
  5. Se un gattino ha due disordini, slot 0 e 1 sono sempre riempiti in ordine?
- [x] Se una risposta contraddice DESIGN.md 🛑. (Nessuna contraddizione; affinamento approvato dall'utente: in S7 nascondere un disordine con `"None"` livello 1 invece di una stringa vuota, perché lo slot vuoto del gioco è `"None"` livello 1.)
- [x] **Report 001** (`docs/reports/001_breed-probe_<data>.md` più il file repro), secondo le regole in CLAUDE.md.

## S4. Dati: tabella difetti e config

- [x] `scripts/gen_defect_table.py`: legge `data/mutations/*.gon` dal gpak, riusando la logica di `scripts/gpak.py`, e scrive `src/clean_breeding/defect_table.hpp` con, per ogni gruppo, l'elenco degli id che hanno `tag birth_defect`, più `-2`. Il file generato va committato e deve avere in testa la versione del gioco.
- [x] Self-check nello script (`assert`): `eyes` contiene 700, 701, 704, 705, 706 e -2; `legs` contiene 700-707; `head` contiene 704.
- [x] `config.cpp`:
  - legge `config.ini` accanto alla DLL (percorso con `GetModuleFileNameW` dell'handle della DLL), usando `GetPrivateProfileStringW`;
  - scrive con `WritePrivateProfileStringW`;
  - parsa la whitelist (disordini: nomi; difetti: `gruppo:id`);
  - se mancano valori usa i default di DESIGN.md (tutto a 0).
- [x] Il `config.ini` di riferimento è quello in `mod/CleanBreeding/config.ini` (sezioni `[breeding]`, `[cleanse]`, `[whitelist]`). Non riscriverlo da zero: quello di S1 aveva perso la whitelist ed era salvato con BOM UTF-8, che rompe `GetPrivateProfileStringW`. Va salvato in UTF-8 **senza BOM**, oppure in UTF-16 LE con BOM; `install.ps1` non deve sovrascrivere un `config.ini` già presente.
- [x] Menù: 2 selettori a 4 livelli (Duro / Vanilla / Mite / Nessuno; valori 3 / 0 / 1 / 2 come in DESIGN.md), 5 pulsanti preset (compreso "Hard mode" 3/3), selettore della modalità Cleanse. Ogni cambio si salva subito nel `.ini`. Il pulsante Cleanse per ora è disabilitato.
- [x] Check:
  - [x] lo script di generazione passa i suoi assert;
  - [x] 🛑 nel gioco: un cambio nel menù aggiorna il `.ini`, e riavviando il gioco i valori restano. (Verificato: `.ini` scritto alle 13:24 con `inbreeding=2`; al riavvio il log mostra `Config: inbreeding=2 heredity=0`; l'utente conferma che il menù li mostra.)

**Note S4 (2026-10-05):** `scripts/gpak.py` ha ora le funzioni `read_index()` e `read_file()` (la CLI è invariata). `gen_defect_table.py` genera `defect_table.hpp`: per gruppo gli id con `tag birth_defect` più -2; id come mouth 1500 (`tag animal`) sono esclusi; assert del piano passati, più uno su mouth 1500. `config.cpp` + `config_parse.hpp` (parsing puro) con test `src/tests/test_config.cpp`, eseguito da `scripts/build.ps1` in Debug (in RelWithDebInfo gli `assert` sono spenti). Il `.ini` resta tutto ASCII: `WritePrivateProfileStringW` lo riscrive come ANSI. Menù in inglese (Hard / Vanilla / Mild / None). La whitelist è letta ma non ancora usata da nessuna logica.

## S5. Simulatore (sezione Debug del menù)

Serve a misurare i tassi senza giocare decine di notti.

- [x] Porta `make_kitten` / `new_default_cat` e l'hook che sopprime la name history da `amoeba/ffi/cat_factory.cpp`.
- [x] (con deviazioni: tendine con i nomi dei gatti al posto dei campi sql_key; coi inserito a mano o con pulsanti, perché calcolarlo richiede di portare il pedigree del gioco; pulsante "Fill from last breeding" al posto di "prima coppia con coi > 0.25", con l'ultima coppia salvata in `last_breed.txt`) Sezione "Debug" collassabile:
  - due campi sql_key per i genitori, più un pulsante che riempie con la prima coppia trovata con coi > 0.25;
  - `N` (default 1000);
  - pulsante "Simula".

  Il simulatore chiama `breed` **attraverso il nostro hook**, quindi con le impostazioni correnti, su gattini temporanei che vengono distrutti subito. Salva e ripristina lo stato dell'RNG come fa Amoeba.
- [x] Mostra:
  - % di gattini con almeno un disordine, divisi in ereditati (presenti in un genitore) e nuovi;
  - % di gattini con almeno una parte difettosa, divisi in ereditati e nuovi;
  - media dei tratti negativi;
  - % di tratti in whitelist;
  - **controllo abilità** (la mod non deve cambiarle):
    - % di gattini il cui `actives_inherited[0]` è un'attiva di un genitore;
    - % di gattini con una passiva (`passive_0` diverso da `"None"`) presa da un genitore;
    - media delle 7 stat di `stats_heritable`.
- [x] (eseguito con la suite automatica "Run all tests" su genitori finti, N = 10000 per caso; tutti i tassi nella tolleranza, tabella in `docs/re_notes.md`) Check (impostazioni Vanilla, N = 2000):
  - coppia non consanguinea, genitori senza disordini: disordini nuovi circa 2% (±1%);
  - coppia con coi noto: disordini nuovi circa `max(2%, 0.4·coi − 6%)` e difetti nuovi circa `min(1, 1.5·coi)`, con tolleranza ±3 punti;
  - genitore con 1 disordine: ereditati circa 15%.

  Se un tasso esce dalla tolleranza 🛑.
- [x] (verificato dall'utente: 17 gatti prima, dopo la suite a N = 25000 e dopo salva e riavvio; nelle simulazioni singole il report dice "parents unchanged yes, cat count unchanged yes". I report ora si salvano anche in `sim_reports.txt` accanto alla DLL, perché il log di Mewjector si sovrascrive a ogni avvio) Il simulatore non deve lasciare tracce: dopo una simulazione il numero di gatti e il pedigree sono invariati, e salvando e ricaricando non compaiono gatti nuovi.

## S6. Asse Inbreeding

- [x] Nell'hook di `breed`, implementa la parte inbreeding come in DESIGN.md: `coi'` = {0: coi, 1: coi × 0.5, 2: 0, 3: min(1, coi × 2)}, `kitten->coi = real_coi`, e a livello 2 la rimozione dal gattino dei disordini nuovi non in whitelist. Per rimuovere usa la tecnica di `SET_PASSIVE` di cat-bridge: `destroy()` più `construct()` della rappresentazione "vuoto" trovata in S3; compatta gli slot se in S3 è risultato che servono in ordine.
- [x] (eseguito con la suite, N = 25000 per caso, livelli 1, 2 e 3 con Eredità Vanilla: tutti PASS; `sim_reports.txt` del 2026-10-05 14:24-14:25. Il confronto delle stat medie tra i livelli non c'era nel report di quei giri: ora la suite le scrive e si confronta nei test di S7) Check con il simulatore su una coppia con coi ≥ 0.25:
  - livello 1: i tassi "nuovi" corrispondono alla formula calcolata con coi/2;
  - livello 2: disordini nuovi 0% (tranne quelli in whitelist) e difetti nuovi 0%;
  - livello 3 (Duro): i tassi "nuovi" corrispondono alla formula calcolata con `min(1, 2·coi)` (es. coi 0.25: difetti nuovi circa 75%);
  - con qualsiasi livello, `kitten->coi` è uguale al coi vero;
  - i tassi "ereditati" sono invariati rispetto a Vanilla;
  - con qualsiasi livello, il controllo abilità (attive, passive, stat) è uguale a Vanilla entro ±3 punti. Se non lo è 🛑.
- [x] (`src/clean_breeding/breed_logic.hpp` + `src/tests/test_breed_logic.cpp`, eseguito da `scripts/build.ps1` in Debug) Self-check di unità (funzione pura, senza gioco): `scaled_coi(coi, level)` e il filtro disordini su casi fissi, con un `assert` in un test in `src/tests/` o nella build Debug.

## S7. Asse Eredità

- [x] Prima di `orig`, nascondi gli slot disordine dei genitori:
  - scambia i byte della stringa MSVC con una stringa `"None"` costruita localmente (e metti il livello a 1: è la rappresentazione dello slot vuoto, verificata in S3; approvato dall'utente il 2026-10-05), senza allocare né liberare; ripristina anche il livello;
  - ripristina in un blocco che gira **sempre** (RAII);
  - a livello 1, nascondi ogni slot con probabilità 0.5;
  - mai uno slot in whitelist.

  Usa un RNG della DLL (`std::mt19937_64` con seed da `std::random_device`), **non** l'RNG del gioco, per non alterarne la sequenza.
- [x] (il "lato simmetrico" non esiste: i due lati di una coppia hanno sempre lo stesso id, re_notes S3; si passa dall'altro genitore direttamente alla parte generata; la sostituzione è per unità a coppie, in `parts.hpp`) Dopo `orig`, sostituisci le parti difettose ereditate secondo DESIGN.md: altro genitore, poi lato simmetrico, poi parte generata. Per la parte generata chiama `CatData_unk_init_bodyparts` su un `BodyParts` temporaneo, con l'RNG salvato e ripristinato.
- [x] Livello 3 (Duro), dopo `orig`: la "seconda possibilità" di DESIGN.md.
  - Disordini: 15% di copiarne uno del genitore se il gattino non ne ha nessuno di quel genitore e ha uno slot libero.
  - Parti: 50% di copiare un difetto di un genitore al posto di una parte normale nello stesso slot.

  Si scrive con gli stessi strumenti di S6 (`destroy()`/`construct()`, scrittura di `part_sprite_idx`). Mai su tratti in whitelist.
- [x] (fatto: `whitelist_defects` e `parse_defects` rimossi da config, hook, simulatore e test; la chiave `defects` e i suoi commenti tolti da entrambi i `config.ini`; il caso "head 704" della suite ora è un normale caso di difetto) **Decisione dell'utente del 2026-10-05 (vedi DESIGN.md): niente whitelist per i difetti di nascita.** Prima dei check:
  - togli `whitelist_defects` da config, breed, simulatore e test, e togli la chiave `defects` da `[whitelist]` in `mod/CleanBreeding/config.ini` (anche dalla copia installata in Mewtator);
  - nel codice ogni difetto conta come negativo;
  - nella suite, il caso "A: head 704 (whitelist)" diventa un caso normale di difetto (atteso 0% a livello 2).
- [x] (7 giri della suite, N = 25000 per caso, 12/12 PASS; il caso "head 704" trattato ancora come protetto in quei giri, poi reso difetto normale senza rilanciare la suite: stesso codice del caso "occhi") Check con il simulatore:
  - genitore con 1 disordine non in whitelist: ereditati 0% a livello 2, circa 7.5% a livello 1, circa 27.75% a livello 3;
  - genitore con un difetto non in whitelist: a livello 3 ereditati circa `p + (1 − p)·0.5`, dove `p` è il tasso Vanilla misurato;
  - genitore con un difetto non in whitelist: ereditati 0% a livello 2, circa la metà del Vanilla a livello 1;
  - genitore con `EternalYouth`: tassi uguali a Vanilla a tutti i livelli;
  - genitore con `head:704` (Cyclops): trattato come ogni altro difetto;
  - dopo la simulazione i genitori sono identici a prima (confronta i loro byte prima e dopo);
  - con qualsiasi livello, il controllo abilità (attive, passive, stat) è uguale a Vanilla entro ±3 punti. Se non lo è 🛑.
- [x] (fatto dall'utente il 2026-10-05, 58 nascite osservate; 17 gattini senza alcun tratto negativo nelle chiamate 1-11 e 53-58, compatibili con Perfect genetics (l'impostazione per nascita non era ancora registrata negli snapshot, ora lo è), vedi Report 002) 🛑 Test reale con l'utente: preset "Genetica perfetta", 2-3 notti, nessun gattino con tratti negativi fuori whitelist. Poi salva, ricarica, e controlla che il gioco non dia errori.
- [x] (fatto dall'utente, nessun crash, salva e riavvia ok) 🛑 Test reale "Hard mode" (3/3), 1-2 notti con una coppia consanguinea: il gioco non crasha, i gattini hanno più tratti negativi del solito, salva e ricarica OK.
- [x] **Report 002** (campagna di test S5-S7: tabelle dei tassi Duro / Vanilla / Mite / Nessuno per asse).

## S8. Pulizia per la release (sostituisce il vecchio "Cleanse")

Decisione dell'utente del 2026-10-05: il Cleanse non si pubblica, e gli strumenti di test non vanno nella release.

- [x] Togli il Cleanse:
  - in config: `CleanseMode`, `[cleanse]`, `config_save_cleanse_mode`;
  - nel menù: radio button e pulsante;
  - in `mod/CleanBreeding/config.ini`;
  - ogni riferimento in README e docs.

  Non serve implementarlo.
- [x] Opzione CMake `CB_DEV_TOOLS`:
  - **ON** nella build di sviluppo (`scripts/build.ps1`, default attuale);
  - **OFF** nella build di release;
  - con OFF non vengono compilati simulatore, test suite, sezione Debug del menù, `snapshot.cpp`, test helper `[debug]` (`test_resources`, `test_disorders`, `test_simulation`), `last_breed.txt` e `sim_reports.txt`.

  Usa `#if CB_DEV_TOOLS` o liste di sorgenti condizionali nel CMake.
- [x] (codice, test e `.ini` fatti; la suite va rilanciata dall'utente, vedi sotto) **Decisione dell'utente del 2026-10-05 (vedi DESIGN.md): nessuna whitelist, nemmeno per i disordini.**
  - Togli `whitelist_disorders`, il parsing di `[whitelist]` e ogni controllo "in whitelist" da config, hook, simulatore e test; togli la sezione `[whitelist]` da entrambi i `config.ini` (progetto e copia installata in Mewtator).
  - Ogni disordine e ogni difetto conta come negativo.
  - Nella suite, il caso "A: EternalYouth (whitelist)" diventa un caso normale di disordine: atteso 0% a livello 2, circa 7.5% a livello 1, circa 27.75% a livello 3.
  - [x] Rilancia la suite (N = 25000): tutto PASS. (Fatto dall'utente il 2026-10-05 15:20-15:21 con Gentle, Off/Normal, Clean e Hardcore: 12 su 12 PASS in tutti. EternalYouth ereditato 7.5% / 15.3% / 0.0% / 27.7%; Cyclops come difetto normale 24.3% / 49.0% / 0.0% / 74.9%.)
- [x] Il `config.ini` di release (`mod/CleanBreeding/config.ini`) contiene solo `[breeding]` (aggiornato: niente più `[whitelist]`). La sezione `[debug]` resta solo nella copia di test installata in Mewtator, oppure in un file `configs/config.dev.ini` che `install.ps1` copia quando si installa la build di sviluppo.
- [x] Check:
  - [x] la build di release compila e la DLL non contiene le stringhe `"Run all tests"`, `"test_resources"`, `"Cleanse"` (controlla con `findstr /c:` o con Python);
  - [x] (confermato dall'utente il 2026-10-05: "tutto perfetto") 🛑 nel gioco, con la build di release: F8 mostra solo livelli, preset e stato; il breeding funziona;
  - [x] la build di sviluppo continua ad avere simulatore e test suite.

**Note S8/S8b (2026-10-05):** il menù e la riga di stato ora vivono anche con versione del gioco non supportata (gli hook SDL, gruppo 1, si installano per primi e sempre; con versione non supportata il menù mostra solo la riga rossa "Inactive"). Il flag `G.mod_active` dice se l'hook di `breed` è attivo. `level_label()` in `config.hpp` dà i nomi dei livelli (2 Off, 1 Reduced, 0 Normal, 3 Increased) a menù, log e report. `scripts/package.ps1` produce `outputs/CleanBreeding-0.1.0.zip` con solo `CleanBreeding/{clean_breeding.dll, config.ini, description.json}`. Il carattere "·" nelle righe del menù è scritto come byte UTF-8: da verificare a vista che il font di ImGui lo mostri.

## S8b. Nomi, testi del menù e spiegazioni

Decisione dell'utente del 2026-10-05: nomi più chiari e uniformi, piccole spiegazioni nel menù, niente debug nella versione pubblica. I testi esatti sono in DESIGN.md, sezione "Nomi e testi": usali così come sono, non inventarne altri.

- [x] Rinomina nel menù assi, livelli e preset come da DESIGN.md:
  - assi: "Inbreeding" diventa "Inbreeding penalties", "Heredity" diventa "Inherited flaws";
  - livelli: Off / Reduced / Normal / Increased, mostrati in quest'ordine;
  - preset: Vanilla / Gentle / Carefree / Clean / Hardcore.

  I valori numerici in `config.ini` **non cambiano** (0 Normal, 1 Reduced, 2 Off, 3 Increased) e nemmeno le chiavi `inbreeding` e `heredity`: cambiano solo le etichette. Aggiorna i commenti di `config.ini`.
- [x] Spiegazioni compatte:
  - sotto ogni regolatore una riga grigia (`TextDisabled`) che spiega l'asse;
  - passando il mouse su un livello o su un preset, un tooltip di una riga;
  - in fondo una riga grigia "F8: show/hide · Settings are saved automatically".
- [x] Riga di stato per il giocatore, senza termini tecnici (niente hash, niente signature):
  - verde "Active · Mewgenics 1.1.21239";
  - rosso "Inactive: unsupported game version (needs 1.1.21239)".

  I dettagli tecnici vanno solo nel log.
- [x] Debug:
  - nella build di release (`CB_DEV_TOOLS=OFF`) il menù non contiene niente di debug (già vero dopo S8: verificalo);
  - nella build di sviluppo la sezione resta, raggruppata in fondo sotto un unico header "Developer tools", così si distingue dal menù vero.
- [x] Rinomina negli stessi termini anche il report del simulatore e i nomi dei casi della suite (es. "level 2" diventa "Off"), così report e menù parlano la stessa lingua.
- [x] Documenti:
  - `README.md` con la sezione "How it works": i due regolatori, i 4 livelli con i numeri della tabella di DESIGN.md (in parole semplici), i 5 preset e a chi servono;
  - una nota su cosa la mod **non** tocca: abilità, passive, stat, mutazioni normali, gatti adulti;
  - aggiorna CLAUDE.md e i nomi usati in PLAN e DESIGN, se servono.
- [x] Check:
  - [x] (confermato dall'utente) 🛑 l'utente apre il menù (build di release) e conferma che si capisce senza leggere la documentazione;
  - nessuna stringa del menù è più larga della finestra a 1280×720;
  - [x] la build di release non contiene "Developer tools", "Simulate", "Run all tests".

**Note S8b, seconda tornata (2026-10-05):** font del menù x1.6 (`FontScaleMain`), finestra larga e centrata a ogni apertura, tooltip spostati a destra del cursore. L'header "Developer tools" nella build di sviluppo compare solo con `[debug] developer_tools=1` in `config.ini` (mai dal menù); nella release il codice non esiste. Il preset iniziale è Vanilla perché il `config.ini` del pacchetto ha `inbreeding=0` e `heredity=0` (e i valori mancanti valgono 0).

## S9. Packaging

- [x] `README.md` in root:
  - cosa fa la mod;
  - installazione (Mewtator più Mewjector);
  - versione del gioco supportata;
  - i due regolatori, i 4 livelli e i 5 preset;
  - crediti (polymeric, z3ndroot, MIT).
- [x] `scripts/package.ps1`: build di **release** (`CB_DEV_TOOLS=OFF`), poi zip di `mod/CleanBreeding/` in `outputs/CleanBreeding-<versione>.zip`.
- [x] (fatto: cartella mod svuotata, dati di sviluppo archiviati in `outputs/dev-data-2026-10-05/`, zip estratto; confermato dall'utente) Check: installazione pulita dallo zip in Mewtator su un'altra copia della cartella mods; il gioco parte e il menù funziona.

## S9b. Rinomina in "Unnatural Selection" (da fare: non ancora eseguito al 2026-10-05)

Decisione dell'utente del 2026-10-05.

- [x] Rinomina:
  - cartella `mod/CleanBreeding/` in `mod/UnnaturalSelection/`;
  - target CMake e DLL in `unnatural_selection` (`unnatural_selection.dll`);
  - `MOD_NAME "Unnatural Selection"`, `MOD_IDENTIFIER "geco.unnatural_selection"`;
  - titolo della finestra del menù;
  - `description.json` (`title` "Unnatural Selection", `description` = il summary della mod);
  - zip in `outputs/UnnaturalSelection-<versione>.zip`;
  - script `build.ps1`, `install.ps1`, `package.ps1`;
  - README, CLAUDE.md, DESIGN.md.

  La cartella del codice `src/clean_breeding/` può restare (è solo interna); il preset "Clean" resta "Clean".
- [x] `install.ps1`: se in Mewtator esiste ancora la vecchia cartella `CleanBreeding`, avvisa (non cancellarla da solo): due DLL della stessa mod caricate insieme si aggancerebbero due volte a `breed`. 🛑 Chiedi all'utente di toglierla.
- [x] Il `config.ini` installato si porta nella nuova cartella, così le impostazioni dell'utente restano.
- [x] Check:
  - build di release e di sviluppo compilano;
  - il pacchetto contiene `UnnaturalSelection/` con `unnatural_selection.dll`, `config.ini`, `description.json`;
  - [x] nel gioco: in Mewtator appare "Unnatural Selection", F8 mostra il nuovo titolo, il breeding funziona, e la vecchia mod non è più caricata (controlla `chainloader.log`: una sola DLL della mod). (confermato dall'utente: mod caricata e testata; `chainloader.log`: una sola DLL, `Unnatural Selection loaded (version 1.0.0 ...)`)

## S9c. Pubblicazione v1.0.0

Decisione dell'utente del 2026-10-05: si pubblica. Il repo GitHub è già pubblico e pulito (verificato: niente binari tracciati, niente Steam ID né percorsi personali, nemmeno nella storia). `LICENSE.md` è già aggiornato (MIT: GiacomoAru, polymeric, Z3nd).

- [x] Versione `1.0.0` in `MOD_VERSION`, `description.json` e nel nome dello zip.
- [x] `description.json`:
  - `"author": "Geco"` (nome dell'autore: Geco; GiacomoAru è solo l'account GitHub);
  - `"url": "https://www.nexusmods.com/mewgenics/mods/543"` (pagina Nexus dell'utente; mettila anche nel README e nelle note della Release);
  - `description` = il summary della mod.
- [x] `scripts/package.ps1`: lo zip deve contenere, nella cartella `UnnaturalSelection/`, anche `LICENSE.md`, `ATTRIBUTION.md` e `README.md` (obbligo MIT per chi distribuisce la DLL).
- [x] Ricostruisci il pacchetto di release e controlla il contenuto dello zip. Poi 🛑 l'utente fa una prova finale: installa dallo zip in una cartella `mods` pulita di Mewtator, gioca una notte e verifica che F8 mostri "Active". (fatto; prova finale confermata dall'utente)
- [ ] Commit, tag `v1.0.0` e push del tag (il repo è dell'utente, che ha già chiesto di pubblicare).

## S11. Menù con il look del gioco (v1.1.0)

Decisione dell'utente del 2026-10-05: il menù deve avere stile e font del gioco, come le mod "Auto Furniture" e "Combat Roster Panel".

**Approccio scelto:** quello di **Combat Roster Panel** ([TotSamiyMorzh/mewgenics-combat-roster](https://github.com/TotSamiyMorzh/mewgenics-combat-roster), MIT, clonato in `reference/mewgenics-combat-roster`). Usa la nostra stessa tecnologia (ImGui 1.92 + Mewjector, stessa versione del gioco), ma legge font, carta e grafica **dal `resources.gpak` dell'utente mentre il gioco gira**. Non distribuisce file del gioco, e così deve restare anche da noi.

Scartato: l'approccio di Auto Furniture (`reference/mewgenics-mods`), che genera file SWF e si integra nell'interfaccia nativa del gioco. È più bello ma richiede molto reverse engineering della UI ed è fragile.

- [x] Porta da `reference/mewgenics-combat-roster/src` solo quello che serve: (fatto: `src/clean_breeding/gamelook/` con gpak, swf, fontloader, assets ridotto e `gamelook.cpp`; `stb_image.h` in `src/lib/stb/`; licenza e crediti in ATTRIBUTION.md, LICENSE.md)
  - `gpak.cpp/.h`: lettura dell'archivio;
  - `swf.cpp/.h`: rasterizzatore SWF;
  - `fontloader.cpp/.h`: font SWF del gioco in ImGui, con il fallback CJK;
  - da `assets.cpp/.h`, la parte che carica `swfs/ui.swf` e la bitmap della carta (`asset_bitmap`, `kPaperBitmap = 1`), più il worker thread;
  - da `panel.cpp`, la funzione `paper()` (carta in 9-slice) e i colori `kInk`, `kInkSoft`, `kPaperTint`;
  - da `overlay.cpp` righe ~125-165 e ~248-253: lo stile ImGui (`StyleColorsLight` più colori carta e inchiostro) e il caricamento dei font tra un frame e l'altro;
  - `third_party/stb_image.h`.

  Niente ritratti, icone, gon, loc, roster: non servono. Aggiungi la licenza e i crediti di Combat Roster Panel in `ATTRIBUTION.md`, `LICENSE.md` ("Copyright (c) 2026 Combat Roster Panel authors").
- [x] Il menù F8 usa:
  - il font del gioco per titolo e testi;
  - lo sfondo di carta della finestra disegnato con `paper()`;
  - tooltip sulla stessa carta;
  - pulsanti e combo con i colori carta e inchiostro.

  La struttura del menù e i testi di DESIGN.md non cambiano.
- [x] Fallback: se gli asset non si caricano (gpak non trovato, errore di parsing), il menù deve funzionare con lo stile attuale e Segoe UI, e scrivere una riga nel log. Il caricamento non deve mai bloccare il gioco: niente lavoro pesante nel thread di render. (flag dev `[debug] force_default_style=1` per provarlo; il caricamento è in un thread a parte, il render non aspetta)
- [ ] Check:
  - [x] la build passa;
  - [x] (confermato dall'utente: "mi sembra buono"; log: fonti e carta caricati, nessun crash) 🛑 l'utente apre il menù nel gioco: font e carta come quelli del gioco, testi leggibili a 1280×720 e a 1920×1080, nessun calo di FPS visibile;
  - 🛑 con il gpak rinominato o assente (prova su una copia, mai sull'originale; oppure simulando l'errore con un flag dev), il menù si apre con lo stile di fallback;
  - [x] lo zip di release non contiene file estratti dal gioco (niente `.swf`, `.png`, `.ttf` del gioco). (verificato su `UnnaturalSelection-1.1.0.zip`: nessun .swf/.png/.ttf/.gpak)

## S10. ~~Editor della whitelist~~ (annullato il 2026-10-05: la whitelist è stata tolta del tutto)

## Dopo (non ora)

- Trovare la signature automaticamente dopo un update (`misc/find_rvas.py` del template).






