# Clean Breeding: design

Decisioni prese con l'utente il 2026-10-05. Questo file dice **cosa** fa la mod e **perché**.
Il **come**, step per step, sta in [PLAN.md](PLAN.md). Non cambiare le decisioni senza chiedere all'utente.

## Obiettivo

Mod DLL per Mewgenics (caricata da Mewjector, gestita da Mewtator) con un menù in-game (ImGui) che:

1. riduce o elimina gli effetti genetici dell'**inbreeding**;
2. riduce o elimina l'**eredità dai genitori** di disordini e difetti fisici;
3. offre un pulsante **Cleanse** che rimuove i tratti negativi da tutti i gatti.

Una **whitelist** protegge i tratti "buoni" da tutte e tre le funzioni.

## Meccanica di breeding (fonti: wiki, Breeding Manager, cat-bridge)

Tutto avviene in una funzione del gioco:
`glaiel::CatData::breed(CatData* kitten, CatData* parentA, CatData* parentB, double coi, void* furniture_effects)`.

| Tiro | Formula (v1.1) | Dipende da |
|---|---|---|
| Disordine ereditato | 15% per genitore, 1 disordine casuale di quel genitore | eredità |
| Disordine "da inbreeding" | `max(2%, 0.4·coi − 6%)` (max 34%), solo se sono stati ereditati < 2 disordini | inbreeding (incluso il 2% minimo) |
| Difetto nuovo | `1.5·coi` (certo da coi 66.6%) | inbreeding |
| Eredità parte del corpo | per slot, da un genitore; normale contro difetto: `50% + 50%·(Stim − 2·coi%)/(200 + |…|)` | eredità, più il malus `−2·coi%` dell'inbreeding |

Fatti verificati da altri (cat-bridge, `reference/mewgenics-cat-bridge/docs/DEVELOPMENT.md`):
- `coi` del gattino = parentela (kinship) tra i genitori; il gioco lo salva in `CatData::coi` e nel pedigree.
- Parti del corpo: `BodyPartDescriptor::part_sprite_idx` (e `BodyParts::texture_sprite_idx`) indicizzano `data/mutations/<gruppo>.gon`. Gli id `>= 300` sono mutazioni, `-2` (`0xFFFFFFFE`) è una parte mancante, cioè un difetto. Braccia e gambe usano entrambe `legs.gon`.
- È un difetto ogni id che nel suo `.gon` ha `tag birth_defect` (quasi tutti i 700+, e tutti i `-2`).
- Disordini: slot `CatData::mutation_0` / `mutation_1` (stringhe MSVC più `int64` livello).
- Le modifiche in memoria a disordini e parti **sopravvivono al salvataggio e ricaricamento del gioco** (verificato da cat-bridge).

## Livelli

Ogni asse ha 4 livelli. Si salvano in `config.ini` come numero: 0 = Vanilla, 1 = Mite, 2 = Nessuno, 3 = Duro.
I numeri restano questi anche se il menù li mostra in ordine di difficoltà (Duro, Vanilla, Mite, Nessuno).

| Asse | 3 Duro (hard mode) | 0 Vanilla | 1 Mite | 2 Nessuno |
|---|---|---|---|---|
| **Inbreeding** | `breed` riceve `min(1, coi × 2)` | invariato | `breed` riceve `coi × 0.5` | `breed` riceve `coi = 0`; in più si rimuove dal gattino ogni disordine (non in whitelist) che nessun genitore aveva (= il tiro da inbreeding, compreso il 2% minimo) |
| **Eredità** | ogni tratto negativo dei genitori ha una **seconda possibilità** di passare (vedi sotto) | invariato | ogni disordine e ogni parte difettosa dei genitori ha il 50% di probabilità di essere bloccata | nessun disordine né parte difettosa passa dai genitori |

Effetto atteso del livello Duro:
- **Inbreeding:** una coppia con coi 25% si comporta come se avesse coi 50%. I difetti nuovi passano dal 37.5% al 75%, il disordine da inbreeding dal 4% al 14%, e raddoppia anche il malus `−2·coi%` sull'eredità delle parti difettose. Il gattino tiene il coi vero, come negli altri livelli.
- **Eredità:** i disordini passano circa dal 15% al 27.75% per genitore (`1 − 0.85²`). Le parti difettose passano da `p` a `p + (1 − p)·0.5`.
- I tratti in whitelist non sono mai amplificati: seguono le regole vanilla.

Preset nel menù:
- Vanilla: 0/0
- Assistito: 1/1
- Libero incrocio: 2/0
- Genetica perfetta: 2/2
- Hard mode: 3/3

