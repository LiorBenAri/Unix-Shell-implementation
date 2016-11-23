#format is target-name: target dependencies
#{-tab-}actions

# All Targets
all: myShell

# Tool invocations
myShell: myShell.o LineParser.o
	gcc -g -m32 -Wall -o myShell LineParser.o myShell.o
myShell.o: myShell.c
	gcc -m32 -g -Wall -c -o myShell.o myShell.c
LineParser.o: LineParser.c
	gcc -m32 -g -Wall -c -o LineParser.o LineParser.c

#tell make that "clean" is not a file name!
.PHONY: clean

#Clean the build directory
clean: 
	rm -f *.o myShell