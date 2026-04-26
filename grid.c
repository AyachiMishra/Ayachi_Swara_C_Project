#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include "common.h"

/*
 * grid.c - Grid Initialization, Cell Access, and Display
 *
 * we'll do these tasks here:
 * 1. Initialization: Sets up the grid with lazy memory allocation
 * 2. Cell Access: Safely retrieves cells using chunked memory system
 * 3. Display: Renders the current 10x10 view to the terminal
 * 4. Column conversion: Converts column names to zero-based indices
 */

// --- Prototypes(forward declarations) to prevent compiler warnings ---
Grid* init_grid(int r, int c);
void print_col_header(int c);
Cell* get_cell(Grid* g, int r, int c);
void print_sheet(Grid* g);
int col_to_idx(const char* col_str);

// --- Implementation ---

/* creates and initialize a new grid with given dimensions.
 * But it uses lazy allocation — cell memory is only allocated when it's first accessed,
 * making large sheets like 999x18278 possible and efficient without wasting memory. */

Grid* init_grid(int r, int c) {
    Grid* g = (Grid*)malloc(sizeof(Grid));
    g->max_rows = r;
    g->max_cols = c;
    g->scroll_row = 0;
    g->scroll_col = 0;
    g->output_enabled = true;
    
    

    /* calloc ensures all row pointers to start off are equal to NULL,
     * which shows us that no chunks are allocated yet */

    g->rows = (Cell***)calloc(r, sizeof(Cell**));
    return g;
}

/* Recursively prints column header letters like (A, Z, AA, ZZZ etc.)
 * recursion takes care of cases where columns with multiple letters where each letter
 * depends on the remaining value after division. */

void print_col_header(int c) {
    if (c >= 26) print_col_header(c / 26 - 1);
    putchar('A' + (c % 26));
}

/* retrieves cell's pointer, allocating memory if needed.
 * chunks of CHUNK_SIZE cells are allocated for every row on the first access,
 * all fields are initialized to proper defaults to prevent
 * wrong or weird behavior from uninitialized memory. */

Cell* get_cell(Grid* g, int r, int c) {
    /* returns NULL if access is out-of-bounds*/
    if (r < 0 || r >= g->max_rows || c < 0 || c >= g->max_cols) return NULL;
    int chunk_idx = c / CHUNK_SIZE;
    int cell_in_chunk = c % CHUNK_SIZE;

    /* allocates chunk pointer array for this row on first access */

    if (g->rows[r] == NULL) {
        int num_chunks = (g->max_cols + CHUNK_SIZE - 1) / CHUNK_SIZE;
        g->rows[r] = (Cell**)calloc(num_chunks, sizeof(Cell*));
    }

    /* Allocate chunk and initialize all cells on first access */

    if (g->rows[r][chunk_idx] == NULL) {
        g->rows[r][chunk_idx] = (Cell*)calloc(CHUNK_SIZE, sizeof(Cell));
        for(int i = 0; i < CHUNK_SIZE; i++) {
            g->rows[r][chunk_idx][i].value = 0.0; /* Updated to double to store values as double internally for intermediate calculations.
                                                   * But, since the requirement is to display values 
                                                   * as integer to the user
                                                  */ 

            g->rows[r][chunk_idx][i].formula = NULL;
            g->rows[r][chunk_idx][i].is_err = false;

        

            /*Cycle detection flags- they're only initialized during graph traversal*/
            g->rows[r][chunk_idx][i].visited = false;
            g->rows[r][chunk_idx][i].in_stack = false;

            /*Dependency lists are built as we add formulas*/
            g->rows[r][chunk_idx][i].dependents = NULL;
            g->rows[r][chunk_idx][i].dependencies = NULL;
        }
    }
    return &g->rows[r][chunk_idx][cell_in_chunk];
}

/* Displays current 10x10 view of grid
 * doesn't printing at all if output is disabled.
 * Error celsl show ERR, normal cells show integer value. */

void print_sheet(Grid* g) {
    if (!g->output_enabled) 
        return; 

    /* print column headers with padding for row number column */
    
    printf("    ");
    for (int j = 0; j < 10 && (g->scroll_col + j) < g->max_cols; j++) {
        print_col_header(g->scroll_col + j);
        printf("   ");
    }
    printf("\n");

    for (int i = 0; i < 10 && (g->scroll_row + i) < g->max_rows; i++) {
        int actual_row = g->scroll_row + i;
        printf("%-4d", actual_row + 1);    /* +1 for 1-indexed row numbers */
        for (int j = 0; j < 10 && (g->scroll_col + j) < g->max_cols; j++) {
            int actual_col = g->scroll_col + j;
            Cell* c = get_cell(g, actual_row, actual_col);

            /* cast to int for display since spec requires integer output */

            if (c && c->is_err) {
                printf("ERR "); 
            } else {
                // truncate double to int for display, coz we're asked to do it that way
                printf("%-4d", c ? (int)c->value : 0);
            }
        }
        printf("\n");
    }
}

/* Converts column name string to zero-based index.
 * Treats column name as base-26 number, the contents in bracket show what I mean by that: (A=1, B=2 ... Z=26, AA=27).
 * if there's input like "AA", it would have to be converted to array indices. 
 * so, we need this function */

int col_to_idx(const char* col_str) {
    int idx = 0;
    while (*col_str && isalpha(*col_str)) {
        idx = idx * 26 + (toupper(*col_str) - 'A' + 1);
        col_str++;
    }
    return idx - 1;   /*Convert to zero-based index*/
}