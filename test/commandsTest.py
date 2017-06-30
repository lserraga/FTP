#!/usr/bin/python3

import os
import pexpect
from operator import eq
from signal import SIGKILL
from random import randint
import sys

port = str(randint(1000,5000))
ip = "127.0.0.1"
server_name = os.path.abspath(os.path.join(os.getcwd(),
                                           "bin/ftpserver"))
client_name = os.path.abspath(os.path.join(os.getcwd(),
                                           "bin/commandTest"))
prompt = "ftp>"
commands = [
    ("RETR test.txt 12", "501 Syntax error (Too many arguments)"),
    ("Fake command", "500 Syntax error (unrecognized command)"),
    ("STOR", "501 Syntax error (Not enough arguments)"),
    ("RETR WDVHUWENUVNEU.txt", "550 File Not Found"),
    ("PORT wdfsdfs", "502 Port command not expected"),
    ("LIST FakeDir", "550 Directory/File Not Found"),
    ("LIST", "200 Command OK"),
    ("LIST", "502 Expected port command"),
    ("LIST", "200 Command OK"),
    ("PORT 1,2,3,4,555,6", "501 Syntax error (PORT: not a valid address)"),
    ("LIST", "200 Command OK"),
    ("PORT 1,2,3,4,5,6", "425 Can't open data connection"),
    ("ABOR", "200 Command OK"),
    ("TEST", "THIS IS A TEST THAT THE TESTING WORKS"),
    ("QUIT","221 Goodbye")]

def test(function, message, *args, **kwargs):
    try:
        r = function(*args, **kwargs)
        if not r:
            print(message)
        return r
    except Exception as e:
        print("{}: {}".format(message, e))



def test_command(program, command, out):
    
    program.sendline(command)
    program.expect(prompt)
    lines = [l.strip() for l in program.before.splitlines()]
    l = [l.decode(sys.stdout.encoding) for l in lines]
    aux=lines[1].decode("utf-8")
    error_message = "Output does not correspond to expected for command "+ command + ".\nExpected:\n"+out+"\n\nFound:\n"+aux+"\n"
    return test(eq, error_message, out, aux)

print("\n\n****This test will make sure that the server can handle correct and incorrect commands properly****\n\n")

dest = os.path.join(os.getcwd(), "testOut")

# Test normalality

server = test(pexpect.spawn, "Server did not start properly",
              server_name, [port], cwd=dest)
client = test(pexpect.spawn, "Client did not start properly",
              client_name, [ip, port])
if not server.isalive():
    print("Server's port is already being used")
    exit(0)

test(client.expect,  "", prompt)

print ("Client and server started properly")

print("Testing server commands")

result=0
for t in commands:
    com = t[0]
    out = t[1]
    if not test_command(client, com, out):
        result+=1
    if not server.isalive():
        print("Server crashed after command '%s'" % com)

if result==1:
    print("All commands ran appropriately")
