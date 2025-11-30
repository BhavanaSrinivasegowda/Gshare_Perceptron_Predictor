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

// ---- NEW: chooser stats from predictor.c ----
extern uint64_t chooser_use_gshare;
extern uint64_t chooser_use_percep;

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
    int user_verbose = 0;

    int user_bpType = -1;  // -1 => run all predictors

    // Default parameters
    ghistoryBits = 12;
    perceptronIndexBits = 10;
    perceptronHistoryLen = 32;
    globalBits = 12;
    phtIndexBits = 10;
    phtBits = 4;
    tournamentBits = 10;
    twobitIndexBits = 10;

    // Parse args
    for (int i = 1; i < argc; i++) {
        const char *a = argv[i];

        if (a[0] != '-' && tracefile == NULL) {
            tracefile = a;
            continue;
        }

        int tmp;
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
        else if (parse_colon(a, "--gshare", &tmp)) {
            user_bpType = GSHARE;
            ghistoryBits = tmp;
        }
        else if (parse_colon(a, "--percep-index", (int*)&perceptronIndexBits)) {}
        else if (parse_colon(a, "--percep-history", (int*)&perceptronHistoryLen)) {}
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

    int predictor_types[] = { GSHARE, STATIC, TOURNAMENT, CUSTOM, TWOBIT };
    const char *predictor_names[] = { "GShare", "Perceptron", "Tournament", "Custom", "2-bit" };
    int num_predictors = 5;

    int start = 0, end = num_predictors;

    if (user_bpType != -1) {
        for (int i = 0; i < num_predictors; i++) {
            if (predictor_types[i] == user_bpType) {
                start = i;
                end = i + 1;
                break;
            }
        }
        verbose = user_verbose;
    } 
    else {
        printf("No specific predictor selected. Running all predictors...\n\n");
        verbose = 0;
    }

    // ------------------ LOOP OVER PREDICTORS ------------------
    for (int idx = start; idx < end; idx++) {
        bpType = predictor_types[idx];

        // Reset chooser stats
        chooser_use_gshare = 0;
        chooser_use_percep = 0;

        cleanup_predictor();
        init_predictor();

        fseek(fp, 0, SEEK_SET);

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
        }

        printf("\n=== Results for %s ===\n", predictor_names[idx]);
        printf("Branches : %llu\n", (unsigned long long)total);
        printf("Mispreds : %llu\n", (unsigned long long)mispred);
        printf("Accuracy : %.4f%%\n", 100.0 * (total - mispred) / total);
        printf("MPKI     : %.6f\n", (1000.0 * mispred) / total);

        // ---- CUSTOM predictor chooser statistics ----
        if (bpType == CUSTOM) {
            double p_g = (100.0 * chooser_use_gshare) / (double)total;
            double p_p = (100.0 * chooser_use_percep) / (double)total;

            printf("Chooser picked GShare     : %.2f%% (%llu times)\n",
                   p_g, (unsigned long long)chooser_use_gshare);
            printf("Chooser picked Perceptron : %.2f%% (%llu times)\n",
                   p_p, (unsigned long long)chooser_use_percep);
        }

        // ---- Hardware cost section ----
        printf("\nHardware Cost (bits):\n");

        if (bpType == GSHARE) {
            uint64_t pht_entries = (1ULL << ghistoryBits);
            uint64_t cost = ghistoryBits + 2 * pht_entries;
            printf("  GHR bits      : %d\n", ghistoryBits);
            printf("  PHT entries   : %llu\n", (unsigned long long)pht_entries);
            printf("  PHT cost      : %llu bits\n", (unsigned long long)(2 * pht_entries));
            printf("  TOTAL hardware: %llu bits\n", (unsigned long long)cost);
        }

        if (bpType == STATIC) {
            uint64_t num_entries  = (1ULL << perceptronIndexBits);
            uint64_t num_weights  = perceptronHistoryLen + 1;
            uint64_t cost_bits    = num_entries * num_weights * 8;
            printf("  Perceptrons   : %llu\n", (unsigned long long)num_entries);
            printf("  Weights/entry : %llu\n", (unsigned long long)num_weights);
            printf("  Total bits    : %llu bits\n", (unsigned long long)cost_bits);
        }

        if (bpType == TOURNAMENT) {
            uint64_t chooser_entries = (1ULL << tournamentBits);
            uint64_t chooser_bits = chooser_entries * 2;

            uint64_t lht_entries = (1ULL << phtIndexBits);
            uint64_t lht_bits = lht_entries * phtBits;

            uint64_t lpht_entries = (1ULL << phtBits);
            uint64_t lpht_bits = lpht_entries * 2;

            printf("  Chooser bits  : %llu\n", (unsigned long long)chooser_bits);
            printf("  LHT bits      : %llu\n", (unsigned long long)lht_bits);
            printf("  Local PHT bits: %llu\n", (unsigned long long)lpht_bits);
            printf("  TOTAL hardware: %llu bits\n",
                  (unsigned long long)(chooser_bits + lht_bits + lpht_bits));
        }

        if (bpType == CUSTOM) {
            uint64_t gshare_pht_entries = (1ULL << ghistoryBits);
            uint64_t gshare_cost = ghistoryBits + (2 * gshare_pht_entries);

            uint64_t num_entries  = (1ULL << perceptronIndexBits);
            uint64_t num_weights  = perceptronHistoryLen + 1;
            uint64_t percep_cost  = num_entries * num_weights * 8;

            uint64_t chooser_entries = (1ULL << tournamentBits);
            uint64_t chooser_bits = chooser_entries * 2;

            uint64_t total_cost = gshare_cost + percep_cost + chooser_bits;

            printf("  GShare cost   : %llu bits\n", (unsigned long long)gshare_cost);
            printf("  Perceptron    : %llu bits\n", (unsigned long long)percep_cost);
            printf("  Chooser table : %llu bits\n", (unsigned long long)chooser_bits);
            printf("  TOTAL hardware: %llu bits\n", (unsigned long long)total_cost);
        }

        if (bpType == TWOBIT) {
            uint64_t entries = (1ULL << twobitIndexBits);
            uint64_t cost = entries * 2;
            printf("  Entries       : %llu\n", (unsigned long long)entries);
            printf("  TOTAL hardware: %llu bits\n", (unsigned long long)cost);
        }

    } // end predictor loop

    cleanup_predictor();
    fclose(fp);
    return 0;
}

