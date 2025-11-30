//========================================================//
//  predictor.h                                           //
//  Header file for branch predictor API                 //
//========================================================//

#ifndef PREDICTOR_H
#define PREDICTOR_H

#include <stdint.h>

/* Predictor types */
#define STATIC      0
#define GSHARE      1
#define TOURNAMENT  2
#define CUSTOM      3
#define TWOBIT      4

/* Prediction outcomes */
#define TAKEN       1
#define NOTTAKEN    0

/* Two-bit counter states */
#define SN 0
#define WN 1
#define WT 2
#define ST 3

/* Local predictor multi-bit states */
#define N2 0
#define N1 1
#define N0 2
#define T0 3
#define T1 4
#define T2 5

/* API functions */
void init_predictor(void);
uint8_t make_prediction(uint32_t pc);
void train_predictor(uint32_t pc, uint8_t outcome);
void cleanup_predictor(void);

/* Predictor configuration (from predictor.c) */
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

extern const char *bpName[5];

extern uint64_t chooser_use_gshare;
extern uint64_t chooser_use_percep;

#endif /* PREDICTOR_H */

