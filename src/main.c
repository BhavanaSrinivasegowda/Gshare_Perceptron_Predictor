#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include "predictor.h"

int main(int argc, char **argv)
{
    if (argc < 2) {
        printf("Usage: %s <input_file>\n", argv[0]);
        return 1;
    }

    const char *input_file = argv[1];

    for (int type = 0; type <= 4; ++type) {
        bpType = type;

        FILE *fp = fopen(input_file, "r");
        if (!fp) {
            perror("fopen");
            return 1;
        }

        init_predictor();

        uint64_t total = 0;
        uint64_t correct = 0;
        uint64_t pc;
        int outcome_bit;

        // Accepts lines like:
        // x40fc96 1
        // 0x40fc96 0
        // 40fc96 1
        while (fscanf(fp, "%lx %d", &pc, &outcome_bit) == 2) {

            uint8_t outcome = (outcome_bit == 1) ? TAKEN : NOTTAKEN;
            uint8_t pred = make_prediction((uint32_t)pc);

            if (pred == outcome)
                correct++;

            total++;

            train_predictor((uint32_t)pc, outcome);
        }

        fclose(fp);
        cleanup_predictor();

        printf("\nPredictor Type: %s\n", bpName[bpType]);
        printf("Total branches: %" PRIu64 "\n", total);
        printf("Correct predictions: %" PRIu64 "\n", correct);
        printf("Accuracy: %.2f%%\n", 100.0 * correct / total);
    }

    return 0;
}