Cleanse ha 3 modalità: `disorders` (solo disordini), `defects` (solo parti difettose), `all`. Pulisce **tutto** ciò che è negativo, compresi i disordini presi giocando (malattie, eventi), tranne la whitelist.

## Come funziona l'hook su `breed`

```
real_coi = coi
coi' = {0: coi, 1: coi × 0.5, 2: 0, 3: min(1, coi × 2)}[inbreeding]
if heredity in {1, 2}:
    for each parent, each disorder slot, if not whitelisted and (heredity == 2 or rand < 0.5):
        nascondi lo slot: scambia i byte della stringa con una stringa locale "None" e metti il livello a 1 (lo slot vuoto del gioco, verificato in S3), poi ripristina stringa e livello
orig(kitten, A, B, coi', furniture)
ripristina gli slot dei genitori (sempre, anche se orig fallisce)
kitten->coi = real_coi                      # il gattino resta "Inbred" e il pedigree è corretto
if inbreeding == 2:
    rimuovi dal gattino i disordini non in whitelist che non erano in A né in B (lista originale)
if heredity in {1, 2}:
    for each slot di parte del corpo del gattino:
        if è un difetto non in whitelist, uguale alla parte di A o di B nello stesso slot,
           and (heredity == 2 or rand < 0.5):
            sostituisci con la parte dell'altro genitore se normale,
            else con la parte simmetrica del gattino se normale,
            else con una parte generata dal gioco (funzione stray bodyparts di Amoeba)
if heredity == 3:                            # hard mode: seconda possibilità
    for each genitore P con disordini non in whitelist:
        if il gattino non ha nessun disordine di P, ha uno slot libero, and rand < 0.15:
            copia nel gattino un disordine casuale (non in whitelist) di P, con il suo livello
    for each slot di parte del corpo:
        if il gattino ha una parte normale, un genitore ha un difetto non in whitelist in quello slot,
           and rand < 0.5:
            copia quel difetto nel gattino (se entrambi i genitori ce l'hanno, scegline uno a caso)
```

Casi limite accettati:
- Un difetto *nuovo* da inbreeding che per caso coincide con la parte di un genitore viene trattato come ereditato.
- Un disordine da inbreeding che coincide con uno dei genitori viene trattato come ereditato.

Sono entrambi rari.

## Whitelist

Valori di default in `mod/CleanBreeding/config.ini`, scelti così:
- **disordini puramente positivi, o scelti dal giocatore tramite un evento** (fontana, desideri MonkeyPaw, idolo demoniaco, Glorg);
- **difetti fisici che danno accesso ad abilità di altre classi** o che sono usati nelle build (Cyclops).

I difetti si identificano come `<gruppo>:<id>`, perché lo stesso id 700 esiste in ogni file `.gon` del gruppo.

La whitelist agisce ovunque:
- un tratto in whitelist non viene mai nascosto ai genitori, quindi si eredita con le regole vanilla;
- non viene mai rimosso dal gattino;
- il Cleanse non lo tocca.

Con "Genetica perfetta" (coi = 0) non nascono difetti *nuovi*, nemmeno quelli in whitelist. I tratti buoni si mantengono solo per eredità.

Modificare la whitelist dal menù è un lavoro futuro: per ora si modifica il file `.ini` a gioco chiuso.

## Menù

Overlay Dear ImGui sopra il rendering del gioco (OpenGL via SDL3), copiato dall'approccio di Amoeba (`reference/mewgenics_analysis/cpp/amoeba/amoeba_imgui.cpp`, hook `SDL_GL_SwapWindow` + `SDL_PollEvent`). Si apre e chiude con **F8**.

Contenuto:
- 2 selettori a 4 livelli (Duro / Vanilla / Mite / Nessuno);
- 5 pulsanti preset;
- selettore della modalità Cleanse e pulsante "Cleanse all cats" con conferma, che mostra quanti gatti e tratti verranno toccati;
- riga di stato (hook attivo / versione del gioco non supportata).

Ogni modifica si salva subito nel `.ini` (`WritePrivateProfileStringW`).

## Sicurezza

- All'avvio la mod controlla lo SHA256 dell'exe e le signature. Se una manca, non installa nessun hook e il menù mostra "versione del gioco non supportata".
- Prima del Cleanse fa un backup automatico: copia ogni `*.sav` di `%APPDATA%\Glaiel Games\Mewgenics\*\saves\` in `...\saves\backups\cleanbreeding_<timestamp>_<nome>.sav`. Se la copia fallisce, il Cleanse non parte.
- Nessuna scrittura diretta sul file `.sav`. Si modifica la memoria e poi è il gioco a salvare.
