How to compile:
	Type "make" in the terminal, or type "gcc client.c -0 client".
------------------------------------------------------------------------------
How to execute:
	In the terminal, type "./server<space><port>"
	In the terminal, type "./client<space><server ip address><space><port>".
	
	When the client executed successfully, the client asked you to type "1" to register or "2" to login. If you type "1", you have to enter a username to register. If you type "2", you have to enter a username and a port number to login.
	After loginned, the client asked you to type "1" to see the online account list or "2" to exit the server. If you type "1", the client asked server to return the online account list. If you type "2", the client exit the server and the stop itself.
------------------------------------------------------------------------------
Execute Environment:
ubuntu 14.04
gcc version 4.8.2 (Ubuntu 4.8.2-19ubuntu1)
