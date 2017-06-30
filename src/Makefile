all: ftpserver ftpclient testingCommandsServer

ftpserver: ftpserver.c
	gcc -o ../bin/ftpserver ftpserver.c -I. -ggdb

ftpclient: ftpclient.c
	gcc -o ../bin/ftpclient ftpclient.c -I. -ggdb

testingCommandsServer: testingCommandsServer.c
	gcc -o ../bin/commandTest testingCommandsServer.c -I. -ggdb
clean:
	-rm -f ../bin/ftpclient ../bin/ftpserver ../bin/commandTest
