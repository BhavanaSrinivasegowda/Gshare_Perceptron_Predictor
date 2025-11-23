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

    for (int type = 0; type <= 3; ++type) {
        bpType = type;

        FILE *fp = fopen(input_file, "r");
        if (!fp) {
            perror("fopen");
            return 1;
        }

        init_predictor();

        uint64_t total = 0;
        uint64_t correct = 0;
        uint32_t pc;
        char outcome_char;

        while (fscanf(fp, "%x %c", &pc, &outcome_char) == 2) {
            uint8_t outcome = (outcome_char == 'T' || outcome_char == 't') ? TAKEN : NOTTAKEN;
            uint8_t pred = make_prediction(pc);

            if (pred == outcome) correct++;
            total++;

            train_predictor(pc, outcome);
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

