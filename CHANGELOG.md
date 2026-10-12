## 2026-10-12
- fix(tracker): duplicate STATE=COMPLETE logging spawned a phantom empty record right after every game (sess-20261009-1745-ee80b668)
- fix(tracker): duplicate CREATE_GAME (GameState + indented PowerTaskList copy, same instant) fractured every live game a few hundred lines in, hiding the player's own drawn cards for the rest of the match; fold-text now trusts only GameState's own CREATE_GAME to abort; tests/e2e.sh extended against a new real fixture (testdata/real_duplicate_create_game.log); resynced with IDUNA's vendored copy (sess-20261009-1745-ee80b668)

- Resynced tracker/game_state.prn with IDUNA's vendored copy (card0 + CHANGE_ENTITY + PowerTaskList fixes, see IDUNA Apple #22235); fixed tests/e2e.sh's card0-broken substring check; corrected CLAUDE.md's 'no code yet' claim to note the real tracker/ subsystem. (sess-20261009-1745-ee80b668)

## 2026-10-10

- tracker --live; Decks.log as deck source; opening-hand me detection (sess-20261009-1745-ee80b668)

## 2026-10-09

- tracker: per-game JSON (revealed cards w/ owner, replay timeline, me detection), validated on 2 real Power.logs / 5 games; PARENA logic + thin C host with native arenas (sess-20261009-1745-ee80b668)

