# Unnatural Selection: design

Decisioni prese con l'utente il 2026-10-05. Questo file dice **cosa** fa la mod e **perché**.
Il **come**, step per step, sta in [PLAN.md](PLAN.md). Non cambiare le decisioni senza chiedere all'utente.

## Obiettivo

Mod DLL per Mewgenics (caricata da Mewjector, gestita da Mewtator) con un menù in-game (ImGui) che:

1. riduce o elimina gli effetti genetici dell'**inbreeding**;
2. riduce o elimina l'**eredità dai genitori** di disordini e difetti fisici.

Niente altro: due regolatori e 5 preset.

**Aggiornamento 2026-10-05, deciso con l'utente:**
- **Il Cleanse (il pulsante che puliva tutti i gatti) è eliminato del tutto.** Nessuna modalità e nessun pulsante, né nella release né nella build di sviluppo. Si toglie anche il codice già scritto (selettore di modalità, `CleanseMode`, sezione `[cleanse]` del `.ini`).
- **Nessuna whitelist**, né per i disordini né per i difetti. Ogni disordine e ogni difetto di nascita conta come negativo, senza eccezioni: anche Eternal Youth, Savant, Cyclops. Chi vuole che i tratti buoni passino ai figli usa Eredità Vanilla. Si toglie anche il codice già scritto (`whitelist_disorders`, `whitelist_defects`, sezione `[whitelist]` del `.ini`); salta anche l'editor previsto in S10.
- **Gli strumenti di sviluppo restano fuori dalla release:** sezione Debug del menù (simulatore, test suite), test helper `[debug]` di `config.ini`, snapshot.

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
| **Inbreeding** | `breed` riceve `min(1, coi × 2)` | invariato | `breed` riceve `coi × 0.5` | `breed` riceve `coi = 0`; in più si rimuove dal gattino ogni disordine che nessun genitore aveva (= il tiro da inbreeding, compreso il 2% minimo) |
| **Eredità** | ogni tratto negativo dei genitori ha una **seconda possibilità** di passare (vedi sotto) | invariato | ogni disordine e ogni parte difettosa dei genitori ha il 50% di probabilità di essere bloccata | nessun disordine né parte difettosa passa dai genitori |

Effetto atteso del livello Duro:
- **Inbreeding:** una coppia con coi 25% si comporta come se avesse coi 50%. I difetti nuovi passano dal 37.5% al 75%, il disordine da inbreeding dal 4% al 14%, e raddoppia anche il malus `−2·coi%` sull'eredità delle parti difettose. Il gattino tiene il coi vero, come negli altri livelli.
- **Eredità:** i disordini passano circa dal 15% al 27.75% per genitore (`1 − 0.85²`). Le parti difettose passano da `p` a `p + (1 − p)·0.5`.
Preset nel menù:
- Vanilla: 0/0
- Assistito: 1/1
- Libero incrocio: 2/0
- Genetica perfetta: 2/2
- Hard mode: 3/3

## Come funziona l'hook su `breed`

```
real_coi = coi
coi' = {0: coi, 1: coi × 0.5, 2: 0, 3: min(1, coi × 2)}[inbreeding]
if heredity in {1, 2}:
    for each parent, each disorder slot non vuoto, if heredity == 2 or rand < 0.5:
        nascondi lo slot: scambia i byte della stringa con una stringa locale "None" e metti il livello a 1 (lo slot vuoto del gioco, verificato in S3), poi ripristina stringa e livello
orig(kitten, A, B, coi', furniture)
ripristina gli slot dei genitori (sempre, anche se orig fallisce)
kitten->coi = real_coi                      # il gattino resta "Inbred" e il pedigree è corretto
if inbreeding == 2:
    rimuovi dal gattino i disordini che non erano in A né in B (lista originale)
if heredity in {1, 2}:
    for each slot di parte del corpo del gattino:
        if è un difetto, uguale alla parte di A o di B nello stesso slot,
           and (heredity == 2 or rand < 0.5):
            sostituisci con la parte dell'altro genitore se normale,
            else con la parte simmetrica del gattino se normale,
            else con una parte generata dal gioco (funzione stray bodyparts di Amoeba)
if heredity == 3:                            # hard mode: seconda possibilità
    for each genitore P con disordini:
        if il gattino non ha nessun disordine di P, ha uno slot libero, and rand < 0.15:
            copia nel gattino un disordine casuale di P, con il suo livello
    for each slot di parte del corpo:
        if il gattino ha una parte normale, un genitore ha un difetto in quello slot,
           and rand < 0.5:
            copia quel difetto nel gattino (se entrambi i genitori ce l'hanno, scegline uno a caso)
```

