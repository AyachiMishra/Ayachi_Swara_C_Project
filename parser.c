/*
----
parser.c

This file handles all formula parsing and evaluation logic.
It takes raw formula strings (like A1+B1 or SUM(A1:B2)),
breaks them down, computes the result, and returns the final value.

Unlike grid.c (which manages storage) or main.c (which handles input),
this file is focused purely on understanding and evaluating expressions.
----
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <unistd.h>
#include "common.h"

extern Cell* get_cell(Grid* g, int r, int c);
extern int col_to_idx(const char* col_str);
extern void register_dependency(Cell* target, Cell* parent);


/*
----
evaluate_range

This function evaluates range-based functions like SUM(A1:B2), MIN, MAX, etc.
It parses the range, loops through all cells inside it, and applies the function.

Needed because functions operate on multiple cells, not just single values.
----
*/
double evaluate_range(Grid* g, char* range_str, char* func, bool* err) {
    char s_col[5], e_col[5];
    int s_row, e_row;

    if (sscanf(range_str, "%[A-Z]%d:%[A-Z]%d", s_col, &s_row, e_col, &e_row) != 4) {
        *err = true;
        return 0;
    }

    int sc = col_to_idx(s_col), ec = col_to_idx(e_col);
    int sr = s_row - 1, er = e_row - 1;

    if (sr > er || sc > ec) {
        *err = true;
        return 0;
    }

    double sum = 0, min_v = INFINITY, max_v = -INFINITY;
    int count = 0;

    for (int r = sr; r <= er; r++) {
        for (int c = sc; c <= ec; c++) {
            Cell* cell = get_cell(g, r, c);

            // If any cell in range is invalid, entire result becomes ERR
            if (!cell || cell->is_err) {
                *err = true;
                return 0;
            }

            double v = cell->value;

            sum += v;
            if (v < min_v) min_v = v;
            if (v > max_v) max_v = v;

            count++;
        }
    }

    if (strcmp(func, "SUM") == 0) return sum;
    if (strcmp(func, "MIN") == 0) return min_v;
    if (strcmp(func, "MAX") == 0) return max_v;
    if (strcmp(func, "AVG") == 0) return sum / count;

    if (strcmp(func, "STDEV") == 0) {
        double avg = sum / count, var = 0;

        for (int r = sr; r <= er; r++) {
            for (int c = sc; c <= ec; c++) {
                double v = get_cell(g, r, c)->value;
                var += (v - avg) * (v - avg);
            }
        }

        return sqrt(var / count);
    }

    return 0;
}


/*
----
precedence

Returns priority of operators.
Used in expression evaluation to decide which operation to apply first.

Higher number = higher priority (* and / > + and -)
----
*/
int precedence(char op) {
    if (op == '+' || op == '-') return 1;
    if (op == '*' || op == '/') return 2;
    return 0;
}


/*
----
apply_op

Applies a binary operator on two values.
Also checks for division by zero and sets error flag if needed.

This is used during expression evaluation (Shunting Yard logic).
----
*/
double apply_op(double a, double b, char op, bool* err) {
    switch (op) {
        case '+': return a + b;
        case '-': return a - b;
        case '*': return a * b;
        case '/':
            if (b == 0) {
                *err = true;  // division by zero
                return 0;
            }
            return a / b;
    }
    return 0;
}


/*
----
evaluate_formula

Main parser function.
Takes a formula string and evaluates it using a stack-based approach.

It supports:
- numbers
- cell references
- operators (+ - * /)
- functions (SUM, AVG, etc.)
- parentheses

Also builds dependency links and propagates errors.

This is the core engine that powers spreadsheet calculations.
----
*/
double evaluate_formula(Grid* g, Cell* target, char* expr, bool* error_flag) {
    double values[200];
    int v_top = -1;

    char ops[200];
    int o_top = -1;

    char* ptr = expr;
    *error_flag = false;

    while (*ptr) {
        if (isspace(*ptr)) {
            ptr++;
            continue;
        }

        // --- Numbers ---
        if (isdigit(*ptr)) {
            int offset;
            sscanf(ptr, "%lf%n", &values[++v_top], &offset);
            ptr += offset;
        }

        // --- Functions / Ranges / Cells ---
        else if (isupper(*ptr)) {
            char name[10];
            int offset;

            sscanf(ptr, "%[A-Z]%n", name, &offset);
            char* next = ptr + offset;

            // Function call like SUM(...)
            if (*next == '(') {
                ptr = next + 1;

                char args[100];
                int k = 0, parens = 1;

                // Extract contents inside parentheses safely
                while (parens > 0 && *ptr) {
                    if (*ptr == '(') parens++;
                    if (*ptr == ')') parens--;

                    if (parens > 0) args[k++] = *ptr++;
                }

                args[k] = '\0';
                ptr++; // skip closing ')'

                if (strcmp(name, "SLEEP") == 0) {
                    int s = atoi(args);
                    sleep(s);  // pauses execution
                    values[++v_top] = (double)s;
                } else {
                    values[++v_top] = evaluate_range(g, args, name, error_flag);
                }
            }

            // Cell reference like A1
            else {
                int r_num, row_off;
                sscanf(next, "%d%n", &r_num, &row_off);

                Cell* ref = get_cell(g, r_num - 1, col_to_idx(name));

                if (!ref || ref->is_err) {
                    *error_flag = true;
                    return 0;
                }

                // Register dependency for recalculation graph
                register_dependency(target, ref);

                values[++v_top] = ref->value;
                ptr = next + row_off;
            }
        }

        // --- Opening parenthesis ---
        else if (*ptr == '(') {
            ops[++o_top] = *ptr++;
        }

        // --- Closing parenthesis ---
        else if (*ptr == ')') {
            while (o_top >= 0 && ops[o_top] != '(') {
                double v2 = values[v_top--];
                double v1 = values[v_top--];

                values[++v_top] = apply_op(v1, v2, ops[o_top--], error_flag);
            }

            o_top--; // remove '('
            ptr++;
        }

        // --- Operators ---
        else {
            // Apply operators with higher or equal precedence first
            while (o_top >= 0 && precedence(ops[o_top]) >= precedence(*ptr)) {
                double v2 = values[v_top--];
                double v1 = values[v_top--];

                values[++v_top] = apply_op(v1, v2, ops[o_top--], error_flag);
            }

            ops[++o_top] = *ptr++;
        }

        if (*error_flag) return 0;
    }

    // Apply remaining operators
    while (o_top >= 0) {
        double v2 = values[v_top--];
        double v1 = values[v_top--];

        values[++v_top] = apply_op(v1, v2, ops[o_top--], error_flag);

        if (*error_flag) return 0;
    }

    return values[0];
}