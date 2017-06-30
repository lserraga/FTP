  FTP SERVER AND CLIENT

  What is it?
  -----------

  The ftp server and client allows a user to setup retrieve and store files
  from and to the server and get directory listing of the server.

  Documentation
  -------------

  Included in the doc directory.

  Installation
  ------------

  Executing make in the main directory compiles all the files needed. The 
  compiled files are inside the bin directory (to get meaningful results
  excecute the client and server in different locations)

  Files
  --------
  /bin/
      ftpclient : executable for the ftp client program
      ftpserver : executable for the ftp client program
      commandTest : executable for the server commands test


  /doc/
      protocol.txt : A specification of the application-layer protocol.
      usage.txt : Documentation of the design usage.
      notesForTA.txt : some notes for the TA.

  /src/
      ftpclient.c : contains the code for the implementation of the
      client
      ftpserver.c : contains the code for the implementation of the
      server
      testingCommandsServer.c : contains the code for the testing of the
      server commands
      Makefile : compiles the previous files. Also contains a clean function

  /test/
      commandsTest.py : contains a python script that tests the correct 
      functionallity of the server commands (STOR, RETR,...)
      mainTest.py : contains a python script that test the correct 
      functionallity of the whole application (with get, put, ls and 
      quit commands)
      6 files with different extensions used by mainTest.py to check
      if there is any problem transfering files

  /testOut/
      This directory is used by mainTest.py to temporarelly send and retreive
      files from it and it deletes them at the end

  Makefile : calls the Makefile in /src/ and also includes a clean
  function

  Contacts
  --------

  Luis Serra Garcia, Creator: lserraga@ucsc.edu
