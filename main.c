/*main.c

This is the central manager of the entire system. Its responsibilities include running the main input loop,
supporting the 3 additional commands(disable/enable outputs, and scrollign to specific cell), tracking command execution time
and lastly the regularly scrolling functionality using w,a,s,d keys. 

It connects all the components(structs) and functions defined in other code files(This file is also dependent on the files containing these!).

Flow:
1. We first import all standard library files and the components and functions we defined in other files.
2. The main function keeps listening for the command ./sheet R C after which it starts the main input loop ( while True loop)
3. create the grid struct and parse the command (after parsing route it to the relevant subroutine )
5. Clear old dependencies
6. check for cycles
7. Recursively update all the dependent cells, their dependents and so on...

*/



// Import of standard library functionalities and our defined structs
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "common.h"
#include <ctype.h>



// Importing our defined components and functionalities from other files
extern Grid* init_grid(int r, int c);
extern void print_sheet(Grid* g);
extern Cell* get_cell(Grid* g, int r, int c);
extern int col_to_idx(const char* col_str);
extern void clear_dependencies(Cell* c);
extern void reset_graph_flags(Grid* g);
extern bool has_cycle(Cell* c);
extern void update_cell(Grid* g, Cell* c);
extern double evaluate_formula(Grid* g, Cell* target, char* expr, bool* error_flag);



int main(int argc, char* argv[]) {
    // Reading Command Line Arguments and initialising the grid with its parameters (rows, columns, etc.)
    if (argc != 3) {
        printf("Usage: ./sheet R C\n");
        return 1;
    }

    int R = atoi(argv[1]);
    int C = atoi(argv[2]);

    // This creates the grid struct with the pointer to it named g
    Grid* g = init_grid(R, C);
    char input[256];
    double last_exec_time = 0.0;

    // a while True loop (continues infinitely unless stopped using the q (quit) command)
    while (1) {
        print_sheet(g);
        printf("[%0.3f] (ok) > ", last_exec_time);
        if (!fgets(input, sizeof(input), stdin)) 
            break;

        input[strcspn(input, "\n")] = 0; 
        if (strcmp(input, "q") == 0) 
            break;

        
        if (strstr(input, "=")) {
            char col_str[10], formula[200];
            int row_num;
            
            struct timespec start, end;
            clock_gettime(CLOCK_MONOTONIC, &start);

            // Parse the command into: <Cell Coordinates> = <Formula String>
            // Basically, splits the input into letters(columns), numbers(rows) and the string after  '=' sign [the formula to be applied]
            if (sscanf(input, "%[A-Z]%d=%s", col_str, &row_num, formula) == 3) {
                Cell* c = get_cell(g, row_num - 1, col_to_idx(col_str));

                if (c) {
                    // 1. Cleanup old dependencies 
                    clear_dependencies(c);
                    if (c->formula) free(c->formula);
                    c->formula = strdup(formula);

                    // 2. Prepare for Cycle Detection
                    reset_graph_flags(g);
                    bool parse_err = false;
                    
                    // 3. Initial evaluation and dependency registration
                    c->value = evaluate_formula(g, c, c->formula, &parse_err);
                    
                    if (has_cycle(c)) {
                        printf("Cycle detected!\n");
                        c->is_err = true;
                    } else {
                        c->is_err = parse_err;
                        // 4. Recursive propagation
                        update_cell(g, c);
                    }
                }
            }
            
            clock_gettime(CLOCK_MONOTONIC, &end);
            last_exec_time = (end.tv_sec - start.tv_sec) + 
                             (end.tv_nsec - start.tv_nsec) / 1e9;
        } 
        
        // below is the implementation of the 3 helper commands (first two commands just require setting the flag)
        // 'output_enabled' to the correct value
        else if (strcmp(input, "disable_output") == 0) {
            g->output_enabled = false; }
        else if (strcmp(input, "enable_output") == 0) {
            g->output_enabled = true; }

        else if (strncmp(input, "scroll_to ", 10) == 0) {
            char cell_str[20];
            if (sscanf(input + 10, "%s", cell_str) == 1) {
                char col_str[10];
                int row_num;
                
                int i = 0;
                while (cell_str[i] && isalpha(cell_str[i])) {
                    col_str[i] = cell_str[i];
                    i++;
                }
                col_str[i] = '\0';
                row_num = atoi(cell_str + i);

                
                g->scroll_row = row_num - 1; 
                g->scroll_col = col_to_idx(col_str); 

                
                if (g->scroll_row < 0) g->scroll_row = 0;
                if (g->scroll_col < 0) g->scroll_col = 0;
                if (g->scroll_row >= g->max_rows) g->scroll_row = g->max_rows - 1;
                if (g->scroll_col >= g->max_cols) g->scroll_col = g->max_cols - 1;
            }
        }
    
        // The w,a,s,d scrolling commands
        else if (strcmp(input, "s") == 0) g->scroll_row += 10;
        else if (strcmp(input, "w") == 0) g->scroll_row -= 10;
        else if (strcmp(input, "a") == 0) g->scroll_col -= 10;
        else if (strcmp(input, "d") == 0) g->scroll_col += 10;

        // You can't scroll upwards or leftwards if you are already at the start
        if (g->scroll_row < 0) g->scroll_row = 0;
        if (g->scroll_col < 0) g->scroll_col = 0;
    }

    return 0;
}