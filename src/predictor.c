//========================================================//
//  predictor.c                                           //
//  Refactored Branch Predictor Implementation            //
//  Supports: Static(Perceptron), GShare, Tournament,     //
//            Custom (GShare + Perceptron chooser)        //
//========================================================//

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <string.h>
#include "predictor.h"

//------------------------------------//
//      Predictor Configuration       //
//------------------------------------//

const char *bpName[4] = { "Perceptron", "Gshare", "Tournament", "Custom" };

int ghistoryBits   = 12;   // global history length for gshare/tournament chooser
int globalBits     = 12;   // global-only BHT size (2^globalBits entries)
int tournamentBits = 12;   // chooser table bits
int phtIndexBits   = 9;    // local-index table size bits
int phtBits        = 9;    // local history length
int bpType;                // selected predictor type
int verbose;               // verbose flag (unused here)

#define GHR_MASK(bits) (((1ULL << (bits)) - 1ULL))

//------------------------------------//
//      Predictor Data Structures     //
//------------------------------------//

static uint8_t *gshare_bht = NULL;
static uint64_t ghr = 0;

static uint8_t *global_bht = NULL;

static uint8_t *local_history_table = NULL;
static uint8_t *local_bht = NULL;

static uint8_t *chooser_table = NULL;

static uint8_t perceptronIndexBits = 7;
static uint8_t perceptronHistoryLen = 22;
static int8_t *perceptron_table = NULL;
static uint32_t percep_ghr = 0;
static int percep_num_entries = 0;
static int percep_num_weights = 0;
static int percep_theta = 0;
static int8_t percep_weight_max = 127;
static int8_t percep_weight_min = -127;

//------------------------------------//
//          Utility functions         //
//------------------------------------//

static inline uint32_t mask_bits_u32(int bits) {
    if (bits >= 32) return 0xFFFFFFFFu;
    return ((1u << bits) - 1u);
}

static inline uint64_t mask_bits_u64(int bits) {
    if (bits >= 64) return 0xFFFFFFFFFFFFFFFFULL;
    return ((1ULL << bits) - 1ULL);
}

static inline int8_t clamp_weight(int32_t w) {
    if (w > percep_weight_max) return percep_weight_max;
    if (w < percep_weight_min) return percep_weight_min;
    return (int8_t)w;
}

//------------------------------------//
//             GShare code            //
//------------------------------------//

static void init_gshare(void) {
    int entries = 1 << ghistoryBits;
    gshare_bht = (uint8_t *)malloc(entries * sizeof(uint8_t));
    if (!gshare_bht) { fprintf(stderr, "malloc gshare_bht failed\n"); exit(1); }
    for (int i = 0; i < entries; ++i) gshare_bht[i] = WN;
    ghr = 0;
}

static uint8_t gshare_predict(uint32_t pc) {
    uint32_t mask = (1u << ghistoryBits) - 1u;
    uint32_t idx = (pc & mask) ^ (uint32_t)(ghr & mask);
    uint8_t ctr = gshare_bht[idx];
    return (ctr == WT || ctr == ST) ? TAKEN : NOTTAKEN;
}

static void gshare_train(uint32_t pc, uint8_t outcome) {
    uint32_t mask = (1u << ghistoryBits) - 1u;
    uint32_t idx = (pc & mask) ^ (uint32_t)(ghr & mask);
    uint8_t state = gshare_bht[idx];

    if (outcome == TAKEN) {
        if (state < ST) gshare_bht[idx] = state + 1;
    } else {
        if (state > SN) gshare_bht[idx] = state - 1;
    }

    ghr = ((ghr << 1) | (uint64_t)(outcome & 1)) & mask_bits_u64(ghistoryBits);
}

//------------------------------------//
//             Global-only            //
//------------------------------------//

static void init_global(void) {
    int entries = 1UL << globalBits;
    global_bht = (uint8_t *)malloc(entries * sizeof(uint8_t));
    if (!global_bht) { fprintf(stderr, "malloc global_bht failed\n"); exit(1); }
    for (int i = 0; i < entries; ++i) global_bht[i] = WN;
    ghr = 0;
}

static uint8_t global_predict(uint32_t pc) {
    uint32_t idx = (uint32_t)(ghr & ((1u << globalBits) - 1u));
    uint8_t ctr = global_bht[idx];
    return (ctr == WT || ctr == ST) ? TAKEN : NOTTAKEN;
}

static void global_train(uint32_t pc, uint8_t outcome) {
    uint32_t idx = (uint32_t)(ghr & ((1u << globalBits) - 1u));
    uint8_t state = global_bht[idx];
    if (outcome == TAKEN) {
        if (state < ST) global_bht[idx] = state + 1;
    } else {
        if (state > SN) global_bht[idx] = state - 1;
    }
    ghr = ((ghr << 1) | (uint64_t)(outcome & 1)) & mask_bits_u64(ghistoryBits);
}

