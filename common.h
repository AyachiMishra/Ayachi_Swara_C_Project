/*
 * common.h - This file contains the shared Data Structures for the Spreadsheet Program
 *
 * This header file defines the core data structures used throughout
 * the interactive spreadsheet program. It is included by all other files to ensure
 * consistent type definitions. The three main structures are:
 * - DependencyNode: a linked list node for tracking cell relationships
 * - Cell: represents a single spreadsheet cell with its value and metadata
 * - Grid: represents the entire spreadsheet with its dimensions and state
 *
 * The dependency system uses adjacency lists (linked lists) to track
 * which cells depend on which, which enables automatic recalculation when
 * a cell's value changes.
 */

 #ifndef COMMON_H
#define COMMON_H

#include <stdbool.h>

/* CHUNK_SIZE tells us how many cells are allocated at a time in a row.
 * Instead of allocating the entire grid at once (wasteful for large sheets),
 * we allocate memory in chunks of 1024 cells only when needed.
 * This saves significant memory for large sheets for example: 999x18278. */

#define CHUNK_SIZE 1024 

typedef struct DependencyNode {
    struct Cell* cell_ptr;
    struct DependencyNode* next;
} DependencyNode;

/* Cell - Represents a single cell in the spreadsheet.
 * Each cell stores its current value, the formula used to compute it,
 * error state, cycle detection flags, and dependency information.
 * The dependency lists helps the program to know which cells need
 * to be recalculated when this cell's value changes. */

typedef struct Cell {
    double value;       /* Current value of the cell(numeric).
                         * It is stored as double to support intermediate
                         * calculations which require decimal value
                         * , converted to int for display. */

    char* formula;      /* The raw formula string for example "A1+B1".
                         * It is stored so that the cell can be recalculated when
                         * its dependencies change. It's value is 
                         * NULL if it's just a constant. */    

    bool is_err;        /* Error flag - it's set to true when an error occurs 
                         * such as division by zero.
                         * The cell displays ERR instead of a value. */        
    
    // Cycle Detection Flags

    /* Cycle Detection Flags - used during dependency graph traversal
     * to detect circular references like A1=A1+1.
     * visited: marks if this cell has been seen in current traversal.
     * in_stack: marks if this cell is in the current DFS call stack.
     * Together they implement standard cycle detection in a directed graph. */

    bool visited;  
    bool in_stack; 

    /* Adjacency Lists for the dependency graph.
     * dependents: cells that watch  cell (need recalculating when
     *             this cell changes) — like children in a tree.
     * dependencies: cells that THIS cell watches (cells whose values
     *               this cell's formula uses) — like parents in a tree.
     * Both directions are stored to allow efficient traversal both ways. */

    // Adjacency Lists
    DependencyNode* dependents;   // Cells that watch THIS cell (Children)
    DependencyNode* dependencies; // Cells that THIS cell watches (Parents)
} Cell;

/* Grid - Represents the entire spreadsheet.
 * Stores the dimensions, current scroll position, output state,
 * and the 3-level pointer structure for memory-efficient cell storage.
 * The Cell*** rows uses chunked allocation — memory is only allocated
 * for cells when they are first accessed, making large sheets possible,easy and efficient. */

typedef struct {
    int max_rows;         /* Total number of rows in the spreadsheet*/   
    int max_cols;         /* Total number of columns in the spreadsheet*/
    int scroll_row;       /* Top left row of current 10x10 view (0-indexed)*/
    int scroll_col;       /* Top left col of current 10x10 view (0-indexed)*/
    bool output_enabled;  /* Controls whether sheet is printed after commands.
                           * Set to false by disable_output, true by enable_output */
                          
    Cell*** rows;         /* 3-level pointer for chunked cell storage:
                             * rows[r] = array of chunk pointers for row r
                             * rows[r][chunk] = array of CHUNK_SIZE cells
                             * rows[r][chunk][cell] = individual cell */
} Grid;

#endif  