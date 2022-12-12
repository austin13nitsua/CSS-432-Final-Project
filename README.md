# CSS-432-Final-Project
Implementing a TFTP

## How to compile
To compile the Server navigate to the /Server directory and compile using the command "gcc -std=c11 -tftp_server.c -o tftp_server".

To compile the Client navigate to the /Client directory and compile using the command "gcc -std=c11 -tftp_client.c -o tftp_client".

## How to run
1. To run this project you must first run the server, navigate to the /Server directory and enter the command "./tftp_server"
2. Once the Server is running next you need to run the client, to do so navigate to the /Client directory and enter the command "./tftp_client -w filename.txt" where "filename.txt" is a stand in for the file you want to send. Currently only writing from the Client side is implemented in this project so the file that you wish to write to the Server's directory must exist in the /Client directory or else an error will occur.
3. As of last testing the Client can successfully write a small file to the Server directory (under 512 bytes), anything larger will go through the transfer sequence but currently does not write correctly.

Note: the "-w" argument does not functionally do anything and is a placeholder for future implementation so you could put any string in its place but something must go there to be able to read the filename argument correctly.

## Directory layout
/CSS-432-Final-Project
  /Client
    tftp_client.c
    tftp_client
  /Server
    tftp.server.c
    tftp_server