//------------------------------------//
//               PHT (local)          //
//------------------------------------//

static void init_pht(void) {
    int pht_entries = 1 << phtIndexBits;
    int bht_entries = 1 << phtBits;

    local_history_table = (uint8_t *)malloc(pht_entries * sizeof(uint8_t));
    if (!local_history_table) { fprintf(stderr, "malloc local_history_table failed\n"); exit(1); }
    local_bht = (uint8_t *)malloc(bht_entries * sizeof(uint8_t));
    if (!local_bht) { fprintf(stderr, "malloc local_bht failed\n"); exit(1); }

    for (int i = 0; i < pht_entries; ++i) local_history_table[i] = 0;
    for (int i = 0; i < bht_entries; ++i) local_bht[i] = N0;
}

static uint8_t pht_predict(uint32_t pc) {
    uint32_t pc_idx = pc & ((1u << phtIndexBits) - 1u);
    uint32_t local_idx = local_history_table[pc_idx];
    uint8_t state = local_bht[local_idx];
    switch (state) {
        case N2: case N1: case N0: return NOTTAKEN;
        case T2: case T1: case T0: return TAKEN;
        default: return NOTTAKEN;
    }
}

static void pht_train(uint32_t pc, uint8_t outcome) {
    uint32_t pc_idx = pc & ((1u << phtIndexBits) - 1u);
    uint32_t local_idx = local_history_table[pc_idx];
    uint8_t state = local_bht[local_idx];

    switch (state) {
        case N2: local_bht[local_idx] = (outcome==TAKEN)?N1:N2; break;
        case N1: local_bht[local_idx] = (outcome==TAKEN)?N0:N2; break;
        case N0: local_bht[local_idx] = (outcome==TAKEN)?T0:N1; break;
        case T0: local_bht[local_idx] = (outcome==TAKEN)?T1:N0; break;
        case T1: local_bht[local_idx] = (outcome==TAKEN)?T2:T0; break;
        case T2: local_bht[local_idx] = (outcome==TAKEN)?T2:T1; break;
        default: break;
    }

    local_history_table[pc_idx] = ((local_history_table[pc_idx] << 1) | (outcome & 1)) & ((1u << phtBits) - 1u);
}

//------------------------------------//
//           Perceptron code          //
//------------------------------------//

static void init_perceptron(void) {
    percep_num_entries = 1 << perceptronIndexBits;
    percep_num_weights = perceptronHistoryLen + 1; 

    perceptron_table = (int8_t *)malloc(percep_num_entries * percep_num_weights * sizeof(int8_t));
    if (!perceptron_table) { fprintf(stderr, "malloc perceptron_table failed\n"); exit(1); }
    memset(perceptron_table, 0, percep_num_entries * percep_num_weights * sizeof(int8_t));

    percep_ghr = 0;
    percep_theta = (int)floor(1.93 * perceptronHistoryLen + 14.0);
}

// Dot product for perceptron
static int32_t perceptron_dotprod(int idx) {
    int32_t sum = perceptron_table[idx * percep_num_weights];
    uint32_t h = percep_ghr;
    for (int i = 0; i < perceptronHistoryLen; ++i) {
        int8_t bit = ((h >> i) & 1u) ? +1 : -1;
        sum += perceptron_table[idx * percep_num_weights + i + 1] * bit;
    }
    return sum;
}

static uint8_t perceptron_predict(uint32_t pc) {
    int idx = pc & ((1u << perceptronIndexBits) - 1u);
    int32_t y = perceptron_dotprod(idx);
    return (y >= 0) ? TAKEN : NOTTAKEN;
}

static void perceptron_train(uint32_t pc, uint8_t outcome) {
    int idx = pc & ((1u << perceptronIndexBits) - 1u);
    int32_t y = perceptron_dotprod(idx);
    int t = (outcome == TAKEN) ? +1 : -1;
    int abs_y = (y < 0) ? -y : y;

    if (((y >= 0) != (t > 0)) || (abs_y <= percep_theta)) {
        int8_t *base = &perceptron_table[idx * percep_num_weights];
        base[0] = clamp_weight(base[0] + t);

        uint32_t h = percep_ghr;
        for (int i = 0; i < perceptronHistoryLen; ++i) {
            int8_t history_bit = ((h >> i) & 1u) ? +1 : -1;
            base[i + 1] = clamp_weight(base[i + 1] + t * history_bit);
        }
    }

    percep_ghr = ((percep_ghr << 1) | (uint32_t)(outcome & 1u)) & ((1u << perceptronHistoryLen) - 1u);
}

//------------------------------------//
//          Custom / Tournament       //
//------------------------------------//

