/* tracker/host.c -- thin C host for the PARENA tracker. Owns ONLY: the file offset, one static
 * read buffer, the native Arena, and the poll/sleep loop. All parsing and game logic is PARENA
 * (power_log.prn, game_state.prn).
 *
 * Memory rules (founder: "avoid using the stack in C"):
 *   - no local arrays/VLAs/alloca anywhere; every buffer is file-scope static;
 *   - no malloc/free here; PARENA's native Arena is reset (arena_free_all + arena_init) after each
 *     fold, so transient strings never accumulate;
 *   - bounded: a line longer than the buffer is dropped and counted, never truncated silently.
 * Build adds -Wframe-larger-than=256 so a stray stack buffer fails the build.
 *
 * usage: tracker <Power.log> [--once]     (--once: read to EOF, print summary, exit)
 */
#ifndef PARENA_NO_GRAPHICS
#define PARENA_NO_GRAPHICS
#endif
#include "parena_runtime.h"
#include <stdio.h>
#include <string.h>
#include "tracker_gen.c"
#ifdef _WIN32
#include <windows.h>
#endif

#define BUF_CAP (1 << 20)

static Arena arena;
static GameState st;
static GameState last_printed;
static char buf[BUF_CAP];
static size_t have;          /* bytes of an unconsumed partial line kept at the buffer front */
static long offset;          /* file read position */
static unsigned dropped;     /* oversize lines discarded */
static int printed_once;

static void sleep_ms(int ms) {
#ifdef _WIN32
    Sleep((DWORD)ms);
#else
    usleep((useconds_t)ms * 1000);
#endif
}

static void print_state(void) {
    printf("events=%d entities=%d played=%d turn=%d complete=%d dropped=%u\n",
           st.events, st.entities, st.played, st.turn, st.complete, dropped);
    fflush(stdout);
    last_printed = st;
    printed_once = 1;
}

/* one poll: returns bytes newly read (0 = nothing new / file missing) */
static size_t poll_once(const char *path) {
    FILE *f = fopen(path, "rb");
    size_t n, i, last;
    char saved;
    if (!f) return 0;
    fseek(f, 0, SEEK_END);
    if (ftell(f) < offset) { offset = 0; have = 0; st = new_state(); } /* file rotated/truncated: new session */
    fseek(f, offset, SEEK_SET);
    n = fread(buf + have, 1, BUF_CAP - 1 - have, f);
    fclose(f);
    offset += (long)n;
    have += n;
    if (n == 0) return 0;

    last = (size_t)-1;                                  /* index of last '\n' */
    for (i = 0; i < have; i++) if (buf[i] == '\n') last = i;
    if (last == (size_t)-1) {
        if (have >= BUF_CAP - 1) { dropped++; have = 0; } /* full buffer, no newline: drop */
        return n;
    }
    saved = buf[last + 1];
    buf[last + 1] = '\0';
    arena_init(&arena);
    st = fold_text(buf, st, &arena);                     /* <- all the logic is PARENA */
    arena_free_all(&arena);
    buf[last + 1] = saved;
    have -= last + 1;
    memmove(buf, buf + last + 1, have);
    return n;
}

int main(int argc, char **argv) {
    int once = (argc > 2 && strcmp(argv[2], "--once") == 0);
    if (argc < 2) { fprintf(stderr, "usage: %s <Power.log> [--once]\n", argv[0]); return 2; }
    st = new_state();
    last_printed = st;
    for (;;) {
        size_t n = poll_once(argv[1]);
        if (memcmp(&st, &last_printed, sizeof st) != 0 || (!printed_once && once)) print_state();
        if (once && n == 0) break;
        if (n == 0) sleep_ms(500);
    }
    if (!printed_once) print_state();
    return 0;
}
