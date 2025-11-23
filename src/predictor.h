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

/* Prediction outcomes */
#define TAKEN       1
#define NOTTAKEN    0

/* Two-bit counter states (used by BHT/chooser) */
#define SN 0  /* Strongly Not Taken */
#define WN 1  /* Weakly Not Taken */
#define WT 2  /* Weakly Taken */
#define ST 3  /* Strongly Taken */

/* Local predictor multi-bit states (example for 3-bit-like) */
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

/* Predictor configuration */
extern int ghistoryBits;
extern int tournamentBits;
extern int phtIndexBits;
extern int phtBits;
extern int bpType;
extern int verbose;

extern const char *bpName[4];

#endif /* PREDICTOR_H */

