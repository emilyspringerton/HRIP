/* tests/test_power_log.c -- exercises tracker/power_log.prn via PARENA-generated C.
 * Fixture lines are SYNTHETIC (from the documented HearthSim shape), not a real Power.log. */
#include "parena_runtime.h"
#include <stdio.h>
#include <string.h>
#include "pl_gen.c"

static int failures = 0;
#define CHECK(c, m) do { if (!(c)) { printf("FAIL: %s\n", m); failures++; } else printf("PASS: %s\n", m); } while (0)

static Arena arena;   /* file scope: nothing buffer-like lives on the C stack */

static const char *L_TAG =
  "D 12:34:56.7890123 GameState.DebugPrintPower() -     TAG_CHANGE Entity=[entityName=Mana Wyrm id=43 zone=HAND zonePos=2 cardId=CORE_EX1_012 player=1] tag=ZONE value=PLAY";
static const char *L_FULL =
  "D 12:34:56.7890123 GameState.DebugPrintPower() -     FULL_ENTITY - Creating ID=5 CardID=CORE_EX1_012";
static const char *L_SHOW =
  "D 12:34:56.7890123 GameState.DebugPrintPower() -     SHOW_ENTITY - Updating Entity=22 CardID=CS2_029";
static const char *L_GAME =
  "D 12:34:56.7890123 GameState.DebugPrintPower() - CREATE_GAME";
static const char *L_NOISE = "D 12:34:56.7890123 LoadingScreen.OnSceneLoaded() - prevMode=HUB";

int main(void) {
    arena_init(&arena);
    LogEvent e;

    e = parse_line((char *)L_TAG, &arena);
    CHECK(e.kind == 1, "TAG_CHANGE kind");
    CHECK(e.entity_id == 43, "bracketed id=43");
    CHECK(strcmp(e.tag, "ZONE") == 0, "tag=ZONE");
    CHECK(strcmp(e.value, "PLAY") == 0, "value=PLAY");

    e = parse_line((char *)L_FULL, &arena);
    CHECK(e.kind == 3, "FULL_ENTITY kind");
    CHECK(e.entity_id == 5, "ID=5");
    CHECK(strcmp(e.card_id, "CORE_EX1_012") == 0, "CardID");

    e = parse_line((char *)L_SHOW, &arena);
    CHECK(e.kind == 2, "SHOW_ENTITY kind");
    CHECK(e.entity_id == 22, "numeric Entity=22");
    CHECK(strcmp(e.card_id, "CS2_029") == 0, "SHOW CardID");

    e = parse_line((char *)L_GAME, &arena);
    CHECK(e.kind == 4, "CREATE_GAME kind");

    e = parse_line((char *)L_NOISE, &arena);
    CHECK(e.kind == 0 && e.entity_id == -1, "noise line ignored");

    arena_free_all(&arena);
    printf("%s (%d failures)\n", failures ? "FAILED" : "ALL PASS", failures);
    return failures != 0;
}
