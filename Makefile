default: prog

get-deps:
	# Assuming Debian or Ubuntu here
	sudo apt -y update
	sudo apt-get install -y build-essential check

dictionary.o: dictionary.c
	~/afl-2.52b/afl-gcc -Wall -c dictionary.c dictionary.h
	#gcc -Wall -c dictionary.c dictionary.h

spell.o: spell.c
	~/afl-2.52b/afl-gcc -Wall -c spell.c
	#gcc -Wall -c spell.c

test.o: test_main.c
	gcc -Wall -c test_main.c

main.o: main.c
	gcc -Wall -c main.c

test: dictionary.o spell.o test_main.o
	gcc -Wall -o test_main test_main.o spell.o dictionary.o -lcheck -lm -lrt -lpthread -lsubunit
	./test_main

prog: dictionary.o spell.o #main.o
	~/afl-2.52b/afl-gcc -Wall -o spell_check dictionary.o spell.o #main.o
	#gcc -Wall -o spell_check dictionary.o spell.o #main.o

# Add || true to allow chaining of make clean, even if not all object files exist
# Also suppress errors
clean:
	rm dictionary.o spell.o main.o test_main.o check_spell.o > /dev/null 2>&1 || true

cleanall:clean
	rm spell_check
	rm test_main
