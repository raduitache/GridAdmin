all: 
	g++ gridAdmin.cpp -o GridAdmin.exe -lssh -pthread -lncurses -lrt
clean:
	rm -f GridAdmin.exe
