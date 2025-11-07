//========================================================//
//  main.c - Driver for Gshare Branch Predictor            //
//========================================================//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "predictor.h"

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <input_trace_file>\n", argv[0]);
        return 1;
    }

    FILE *fp = fopen(argv[1], "r");
    if (!fp) {
        perror("Error opening input file");
        return 1;
    }

    char outcome;
    unsigned int pc;
    unsigned long total = 0, correct = 0;

    // Initialize Gshare predictor
    init_gshare_predictor();

    printf("Running Gshare Branch Predictor...\n\n");

    // Example trace format: "0x00400000 T" or "0x00400004 N"
    while (fscanf(fp, "%x %c", &pc, &outcome) == 2) {
        int prediction = gshare_make_prediction(pc);
        int actual = (outcome == 'T') ? 1 : 0;

        if ((prediction && actual) || (!prediction && !actual))
            correct++;

        gshare_train_predictor(pc, actual);
        total++;
    }

    fclose(fp);

    // Final statistics
    double accuracy = (total > 0) ? ((double) correct / total) * 100.0 : 0.0;

    printf("------------------------------------\n");
    printf("Total Branches:     %lu\n", total);
    printf("Correct Predictions:%lu\n", correct);
    printf("Prediction Accuracy: %.2f%%\n", accuracy);
    printf("------------------------------------\n");

    cleanup_gshare_predictor();

    return 0;
}

