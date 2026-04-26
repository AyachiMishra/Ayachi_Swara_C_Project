#include "sheet.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Sheet *createSheet(int rows, int cols) {
    Sheet *sheet = malloc(sizeof(Sheet));
    if (sheet == NULL) return NULL;

    sheet->rows = rows;
    sheet->cols = cols;

    sheet->cells = malloc(rows * sizeof(Cell *));
    if (sheet->cells == NULL) {
        free(sheet);
        return NULL;
    }

    for (int i = 0; i < rows; i++) {
        sheet->cells[i] = malloc(cols * sizeof(Cell));  
        if (sheet->cells[i] == NULL) {
            for (int k = 0; k < i; k++) free(sheet->cells[k]);
            free(sheet->cells);
            free(sheet);
            return NULL;
        }
        for (int j = 0; j < cols; j++) {
            sheet->cells[i][j].value = 0;
            sheet->cells[i][j].is_error = 0;    
            sheet->cells[i][j].formula = NULL;  
            sheet->cells[i][j].deps = NULL;     
            sheet->cells[i][j].dep_count = 0;   
        }
    }

    sheet->view_row=0;
    sheet->view_col=0;
    sheet->output_enabled=1;

    return sheet;
}

void freeSheet(Sheet *sheet) {
    if (sheet == NULL) return;  // fixed
    for (int i = 0; i < sheet->rows; i++) free(sheet->cells[i]);
    free(sheet->cells);
    free(sheet);
}

void indexToColName(int index, char *buf) {
    
    int len = 0;
    index++;
    while (index > 0) {
        index--;
        buf[len++] = 'A' + (index % 26);
        index /= 26;
    }
    
    for (int i = 0; i < len / 2; i++) {
        char tmp = buf[i];
        buf[i] = buf[len - 1 - i];
        buf[len - 1 - i] = tmp;
    }
    buf[len] = '\0';
}

#include <stdio.h>
#include <stdlib.h>
#include "sheet.h"

void printSheet(Sheet *sheet) {
    int max_rows = 10;
    int max_cols = 10;

    
    int end_row = sheet->view_row + max_rows;
    int end_col = sheet->view_col + max_cols;
    if (end_row > sheet->rows) end_row = sheet->rows;
    if (end_col > sheet->cols) end_col = sheet->cols;

    
    printf("   "); 
    for (int j = sheet->view_col; j < end_col; j++) {
        char colName[5];
        indexToColName(j, colName);
        printf("%6s", colName);
    }
    printf("\n");

    
    for (int i = sheet->view_row; i < end_row; i++) {
        printf("%3d", i + 1); // row number (1-indexed)
        for (int j = sheet->view_col; j < end_col; j++) {
            if (sheet->cells[i][j].is_error) {
                printf("%6s", "ERR");
            } else {
                printf("%6d", sheet->cells[i][j].value);
            }
        }
        printf("\n");
    }
}

int colNameToIndex(const char *col) {
    int index = 0;
    while (*col) {
        index = index * 26 + (*col - 'A' + 1);
        col++;
    }
    return index - 1;
}

int cellNameToCol(const char *cell) {
    char col[5] = {0};
    int i = 0;
    while (cell[i] && cell[i] >= 'A' && cell[i] <= 'Z') {
        col[i] = cell[i];
        i++;
    }
    return colNameToIndex(col);
}

int cellNameToRow(const char *cell) {
    while (*cell && *cell >= 'A' && *cell <= 'Z') cell++;
    return atoi(cell) - 1;
}

int isValidCellName(char *s) {
    if (!(s[0] >= 'A' && s[0] <= 'Z')) {
        return 0;
    }
    
    int i = 0;
    while (s[i] >= 'A' && s[i] <= 'Z') {
        i++;
    }
    
    if (i == 0) {
        return 0;
    }

    if (i > 3) return 0;        
    
    if (!(s[i] >= '0' && s[i] <= '9')) {
        return 0;
    }

    int rowStart=i;
    
    while (s[i] >= '0' && s[i] <= '9') {
        i++;
    }
    
    if (s[i] != '\0') {
        return 0;
    }

    int rowNum=atoi(&s[rowStart]);

    if (rowNum<1||rowNum>999) return 0;
    
    return 1;
}

int isFormula(char *s) {
    char *eq = strchr(s, '=');
    
    if (eq == NULL) return 0;
    
    int len = eq - s;
    if (len == 0) return 0;
    
    char cell_part[7];  
    strncpy(cell_part, s, len);
    cell_part[len] = '\0';  
    
    if (!isValidCellName(cell_part)) return 0;
    
    if (*(eq + 1) == '\0') return 0;
    
    return 1;
}

int evaluateValue(Sheet *sheet, char *v, int *error) {
    if (v[0] >= '0' && v[0] <= '9') return atoi(v);
    
    if (v[0] == '-' && v[1] >= '0' && v[1] <= '9') return atoi(v);
    
    if (v[0] >= 'A' && v[0] <= 'Z') {
        if (!isValidCellName(v)) {
            *error = 1;
            return 0;
        }
        
        int row = cellNameToRow(v);
        int col = cellNameToCol(v);
        
        if (sheet->cells[row][col].is_error) {
            *error = 1;
            return 0;
        }
        
        return sheet->cells[row][col].value;
    }
    
    *error = 1;
    return 0;
}

int evaluateExpression(Sheet *sheet, char *expr, int *error) {
    int len = strlen(expr);
    
    for (int i = 1; i < len; i++) {
        if (expr[i] == '+' || expr[i] == '-' || expr[i] == '*' || expr[i] == '/') {
            char leftStr[256];
            strncpy(leftStr, expr, i);
            leftStr[i] = '\0';
            
            char *rightStr = expr + i + 1;
            
            int leftVal = evaluateValue(sheet, leftStr, error);
            int rightVal = evaluateValue(sheet, rightStr, error);
            
            if (*error) return 0;
            
            if (expr[i] == '+') return leftVal + rightVal;
            if (expr[i] == '-') return leftVal - rightVal;
            if (expr[i] == '*') return leftVal * rightVal;
            if (expr[i] == '/') {
                if (rightVal == 0) {
                    *error = 1;
                    return 0;
                }
                return leftVal / rightVal;
            }
        }
    }
    
    return evaluateValue(sheet, expr, error);
}

void parseAndSetCell(Sheet *sheet, char *input, const char **status) {
    char *eq = strchr(input, '=');
    if (eq == NULL) { *status = "unrecognized cmd"; return; }

    char leftstr[7];
    strncpy(leftstr, input, eq - input);
    leftstr[eq - input] = '\0';

    char *rightstr = eq + 1;

    int row = cellNameToRow(leftstr);
    int col = cellNameToCol(leftstr);

    int error = 0;
    int result = evaluateExpression(sheet, rightstr, &error);

    if (error) {
        sheet->cells[row][col].is_error = 1;
        *status = "div by zero";
    } else {
        sheet->cells[row][col].value = result;
        sheet->cells[row][col].is_error = 0;
        sheet->cells[row][col].formula = strdup(input);
        *status = "ok";
    }
}