# HRIP tracker — Hearthstone deck tracker (PARENA logic, thin C host)

Tails Hearthstone's `Power.log` (enabled by `log.config`), parses game events, and folds them into
a game summary. Logic is PARENA; C is only the file/poll/arena host.

| File | Role |
|---|---|
| `power_log.prn` | line parser: kind, entity id, tag/value, card id (`defstruct LogEvent`, `loop`/`recur` scanning) |
| `game_state.prn` | `defstruct GameState` + `apply-event` + `fold-text` (loop over newline-terminated lines; a trailing partial line is left for the next poll) |
| `host.c` | file offset, static buffer, native `Arena` (reset per fold), sleep loop. No stack buffers, no malloc; built with `-Werror -Wframe-larger-than=256` |

```
make test PARENA=../PARENA        # 12 parser + 5 state assertions
make tracker PARENA=../PARENA     # build/tracker
build/tracker <Power.log> [--once]
```

## Status (honest)
- Works on **synthetic** fixtures written from the documented HearthSim line shape. **Not yet
  validated against a real `Power.log`** — the first real log will likely change the parser.
- Game state is counters only (events, entities, ZONE→PLAY, turn, complete). Per-entity zone
  tracking (hand/deck/graveyard), player/deck identification and the WOTAN `hs_games` upload are
  next, and need real log data.
- Needs only PARENA's runtime with `PARENA_NO_GRAPHICS` (no SDL). Windows build untested.
- Caught by tests during bring-up: `ID=` matching inside `CardID=` (now anchored).
