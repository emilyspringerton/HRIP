/* tests/test_game_state.c -- folds a synthetic multi-line chunk through game_state.prn. */
#ifndef PARENA_NO_GRAPHICS
#define PARENA_NO_GRAPHICS
#endif
#include "parena_runtime.h"
#include <stdio.h>
#include <string.h>
#include "gs_gen.c"

static int failures = 0;
#define CHECK(c, m) do { if (!(c)) { printf("FAIL: %s\n", m); failures++; } else printf("PASS: %s\n", m); } while (0)
static Arena arena;
static GameState st;

static const char *CHUNK =
  "D 1 GameState.DebugPrintPower() - CREATE_GAME\n"
  "D 2 GameState.DebugPrintPower() -     FULL_ENTITY - Creating ID=4 CardID=\n"
  "D 3 GameState.DebugPrintPower() -     FULL_ENTITY - Creating ID=5 CardID=CORE_EX1_012\n"
  "D 4 GameState.DebugPrintPower() -     TAG_CHANGE Entity=GameEntity tag=TURN value=3\n"
  "D 5 GameState.DebugPrintPower() -     TAG_CHANGE Entity=[entityName=Mana Wyrm id=43 zone=HAND zonePos=2 cardId=CORE_EX1_012 player=1] tag=ZONE value=PLAY\n"
  "D 6 Noise.Line() - nothing here\n"
  "D 7 GameState.DebugPrintPower() -     TAG_CHANGE Entity=GameEntity tag=STATE value=COMPLETE\n"
  "D 8 GameState.DebugPrintPower() -     TAG_CHANGE Entity=GameEntity tag=TURN value=9";  /* partial: no \n */

int main(void) {
    arena_init(&arena);
    st = new_state();
    st = fold_text((char *)CHUNK, st, &arena);
    CHECK(st.events == 6, "6 recognised events (partial last line not consumed)");
    CHECK(st.entities == 2, "2 FULL_ENTITY");
    CHECK(st.played == 1, "1 ZONE->PLAY");
    CHECK(st.turn == 3, "turn=3 (turn 9 line was partial)");
    CHECK(st.complete == 1, "STATE=COMPLETE seen");
    arena_free_all(&arena);
    printf("%s (%d failures)\n", failures ? "FAILED" : "ALL PASS", failures);
    return failures != 0;
}
