# HRIP tracker — Hearthstone deck/game tracker (PARENA logic, thin C host)

Tails Hearthstone's `Power.log` and emits **one JSON record per finished game**: header, both
players' revealed cards (the substrate for opponent-deck inference and matchup stats), and an
ordered **replay timeline**. Logic is PARENA; C is only the file/poll/arena host.

```
make test PARENA=../PARENA      # build + end-to-end test on a real (scrubbed) Power.log excerpt
build/tracker <Power.log> [--once]
```

## Record shape
```
{"game_type":"GT_RANKED","format":"FT_STANDARD","player1":"…","player2":"…",
 "result1":1,"result2":2,"turns":25,"complete":1,          # result: 1 won, 2 lost, 3 tied, 0 unknown
 "entities":[{"id":28,"player":1,"card":"CORE_WON_141","zone0":"DECK"}, …],
 "timeline":[["t",turn], ["z",turn,entity,player,card,zone], ["p",turn,entity,player,card], …]}
```
- `entities`: every card that was *revealed*, once, with its owner and the zone it first appeared in.
  `zone0` DECK/HAND = part of that player's deck; anything else = created by an effect. For the
  opponent this is the "cards seen" list = their deck as far as it was revealed.
- `timeline`: replay stream in log order (turn starts, zone changes, plays), each resolved to
  owner + card. Hearthstone has no local engine to re-run (unlike DEADWEIGHT's seed+inputs replay),
  so the log *is* the replay.

## Files
| File | Role |
|---|---|
| `power_log.prn` | scanning helpers (find, token, int, name) |
| `game_state.prn` | `defstruct Fact`/`GameState`, per-line handlers, `fold-text` loop, summary JSON |
| `host.c` | file offset, one static buffer, two native `Arena`s (scratch reset per fold, game reset per game), sleep loop. No stack buffers, no malloc |

## Validated against real logs
Two real `Power.log` files (build 253216, 5 complete games: ranked + friendly): plays, final turn
and results match an independent Python count on all 5. 82k-line / 10 MB log folds in ~0.7 s.
Real-log facts the parser handles: CRLF line endings; every event logged twice (only `GameState.*`
is read); opponent name `UNKNOWN HUMAN PLAYER` at game start (result assigned to the leftover
slot); abandoned games (next `CREATE_GAME` with no `STATE=COMPLETE`) emit `complete:2`.

## Not done yet
- Not yet validated for: Battlegrounds/Mercenaries/Arena formats, reconnects mid-game, `log.config`
  installer, Mac paths. A game still running at EOF is held, not emitted.
- Hero/class identification (hero entity → class) and deck-code reconstruction need the card DB
  (`hs_cards`) join, done server-side.
- WOTAN side: `hs_games` ingest + matchup/win-rate views + replay viewer (same pattern as the
  DEADWEIGHT match viewer). Windows build and the overlay UI are untested/unbuilt.
- Generated PARENA code copies structs by value on the stack (largest frame ~370 B, build limit
  `-Wframe-larger-than=512`); the host itself keeps no stack buffers.
