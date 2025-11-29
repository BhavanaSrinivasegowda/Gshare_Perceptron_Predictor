#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>

#include "predictor.h"   // your predictor header

// external configuration variables from predictor.c
extern int ghistoryBits;
extern int globalBits;
extern int tournamentBits;
extern int phtIndexBits;
extern int phtBits;
extern int bpType;
extern int verbose;

extern uint8_t perceptronIndexBits;
extern uint8_t perceptronHistoryLen;
extern int twobitIndexBits;

static void usage(char *prog) {
    fprintf(stderr,
        "Usage: %s <tracefile> [options]\n\n"
        "Trace format: <hex_pc> <1/0>\n\n"
        "Predictor Selection:\n"
        "  --gshare:<bits>          Use GShare predictor\n"
        "  --perceptron             Use Perceptron predictor\n"
        "  --tournament             Use Tournament predictor\n"
        "  --custom                 Use Custom (GShare+Perceptron)\n"
        "  --2bit                   Use 2-bit predictor\n\n"
        "Optional Parameters:\n"
        "  --percep-index:N         Perceptron index bits\n"
        "  --percep-history:N       Perceptron history length\n"
        "  --globalbits:N           Global BHT bits\n"
        "  --pht-index:N            Local PHT index bits\n"
        "  --pht-bits:N             Local history bits\n"
        "  --tournament-bits:N      Chooser table bits\n"
        "  --twobit-index:N         2-bit table index bits\n"
        "  --verbose                Print detailed logs\n",
        prog  // <-- pass the program name here
    );
}



static int parse_colon(const char *arg, const char *prefix, int *out) {
    size_t n = strlen(prefix);
    if (strncmp(arg, prefix, n) != 0) return 0;

    const char *p = arg + n;
    if (*p == ':' || *p == '=') p++;

    errno = 0;
    long v = strtol(p, NULL, 10);
    if (errno != 0) return 0;

    *out = (int)v;
    return 1;
}


int main(int argc, char **argv)
{
    if (argc < 2) {
        usage(argv[0]);
        return 1;
    }

    const char *tracefile = NULL;
    bpType = GSHARE;   // default
    verbose = 0;

    // Parse command-line arguments
    for (int i = 1; i < argc; i++) {
        const char *a = argv[i];

        if (a[0] != '-' && tracefile == NULL) {
            tracefile = a;
            continue;
        }

        if (strcmp(a, "--perceptron") == 0) {
            bpType = STATIC;
        }
        else if (strcmp(a, "--tournament") == 0) {
            bpType = TOURNAMENT;
        }
        else if (strcmp(a, "--custom") == 0) {
            bpType = CUSTOM;
        }
        else if (strcmp(a, "--2bit") == 0) {
            bpType = TWOBIT;
        }
        else if (parse_colon(a, "--gshare", &ghistoryBits)) {
            bpType = GSHARE;
        }
        else if (parse_colon(a, "--percep-index", (int *)&perceptronIndexBits)) {}
        else if (parse_colon(a, "--percep-history", (int *)&perceptronHistoryLen)) {}
        else if (parse_colon(a, "--globalbits", &globalBits)) {}
        else if (parse_colon(a, "--pht-index", &phtIndexBits)) {}
        else if (parse_colon(a, "--pht-bits", &phtBits)) {}
        else if (parse_colon(a, "--tournament-bits", &tournamentBits)) {}
        else if (parse_colon(a, "--twobit-index", &twobitIndexBits)) {}
        else if (strcmp(a, "--verbose") == 0) {
            verbose = 1;
        }
        else {
            fprintf(stderr, "Unknown argument: %s\n", a);
            return 1;
        }
    }

    if (!tracefile) {
        fprintf(stderr, "Error: No trace file provided!\n");
        return 1;
    }

    FILE *fp = fopen(tracefile, "r");
    if (!fp) {
        perror("fopen");
        return 1;
    }

    init_predictor();

    long line = 0;
    uint64_t total = 0, mispred = 0;
    uint64_t pc;
    int outcome_int;

    while (fscanf(fp, "%lx %d", &pc, &outcome_int) == 2) {
        uint8_t actual = (outcome_int ? TAKEN : NOTTAKEN);
        uint8_t pred   = make_prediction((uint32_t)pc);

        if (pred != actual) mispred++;
        train_predictor((uint32_t)pc, actual);

        if (verbose) {
            printf("pc=0x%lx actual=%d pred=%d %s\n",
                pc, outcome_int, pred,
                (pred == actual ? "OK" : "MISS"));
        }

        total++;
        line++;
    }

    fclose(fp);

    printf("\n=== Results ===\n");
    printf("Predictor: %d\n", bpType);
    printf("Branches : %llu\n", (unsigned long long)total);
    printf("Mispreds : %llu\n", (unsigned long long)mispred);
    printf("Accuracy: %.4f%%\n", total ? (100.0 * (total - mispred) / total) : 0.0);

    cleanup_predictor();
    return 0;
}