Casi limite accettati:
- Un difetto *nuovo* da inbreeding che per caso coincide con la parte di un genitore viene trattato come ereditato.
- Un disordine da inbreeding che coincide con uno dei genitori viene trattato come ereditato.

Sono entrambi rari.

## Nomi e testi (decisi il 2026-10-05)

Regole di stile: inglese, parole semplici, una riga per spiegazione, nessun termine tecnico (coi, hook, slot) nel menù.

**Perché questi nomi:**
- "None" era ambiguo: "Heredity: None" sembrava "nessuna ereditarietà", cioè niente stat né abilità dai genitori.
- "Hard" stonava accanto a livelli che descrivono un'intensità.
- "Perfect genetics" prometteva stat perfette.

I nuovi nomi dicono **cosa** viene regolato (penalità e difetti) e **quanto**.

**Assi** (etichetta, poi riga grigia sotto):

| Asse (chiave `.ini`) | Etichetta | Spiegazione nel menù |
|---|---|---|
| `inbreeding` | Inbreeding penalties | New disorders and birth defects caused by breeding related cats. |
| `heredity` | Inherited flaws | Disorders and birth defects passed down from the parents. |

**Livelli** (in quest'ordine nel menù; il valore nel `.ini` resta quello di sempre):

| Valore `.ini` | Livello | Tooltip: Inbreeding penalties | Tooltip: Inherited flaws |
|---|---|---|---|
| 2 | Off | Related parents count as unrelated. | Parents never pass on disorders or birth defects. |
| 1 | Reduced | Inbreeding counts half. | Half the usual chance. |
| 0 | Normal | Game default. | Game default. |
| 3 | Increased | Inbreeding counts double. | Flaws get a second chance to pass on. |

**Preset** (tooltip di una riga):

| Preset | Valori (inbreeding/heredity) | Tooltip |
|---|---|---|
| Vanilla | 0/0 | The game's own rules. |
| Gentle | 1/1 | Half the penalties and flaws. |
| Carefree | 2/0 | Breed relatives freely; parents still pass on their own flaws. |
| Clean | 2/2 | No disorders or birth defects from breeding. |
| Hardcore | 3/3 | Inbreeding hits harder and flaws spread more. |

In fondo al menù: "F8: show/hide · Settings are saved automatically".
Stato: "Active · Mewgenics 1.1.21239" (verde) oppure "Inactive: unsupported game version (needs 1.1.21239)" (rosso).

Cosa la mod **non** tocca (da dire nel README): abilità attive e passive, stat, mutazioni normali, gatti già nati. Agisce solo nel momento in cui nasce un gattino.

## Menù

Overlay Dear ImGui sopra il rendering del gioco (OpenGL via SDL3), copiato dall'approccio di Amoeba (`reference/mewgenics_analysis/cpp/amoeba/amoeba_imgui.cpp`, hook `SDL_GL_SwapWindow` + `SDL_PollEvent`). Si apre e chiude con **F8**.

Contenuto:
- 2 selettori a 4 livelli (Off / Reduced / Normal / Increased), ognuno con una riga di spiegazione;
- 5 pulsanti preset con tooltip;
- riga di stato e riga di aiuto (testi nella sezione "Nomi e testi");
- solo nella build di sviluppo: un header "Developer tools" in fondo (simulatore, test suite).

Ogni modifica si salva subito nel `.ini` (`WritePrivateProfileStringW`).

## Sicurezza

- All'avvio la mod controlla lo SHA256 dell'exe e le signature. Se una manca, non installa nessun hook e il menù mostra "versione del gioco non supportata".
- Nessuna scrittura sul file `.sav`. La mod agisce solo sui gattini appena nati, in memoria, e poi è il gioco a salvare.
