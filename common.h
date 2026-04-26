/*
 * common.h - Shared Data Structures for the Spreadsheet Program
 *
 * This header defines the three core structures used across the program.
 * All other files include this to stay consistent with type definitions.
 * The dependency system lets cells track each other so the sheet can
 * automatically recalculate values when something changes.
 */

#ifndef COMMON_H
#define COMMON_H

#include <stdbool.h>

/* We allocate memory in chunks of 1024 rather than all at once.
 * This matters a lot for large sheets — like let's say we have a full 999x18278 grid
 * , it would
 * be enormous if we reserved memory for entire grid together. */

#define CHUNK_SIZE 1024

/* A single node in a linked list of cell relationships.
 * build two lists per cell, one for cells that it depends on
 * and one for cells that depend on it. */
typedef struct DependencyNode {
    struct Cell* cell_ptr;
    struct DependencyNode* next;
} DependencyNode;

/* to represent one cell
 * Holds the value, the formula, whether it's in an
 * error state, and dependency relationships. */
typedef struct Cell {
    double value;       /* cell's current numeric value. Store it as
                         * a double for accurate intermediate calculations,
                         * but display as an integer on the screen. */

    char* formula;      /* formula string typed by user e.g. "A1+B1".
                         * Keep this to recalculate the cell later
                         * if any of its dependencies change. NULL for constants. */

    bool is_err;        /* True if cell has error, for eg: division by zero.
                         * When we set this, the cell displays ERR instead of a number. */

    /* these two flags are used together to detect circular references, let's give an example
     * let's say, if you do A1=A1+1, it would cause infinite recalculation.
     * run a dfs and use it to spot cycles. */
    bool visited;
    bool in_stack;

    /* two linked lists form our dependency graph.
     * dependents: other cells whose formulas reference this cell.
     * dependencies: cells that this cell's formula references.
     * keeping both directions makes recalculation traversal speedy, so we'll do this. */
    DependencyNode* dependents;
    DependencyNode* dependencies;
} Cell;

/* To represent the whole spreadsheet.
 * Tracks size, the scroll position, whether output is on,
 * and the actual cell data through a chunked 3-level pointer. */
typedef struct {
    int max_rows;
    int max_cols;
    int scroll_row;       /* Row index of the top-left corner of the visible window */
    int scroll_col;       /* Column index of the top-left corner of the visible window */
    bool output_enabled;  /* use disable_output and enable_output commands here, they toggle it */

    Cell*** rows;         /* Three levels: rows[r][chunk][cell_in_chunk]
                           * Memory is only allocated when a cell is first accessed. */
} Grid;

#endif