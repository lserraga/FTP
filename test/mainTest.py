#!/usr/bin/python3

#Test that tests the correct functionallity of the ftp server-ftp client programs
#Include it in a Test directory but excecute it in the main directory python3 test/mainTest.py
#The test directory has to contain the 6 test files "Test1.txt","Test2.docx","Test3.jpg","Test4.pdf","Test5.c","Test6.png"


import time
import os
import pexpect
from operator import eq
from signal import SIGKILL
import sys
from random import randint
import filecmp
from os.path import isfile

port = str(randint(1000,5000))
ip = "127.0.0.1"
server_name = os.path.abspath(os.path.join(os.getcwd(),
                                           "bin/ftpserver"))
client_name = os.path.abspath(os.path.join(os.getcwd(),
                                           "bin/ftpclient"))

prompt = "ftp>"
testFiles = ["Test1.txt","Test2.docx","Test3.jpg","Test4.pdf","Test5.c","Test6.png"]

def test(function, message, *args, **kwargs):
    try:
        r = function(*args, **kwargs)
        if not r:
            print(message)
        return r
    except Exception as e:
        print("{}: {}".format(message, e))


def test_command(program, command):
    
    program.sendline(command)
    program.expect(prompt)

def test_command2(program, command, out):
    
    program.sendline(command)
    program.expect(prompt)
    lines = [l.strip() for l in program.before.splitlines()]
    l = [l.decode(sys.stdout.encoding) for l in lines]
    aux=""
    for i in range(1,len(lines)):
        aux+=lines[i].decode("utf-8")
        if(i!=(len(lines)-1)):
            aux+="\n"
    error_message = "Output does not correspond to expected for command "+ command + ".\nExpected:\n"+out+"\n\nFound:\n"+aux+"\n"
    return test(eq,error_message, out, aux)

def test_command3(program, command, out):
    
    program.sendline(command)
    program.expect(prompt)
    lines = [l.strip() for l in program.before.splitlines()]
    l = [l.decode(sys.stdout.encoding) for l in lines]
    aux=""
    for i in range(0,len(lines)):
        aux+=lines[i].decode("utf-8")
        if(i!=(len(lines)-1)):
            aux+="\n"
    error_message = "Output does not correspond to expected for command "+ command + ".\nExpected:\n"+out+"\n\nFound:\n"+aux+"\n"
    return test(eq,error_message, out, aux)

print("\n\n****This test will make sure that the server can handle correct and incorrect commands properly****\n\n")

dest = os.path.join(os.getcwd(), "test")
dest2 = os.path.join(os.getcwd(), "testOut")

# Test normalality

server = test(pexpect.spawn, "Server did not start properly",
              server_name, [port], cwd=dest)
client = test(pexpect.spawn, "Client did not start properly",
              client_name, [ip, port],cwd=dest2)
if not server.isalive():
    print("Server's port is already being used")
    exit(0)

test(client.expect,  "", prompt)

print ("Client and server started properly\n")

print("****Testing retrieving a file from the server****\n")


result=0
for file in testFiles:
    command = "get "+ file

    test_command(client, command)

    originalFile = "test/" + file
    copiedFile = "testOut/" + file

    if not filecmp.cmp(originalFile, copiedFile):
        print("%s was not succesfully retrieved from the server" % file)
    else:
        print("%s was succesfully retrieved from the server" % file)
    
    if not server.isalive() or not client.isalive():
        print("Server crashed after command '%s'" % command)


print("All get commands ran apropiatelly\n")

print("****Testing the put command****\n")
#Creating a file with random data in the client directory and trying to send it to the server
os.system("dd if=/dev/zero of=testOut/testPut.txt  bs=1M  count=100") # 100MB of random data)

test_command(client, "put testPut.txt")

if not filecmp.cmpfiles("testOut","test",("testPut.txt", "testPut.txt")):
        print("Put test failed")
else:
    print("Put was successfull")


print("\n****Testing ls commands****")

paths=["","..","../bin","../test"]
ok=True
for path in paths:
    desiredDir=os.listdir("./test/"+path)
    listOutput="File listing for the directory" 

    if (len(path)!=0):
        listOutput+=" "

    listOutput+= path +"\n.\t\tDIR\n..\t\tDIR\n"
    for i in desiredDir:
        listOutput+=i + "\t\t"
        pathFiel="test/"
        if len(path)!=0:
            pathFiel+=path+"/";
        if isfile(pathFiel+i):
            listOutput+="FILE\n"
        else:
            listOutput+="DIR\n"

    if(test_command2(client, ("ls " + path),listOutput)):
        print("Command ls %s worked fine" % path)
    else:
        ok=False
if ok:
    print("All listing commands worked fine")

print("\nChecking if programs are still alive: Server:%s Client:%s\n\n" % (server.isalive(),client.isalive()))

print("Quiting from the client:")

client.sendline("quit\n")

time.sleep(2)

print("Checking if programs are still alive: Server:%s Client:%s" % (server.isalive(),client.isalive()))

print("\nRemoving copied files")

for file in testFiles:

    copiedFile = "testOut/" + file
    
    os.remove(copiedFile)

os.remove("testOut/testPut.txt")
os.remove("test/testPut.txt")