static void init_custom(void) {
    if (!gshare_bht) init_gshare();
    if (!perceptron_table) init_perceptron();

    int chooser_entries = 1 << tournamentBits;
    chooser_table = (uint8_t *)malloc(chooser_entries * sizeof(uint8_t));
    if (!chooser_table) { fprintf(stderr, "malloc chooser_table failed\n"); exit(1); }

    for (int i = 0; i < chooser_entries; ++i) chooser_table[i] = WN;
}

static uint8_t tournament_predict(uint32_t pc) {
    uint8_t g_pred = global_predict(pc);
    uint8_t l_pred = pht_predict(pc);
    uint32_t chooser_idx = (uint32_t)(ghr & ((1u << tournamentBits) - 1u));
    uint8_t choice = chooser_table ? chooser_table[chooser_idx] : WN;
    return (choice == SN || choice == WN) ? g_pred : l_pred;
}

static void train_tournament(uint32_t pc, uint8_t outcome) {
    uint8_t g_pred = global_predict(pc);
    uint8_t l_pred = pht_predict(pc);
    global_train(pc, outcome);
    pht_train(pc, outcome);

    if (g_pred != l_pred) {
        uint32_t chooser_idx = (uint32_t)(ghr & ((1u << tournamentBits) - 1u));
        uint8_t choice = chooser_table[chooser_idx];
        if (g_pred == outcome && l_pred != outcome) {
            if (choice > SN) chooser_table[chooser_idx] = choice - 1;
        } else if (l_pred == outcome && g_pred != outcome) {
            if (choice < ST) chooser_table[chooser_idx] = choice + 1;
        }
    }
}

static uint8_t custom_predict(uint32_t pc) {
    uint8_t g_pred = gshare_predict(pc);
    uint8_t p_pred = perceptron_predict(pc);
    uint32_t chooser_idx = (uint32_t)(ghr & ((1u << tournamentBits) - 1u));
    uint8_t choice = chooser_table ? chooser_table[chooser_idx] : WN;
    return (choice == SN || choice == WN) ? g_pred : p_pred;
}

static void train_custom(uint32_t pc, uint8_t outcome) {
    uint8_t g_pred = gshare_predict(pc);
    uint8_t p_pred = perceptron_predict(pc);

    gshare_train(pc, outcome);
    perceptron_train(pc, outcome);

    if (g_pred != p_pred) {
        uint32_t chooser_idx = (uint32_t)(ghr & ((1u << tournamentBits) - 1u));
        uint8_t choice = chooser_table[chooser_idx];
        if (g_pred == outcome && p_pred != outcome) {
            if (choice > SN) chooser_table[chooser_idx] = choice - 1;
        } else if (p_pred == outcome && g_pred != outcome) {
            if (choice < ST) chooser_table[chooser_idx] = choice + 1;
        }
    }
}

//------------------------------------//
//             Init / Main API        //
//------------------------------------//

void init_predictor(void) {
    printf("Initializing Predictor...\n");

    switch (bpType) {
        case GSHARE: init_gshare(); break;
        case STATIC: init_perceptron(); break;
        case TOURNAMENT:
            init_global();
            init_pht();
            {
                int chooser_entries = 1 << tournamentBits;
                chooser_table = (uint8_t *)malloc(chooser_entries * sizeof(uint8_t));
                if (!chooser_table) { fprintf(stderr, "malloc chooser_table failed\n"); exit(1); }
                for (int i = 0; i < chooser_entries; ++i) chooser_table[i] = WN;
            }
            break;
        case CUSTOM:
            init_gshare();
            init_perceptron();
            init_custom();
            break;
        default:
            init_gshare();
            break;
    }
}

uint8_t make_prediction(uint32_t pc) {
    switch (bpType) {
        case GSHARE: return gshare_predict(pc);
        case STATIC: return perceptron_predict(pc);
        case TOURNAMENT: return tournament_predict(pc);
        case CUSTOM: return custom_predict(pc);
        default: return NOTTAKEN;
    }
}

void train_predictor(uint32_t pc, uint8_t outcome) {
    switch (bpType) {
        case GSHARE: gshare_train(pc, outcome); break;
        case STATIC: perceptron_train(pc, outcome); break;
        case TOURNAMENT: train_tournament(pc, outcome); break;
        case CUSTOM: train_custom(pc, outcome); break;
        default: break;
    }
}

//------------------------------------//
//              Cleanup               //
//------------------------------------//

void cleanup_predictor(void) {
    if (gshare_bht) { free(gshare_bht); gshare_bht = NULL; }
    if (global_bht) { free(global_bht); global_bht = NULL; }
    if (local_history_table) { free(local_history_table); local_history_table = NULL; }
    if (local_bht) { free(local_bht); local_bht = NULL; }
    if (chooser_table) { free(chooser_table); chooser_table = NULL; }
    if (perceptron_table) { free(perceptron_table); perceptron_table = NULL; }
    ghr = 0;
    percep_ghr = 0;
}

