/*
----
dependency.c

This file manages the dependency graph between cells.

It handles:
- linking cells when formulas reference other cells
- removing old dependencies when formulas change
- detecting cycles in formulas
- propagating updates when a cell value changes

This is what enables automatic recalculation in the spreadsheet.
----
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"

// External parser function
extern double evaluate_formula(Grid* g, Cell* target, char* expr, bool* error_flag);


/*
----
remove_node

Removes a specific cell from a linked list of dependencies.
Used when cleaning up old relationships before re-parsing a formula.

This prevents stale or incorrect dependency links from staying in the graph.
----
*/
void remove_node(DependencyNode** head, Cell* target) {
    DependencyNode* curr = *head;
    DependencyNode* prev = NULL;

    while (curr) {
        if (curr->cell_ptr == target) {
            if (prev)
                prev->next = curr->next;
            else
                *head = curr->next;

            free(curr);
            return;
        }

        prev = curr;
        curr = curr->next;
    }
}


/*
----
clear_dependencies

Removes all existing dependencies of a cell.
Also ensures this cell is removed from all parent "dependents" lists.

This is needed before assigning a new formula to avoid incorrect graph links.
----
*/
void clear_dependencies(Cell* c) {
    DependencyNode* curr = c->dependencies;

    while (curr) {
        // Remove this cell from the parent’s dependent list
        remove_node(&(curr->cell_ptr->dependents), c);

        DependencyNode* next = curr->next;
        free(curr);
        curr = next;
    }

    c->dependencies = NULL;
}


/*
----
register_dependency

Links two cells in the dependency graph.

target depends on parent:
- parent → added to target's dependencies
- target → added to parent's dependents

This allows updates to flow correctly during recomputation.
----
*/
void register_dependency(Cell* target, Cell* parent) {
    // Add parent to target's dependency list
    DependencyNode* newNode = malloc(sizeof(DependencyNode));
    newNode->cell_ptr = parent;
    newNode->next = target->dependencies;
    target->dependencies = newNode;

    // Add target to parent's dependent list
    DependencyNode* depNode = malloc(sizeof(DependencyNode));
    depNode->cell_ptr = target;
    depNode->next = parent->dependents;
    parent->dependents = depNode;
}


/*
----
has_cycle

Performs DFS to detect cycles in the dependency graph.

Uses:
- visited → to avoid reprocessing
- in_stack → to detect back edges (cycle)

If a cycle is found, the formula is invalid.
----
*/
bool has_cycle(Cell* c) {
    if (c->in_stack) return true;
    if (c->visited) return false;

    c->visited = true;
    c->in_stack = true;

    DependencyNode* curr = c->dependencies;

    while (curr) {
        if (has_cycle(curr->cell_ptr))
            return true;

        curr = curr->next;
    }

    c->in_stack = false;
    return false;
}


/*
----
update_cell

Recomputes a cell’s value and propagates the update to dependent cells.

Steps:
1. Re-evaluate formula
2. Update value and error flag
3. Recursively update all dependent cells

This ensures automatic recalculation across the spreadsheet.
----
*/
void update_cell(Grid* g, Cell* c) {
    if (!c->formula) return;

    bool error_flag = false;

    double new_val = evaluate_formula(g, c, c->formula, &error_flag);

    c->value = new_val;
    c->is_err = error_flag;

    // Push update to all dependent cells
    DependencyNode* curr = c->dependents;

    while (curr) {
        update_cell(g, curr->cell_ptr);
        curr = curr->next;
    }
}


/*
----
reset_graph_flags

Resets DFS-related flags (visited and in_stack) for all allocated cells.

Needed before running a fresh cycle detection pass to avoid stale state.
----
*/
void reset_graph_flags(Grid* g) {
    for (int r = 0; r < g->max_rows; r++) {
        if (g->rows[r] == NULL) continue;

        int num_chunks = (g->max_cols + CHUNK_SIZE - 1) / CHUNK_SIZE;

        for (int ch = 0; ch < num_chunks; ch++) {
            if (g->rows[r][ch] == NULL) continue;

            for (int i = 0; i < CHUNK_SIZE; i++) {
                g->rows[r][ch][i].visited = false;
                g->rows[r][ch][i].in_stack = false;
            }
        }
    }
}