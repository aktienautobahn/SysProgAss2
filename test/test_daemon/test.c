#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>

#include "../include/daemon.h"

int check_files(const char *file1, const char *file2) {
    FILE *fp1 = fopen(file1, "r");
    if (fp1 == NULL) {
        fprintf(stderr, "Cannot open file with name %s\n", file1);
        return 1;
    }
    FILE *fp2 = fopen(file2, "r");
    if (fp2 == NULL) {
        fprintf(stderr, "Cannot open file with name %s\n", file2);
        return 1;
    }

    int c1, c2;
    while ((c1 = fgetc(fp1)) != EOF) {
        c2 = fgetc(fp2);
        if (c1 != c2) {
            fclose(fp1);
            fclose(fp2);
            return 1;
        }
    }

    fclose(fp1);
    fclose(fp2);
    return 0;
}


int main(int argc, char *argv[]) {
    /* Default values for filenames and output numbers */
    char *default_file1 = "test/test_daemon/rndtxt1.txt";
    char *default_file2 = "test/test_daemon/rndtxt2.txt";
    char *default_file3 = "test/test_daemon/rndtxt3.txt";
    char *default_compare1 = "test/test_daemon/rndtxt1_lsg.txt";
    char *default_compare2 = "test/test_daemon/rndtxt2_lsg.txt";
    char *default_compare3 = "test/test_daemon/rndtxt3_lsg.txt";
    char *default_output1_str = "11";
    char *default_output2_str = "12";
    char *default_output3_str = "13";

    /* Variable initialization for filenames */
    char *file1 = default_file1;
    char *file2 = default_file2;
    char *file3 = default_file3;
    char *compare1 = default_compare1;
    char *compare2 = default_compare2;
    char *compare3 = default_compare3;
    char *output1_str = default_output1_str;
    char *output2_str = default_output2_str;
    char *output3_str = default_output3_str;

    if (argc >= 10) {
        file1 = argv[1];
        file2 = argv[2];
        file3 = argv[3];
        compare1 = argv[4];
        compare2 = argv[5];
        compare3 = argv[6];
        output1_str = argv[7];
        output2_str = argv[8];
        output3_str = argv[9];
         /* Print all elements */
        // printf("file1=%s, file2=%s, file3=%s, compare1=%s, compare2=%s, compare3=%s, output1=%s, output2=%s, output3=%s\n",
        // file1, file2, file3, compare1, compare2, compare3, output1_str, output2_str, output3_str);

    } else {
        printf("Default values are inserted because the amount of passed elements is smaller than 9.\n");
    }

     /* Convert output arguments to size_t with error checking */
    char *endptr;
    errno = 0; /* To distinguish success/failure after call */
    size_t output1_num = strtoull(output1_str, &endptr, 10);
    if (errno != 0 || *endptr != '\0' || output1_num < MINIMUM_PORT || output1_num > MAXIMUM_PORT) {
        fprintf(stderr, "Error: Invalid port number for output1: %s\n", output1_str);
        return 1;
    }

    errno = 0;
    size_t output2_num = strtoull(output2_str, &endptr, 10);
    if (errno != 0 || *endptr != '\0' || output2_num < MINIMUM_PORT || output2_num > MAXIMUM_PORT) {
        fprintf(stderr, "Error: Invalid port number for output2: %s\n", output2_str);
        return 1;
    }

    errno = 0;
    size_t output3_num = strtoull(output3_str, &endptr, 10);
    if (errno != 0 || *endptr != '\0' || output3_num < MINIMUM_PORT || output3_num > MAXIMUM_PORT) {
        fprintf(stderr, "Error: Invalid port number for output3: %s\n", output3_str);
        return 1;
    }

    connection_t connection[3] = {
        {.from = 1, .to = (size_t)output1_num, .filename = file1},
        {.from = 2, .to = (size_t)output2_num, .filename = file2},
        {.from = 3, .to = (size_t)output3_num, .filename = file3}
    };
    char output1[21];
    char output2[21];
    char output3[21];
    snprintf(output1, sizeof(output1), "%zu.txt", output1_num);
    snprintf(output2, sizeof(output2), "%zu.txt", output2_num);
    snprintf(output3, sizeof(output3), "%zu.txt", output3_num);
    /* delete specified files if they exist (from previous runs) */
    remove(output1);
    remove(output2);
    remove(output3);

    

    /* execute daemon */
    printf("Executing daemon! If it does not terminate (in 10s), it is likely stuck\n");
    printf("You may need to kill it manually (CTRL+C) and compare the files with 'diff'\n");
    printf("If the files are the same, you are nearly done, and have to worry only about pthread_cancel logic!\n\n");
    simpledaemon((connection_t *)connection, 3);

    /* check if correct files were created */
    printf("Checking results\n");
    FILE *fp1 = fopen(output1, "r");
    if (fp1 == NULL) {
        fprintf(stderr, "Error: There should be a file with name %s\n", output1);
        return 1;
    }
    fclose(fp1);

    FILE *fp2 = fopen(output2, "r");
    if (fp2 == NULL) {
        fprintf(stderr, "Error: There should be a file with name %s\n", output2);
        return 1;
    }
    fclose(fp2);

    FILE *fp3 = fopen(output3, "r");
    if (fp3 == NULL) {
        fprintf(stderr, "Error: There should be a file with name %s\n", output3);
        return 1;
    }
    fclose(fp3);

    /* check if the files have the correct content */
    if (check_files(compare1, output1) != 0) {
        fprintf(stderr, "Error: files %s and %s are not the same\n", output1, compare1);
        return 1;
    }

    if (check_files(compare2, output2) != 0) {
        fprintf(stderr, "Error: files %s and %s are not the same\n", output2, compare2);
        return 1;
    }

    if (check_files(compare3, output3) != 0) {
        fprintf(stderr, "Error: files %s and %s are not the same\n", output3, compare3);
        return 1;
    }

    printf("Test passed!\n");

    return 0;
}
