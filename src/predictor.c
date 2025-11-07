//========================================================//
//  predictor.c                                           //
//  GShare Branch Predictor Implementation                //
//========================================================//

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "predictor.h"

//------------------------------------//
//      Predictor Configuration       //
//------------------------------------//

const char *bpName = "GShare";

int ghistoryBits = 12;   // Number of bits for global history
uint64_t gshareHistory;  // Global history register
uint8_t *gshareTable;    // Branch History Table (BHT)
int bpType;              // Prediction type (unused, fixed to GShare)
int verbose;             // Verbose mode flag

//------------------------------------//
//      GShare Predictor Setup        //
//------------------------------------//

void init_gshare_predictor() {
  int num_entries = 1 << ghistoryBits;
  gshareTable = (uint8_t *)malloc(num_entries * sizeof(uint8_t));
  if (!gshareTable) {
    fprintf(stderr, "Error: Memory allocation failed for GShare BHT.\n");
    exit(1);
  }

  // Initialize all entries to Weakly Not Taken (WN)
  for (int i = 0; i < num_entries; i++) {
    gshareTable[i] = WN;
  }

  gshareHistory = 0;
  printf("GShare initialized with %d entries.\n", num_entries);
}

//------------------------------------//
//         GShare Prediction          //
//------------------------------------//

uint8_t gshare_make_prediction(uint32_t pc) {
  uint32_t mask = (1 << ghistoryBits) - 1;
  uint32_t pc_index = pc & mask;
  uint32_t history_index = gshareHistory & mask;
  uint32_t index = pc_index ^ history_index;

  uint8_t counter = gshareTable[index];
  return (counter == WT || counter == ST) ? TAKEN : NOTTAKEN;
}

//------------------------------------//
//        GShare Training Logic       //
//------------------------------------//

void gshare_train_predictor(uint32_t pc, uint8_t outcome) {
  uint32_t mask = (1 << ghistoryBits) - 1;
  uint32_t pc_index = pc & mask;
  uint32_t history_index = gshareHistory & mask;
  uint32_t index = pc_index ^ history_index;

  uint8_t state = gshareTable[index];
  // 2-bit saturating counter update
  if (outcome == TAKEN) {
    if (state < ST) gshareTable[index]++;
  } else {
    if (state > SN) gshareTable[index]--;
  }

  // Update global history register
  gshareHistory = ((gshareHistory << 1) | outcome) & mask;
}

//------------------------------------//
//        Cleanup Function            //
//------------------------------------//

void cleanup_gshare_predictor() {
  free(gshareTable);
}

