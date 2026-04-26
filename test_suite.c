/*test_suite.c

To make the test suite automated,
instead of doing doing Keyboard > program > terminal , the test suite simulates this manual process
using parent > pipe > program > pipe > parent

In the manual case, we can see that keyboard and terminal are the main phases of interaction with the program. 

Here parent process can be visualised as the user/interactor and child process as the running program..
The parent process interacts with the child (sending inputs and reading outputs) using pipes

The main idea is that the child process cannot differentiate whether the parent process is an actual user typign 
in from the keyboard or the simulated process as defined here

Also, every output also comes to the parent process using the output pipe. Here the output string can be 
compared  with the expected string using strcmpr function and then pass or fail gets declared.

*/ 



#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdbool.h>

/*check_output:
  
 Reads the output produced by the child process (the grid) via a pipe
 and checks whether a given expected substring appears in it. It briefly
 waits to allow the child to finish computation (e.g., SLEEP), then reads
 from the pipe into a buffer and performs a substring match using strstr.
*/
bool check_output(int read_fd, const char* expected) {
    char buffer[8192];
    // Give the process a moment to compute if SLEEP is involved
    usleep(100000); 

    // Read the data from the piper to the buffer
    int n = read(read_fd, buffer, sizeof(buffer) - 1);

    if (n <= 0) 
        return false;
    buffer[n] = '\0';

    // checks whether expected substring exists in the buffer
    return strstr(buffer, expected) != NULL;
}

void run_test(const char* name, const char* commands, const char* expected_result) {
    int to_sheet[2], from_sheet[2];
    pipe(to_sheet); pipe(from_sheet);

    pid_t pid = fork();
    if (pid == 0) {
        dup2(to_sheet[0], STDIN_FILENO);
        dup2(from_sheet[1], STDOUT_FILENO);
        close(to_sheet[1]); close(from_sheet[0]);
        execl("./sheet", "./sheet", "20", "20", (char*)NULL);
        exit(1);
    }

    close(to_sheet[0]); close(from_sheet[1]);
    write(to_sheet[1], commands, strlen(commands));
    write(to_sheet[1], "q\n", 2);

    bool success = check_output(from_sheet[0], expected_result);
    printf("Test %-25s: %s\n", name, success ? "PASSED " : "FAILED ");

    close(to_sheet[1]); close(from_sheet[0]);
    wait(NULL);
}

int main() {
    printf("=== Automated Test Suite ===\n\n");

    // A. Arithmetic & Precedence
    run_test("Arithmetic Precedence", "A1=2+3*4\n", "14");
    run_test("Integer Truncation", "A1=5/2\n", "2");

    // B. Range Functions
    run_test("SUM Range", "A1=5\nA2=10\nA3=SUM(A1:A2)\n", "15");
    run_test("AVG Range", "A1=10\nA2=20\nA3=AVG(A1:A2)\n", "15");
    run_test("MIN/MAX Range", "A1=5\nA2=10\nB1=MIN(A1:A2)\nB2=MAX(A1:A2)\n", "5");
    run_test("Rectangular Range", "A1=1\nB1=1\nA2=1\nB2=1\nC1=SUM(A1:B2)\n", "4");

    // C. Error Handling
    run_test("Division by Zero", "A1=1/0\n", "ERR");
    run_test("Circular (Self)", "A1=A1+1\n", "Cycle detected!");
    run_test("Circular (Mutual)", "A1=B1\nB1=A1\n", "Cycle detected!");

    // D. Automatic Recalculation
    run_test("Chain Update", "A1=10\nB1=A1+1\nA1=20\n", "21");
    
    // E. System & Helper Commands
    run_test("Scrolling (scroll_to)", "scroll_to C15\n", "15"); // Checks if row 15 is visible
    run_test("Output Control", "disable_output\nA1=5\n", ""); // Expecting minimal/no grid output

    printf("\nSuite Complete.\n");
    return 0;
}