all: sheet

sheet: main.c sheet.c sheet.h
	gcc -o sheet main.c sheet.c -lm -Wall

clean:
	rm -f sheet

test:
	./test_sheet