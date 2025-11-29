#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>

#include "predictor.h"   // your predictor header

// External configuration variables from predictor.c
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
        "  --verbose                Print detailed logs\n\n"
        "If no predictor is specified, all predictors will be run.\n",
        prog
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
    verbose = 0;
    int user_verbose = 0;  // Track if user requested verbose

    // Initialize defaults
    int user_bpType = -1;  // -1 means "run all predictors"
    
    // Set default parameters
    ghistoryBits = 12;          // GShare default
    perceptronIndexBits = 10;   // Perceptron defaults
    perceptronHistoryLen = 32;
    globalBits = 12;            // Tournament defaults
    phtIndexBits = 10;
    phtBits = 4;
    tournamentBits = 10;
    twobitIndexBits = 10;       // 2-bit default

    // Parse command-line arguments
    for (int i = 1; i < argc; i++) {
        const char *a = argv[i];

        if (a[0] != '-' && tracefile == NULL) {
            tracefile = a;
            continue;
        }

        int temp_bits;
        if (strcmp(a, "--perceptron") == 0) {
            user_bpType = STATIC;
        }
        else if (strcmp(a, "--tournament") == 0) {
            user_bpType = TOURNAMENT;
        }
        else if (strcmp(a, "--custom") == 0) {
            user_bpType = CUSTOM;
        }
        else if (strcmp(a, "--2bit") == 0) {
            user_bpType = TWOBIT;
        }
        else if (parse_colon(a, "--gshare", &temp_bits)) {
            user_bpType = GSHARE;
            ghistoryBits = temp_bits;
        }
        else if (parse_colon(a, "--percep-index", (int *)&perceptronIndexBits)) {}
        else if (parse_colon(a, "--percep-history", (int *)&perceptronHistoryLen)) {}
        else if (parse_colon(a, "--globalbits", &globalBits)) {}
        else if (parse_colon(a, "--pht-index", &phtIndexBits)) {}
        else if (parse_colon(a, "--pht-bits", &phtBits)) {}
        else if (parse_colon(a, "--tournament-bits", &tournamentBits)) {}
        else if (parse_colon(a, "--twobit-index", &twobitIndexBits)) {}
        else if (strcmp(a, "--verbose") == 0) {
            user_verbose = 1;
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

    // Predictor types and names
    int predictor_types[] = { GSHARE, STATIC, TOURNAMENT, CUSTOM, TWOBIT };
    const char *predictor_names[] = { "GShare", "Perceptron", "Tournament", "Custom", "2-bit" };
    int num_predictors = 5;

    // Determine which predictors to run
    int start = 0, end = num_predictors;
    if (user_bpType != -1) {
        // User specified a predictor - run only that one
        for (int i = 0; i < num_predictors; i++) {
            if (predictor_types[i] == user_bpType) {
                start = i;
                end = i + 1;
                break;
            }
        }
        // Enable verbose only when running a single predictor
        verbose = user_verbose;
    } else {
        // No predictor specified - print header
        printf("No specific predictor selected. Running all predictors...\n");
        if (user_verbose) {
            printf("Note: Verbose output disabled when running all predictors.\n");
            printf("      Use a specific predictor flag (e.g., --gshare:12) with --verbose.\n");
        }
        verbose = 0;  // Disable verbose for "all predictors" mode
        printf("\n");
    }

    // Loop over selected predictors
    for (int idx = start; idx < end; idx++) {
        bpType = predictor_types[idx];

        cleanup_predictor();   // ensure previous predictor state cleared
        init_predictor();      // initialize current predictor

        fseek(fp, 0, SEEK_SET); // rewind trace file

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
                       pc, outcome_int, pred, (pred == actual ? "OK" : "MISS"));
            }
            total++;
        }

        printf("\n=== Results for %s ===\n", predictor_names[idx]);
        printf("Branches : %llu\n", (unsigned long long)total);
        printf("Mispreds : %llu\n", (unsigned long long)mispred);
        if (total > 0) {
            printf("Accuracy : %.4f%%\n", 100.0 * (total - mispred) / total);
        } else {
            printf("Accuracy : N/A (no branches)\n");
        }
    }

    cleanup_predictor();
    fclose(fp);
    return 0;
}
