CC = gcc
CFLAGS = -Wall -Wextra -g
OBJ = main.o grid.o parser.o engine.o

# The default target
all: sheet

sheet: $(OBJ)
	$(CC) $(CFLAGS) -o sheet $(OBJ) -lm

%.o: %.c
	$(CC) $(CFLAGS) -c $<

# New targets for the test suite
test: sheet test_suite
	./test_suite

test_suite: test_suite.c
	$(CC) $(CFLAGS) -o test_suite test_suite.c

# Combined clean target
clean:
	rm -f *.o sheet test_suite report.pdf report.log report.aux

# Lab requirement: produce report from LaTeX
report:
	pdflatex report.tex