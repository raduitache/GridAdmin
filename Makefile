all: 
	g++ gridAdmin.cpp -o GridAdmin.exe -lssh
clean:
	rm -f GridAdmin.exe
