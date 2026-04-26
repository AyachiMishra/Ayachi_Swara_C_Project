#include <stdio.h>
#include <stdlib.h>
#include "sheet.h"
#include <string.h>
#include <time.h>

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: ./sheet R C\n");
        return 1;
    }

    int rows = atoi(argv[1]);
    int cols = atoi(argv[2]);

    if (rows < 1 || rows > 999 || cols < 1 || cols > 26*26*26+26*26+26) {
        fprintf(stderr, "Invalid dimensions\n");
        return 1;
    }

    Sheet *sheet = createSheet(rows, cols);
    if (sheet == NULL) {
        fprintf(stderr, "Failed to create sheet\n");
        return 1;
    }

    char input[256];
    double elapsed = 0.0;
    const char *status = "ok";

printSheet(sheet); 

while (1) {
    printf("[%.1f] (%s) > ", elapsed, status);


    if (fgets(input, sizeof(input), stdin) == NULL) break;
    int len = strlen(input);
    if (len > 0 && input[len-1] == '\n') input[len-1] = '\0';

    clock_t start=clock();

    if (strcmp(input, "q") == 0) {
        break;
    } else if (strcmp(input, "w") == 0) {
        sheet->view_row -= 10;
        if (sheet->view_row < 0) sheet->view_row = 0;
        status = "ok";
    } else if (strcmp(input, "s") == 0) {
        sheet->view_row += 10;
        if (sheet->view_row >= sheet->rows) sheet->view_row = sheet->rows - 1;
        status = "ok";
    } else if (strcmp(input, "a") == 0) {
        sheet->view_col -= 10;
        if (sheet->view_col < 0) sheet->view_col = 0;
        status = "ok";
    } else if (strcmp(input, "d") == 0) {
        sheet->view_col += 10;
        if (sheet->view_col >= sheet->cols) sheet->view_col = sheet->cols - 1;
        status = "ok";
    } else if (strcmp(input, "disable_output") == 0) {
        sheet->output_enabled = 0;
        status = "ok";
    } else if (strcmp(input, "enable_output") == 0) {
        sheet->output_enabled = 1;
        status = "ok";
    } else if (strncmp(input, "scroll_to ", 10) == 0) {
        char *cell = input + 10;
        int row = cellNameToRow(cell);
        int col = cellNameToCol(cell);
        if (row < 0 || col < 0 || row >= sheet->rows || col >= sheet->cols) {
            status = "invalid cell";
        } else {
            sheet->view_row = row;
            sheet->view_col = col;
            status = "ok";
        }
    } else if (isFormula(input)) {
        parseAndSetCell(sheet,input,&status);
    } else {
        status = "unrecognized cmd";
    }

    // Only print if output is enabled
    if (sheet->output_enabled) {
        printSheet(sheet);
    }

    clock_t end=clock() ;
    elapsed=(double)(end-start)/CLOCKS_PER_SEC;
}

    freeSheet(sheet);
    return 0;

}