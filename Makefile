all: 
	g++ gridAdmin.cpp -o GridAdmin.exe -lssh -pthread -lncurses
clean:
	rm -f GridAdmin.exe
