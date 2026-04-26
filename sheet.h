#ifndef SHEET_H
#define SHEET_H


typedef struct {
    int value;          
    char *formula;      
    int is_error;       
    int *deps;          
    int dep_count;      
} Cell;


typedef struct {
    int rows;           
    int cols;           
    Cell **cells;       
    int view_row;       
    int view_col;       
    int output_enabled; 
} Sheet;


Sheet *createSheet(int rows, int cols);
void freeSheet(Sheet *sheet);


void printSheet(Sheet *sheet);


int colNameToIndex(const char *col);   // e.g. "A" -> 0, "ZZZ" -> max
int cellNameToRow(const char *cell);   // e.g. "A3" -> 2 (0-indexed)
int cellNameToCol(const char *cell);   // e.g. "B3" -> 1 (0-indexed)


void setCell(Sheet *sheet, int row, int col, const char *formula);
void recalculate(Sheet *sheet, int row, int col);

#endif

void indexToColName(int index, char *buf);

int colNameToIndex(const char *col);
int cellNameToCol(const char *cell);
int cellNameToRow(const char *cell);

int isValidCellName(char *s);
int isFormula(char *s);

int evaluateValue(Sheet *sheet, char *v, int *error);


int evaluateExpression(Sheet *sheet, char *expr, int *error);

void parseAndSetCell(Sheet *sheet, char *input, const char **status);