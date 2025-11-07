//========================================================//
//  predictor.h                                           //
//========================================================//

#ifndef PREDICTOR_H
#define PREDICTOR_H

#include <stdint.h>

// Prediction outcomes
#define TAKEN 1
#define NOTTAKEN 0

// 2-bit saturating counter states
#define SN 0  // Strongly Not Taken
#define WN 1  // Weakly Not Taken
#define WT 2  // Weakly Taken
#define ST 3  // Strongly Taken

// Function declarations
void init_gshare_predictor();
uint8_t gshare_make_prediction(uint32_t pc);
void gshare_train_predictor(uint32_t pc, uint8_t outcome);
void cleanup_gshare_predictor();

#endif

