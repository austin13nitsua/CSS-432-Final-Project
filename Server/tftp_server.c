//for system calls, please refer to the MAN pages help in Linux 
//sample echo tensmit receive program over udp - CSS432 - Autumn 2022
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <errno.h>          // for retrieving the error number.
#include <string.h>         // for strerror function.
#include <signal.h>         // for the signal handler registration.
#include <unistd.h>

#define SERV_UDP_PORT   514171 // REPLACE WITH YOUR PORT NUMBER


char *progname;

/* Size of maximum message to received.                            */

#define MAX_BUFFER_SIZE 516

/* Opcode definitions.											   */

const static unsigned short OP_CODE_RRQ 	= 1;
const static unsigned short OP_CODE_WRQ 	= 2;
const static unsigned short OP_CODE_DATA 	= 3;
const static unsigned short OP_CODE_ACK 	= 4;
const static unsigned short OP_CODE_ERROR 	= 5;

/* Offset values.												   */
const static int FILENAME_OFFSET = 2;
const static int BLOCK_OFFSET = 2;

/* Mode values. 												   */
const static char MODE_OCTET[]  = "octet";

/* The dg_echo function receives data from the already initialized */
/* socket sockfd and returns them to the sender.                   */

dg_echo(sockfd)
int            sockfd;
{
/* struct sockaddr is a general purpose data structure that holds  */
/* information about a socket that can use a variety of protocols. */
/* Here, we use Internet family protocols and UDP datagram ports.  */
/* This structure receives the client's address whenever a         */
/* datagram arrives, so it needs no initialization.                */
	
	struct sockaddr pcli_addr;
	
/* Temporary variables, counters and buffers.                      */

	int    n, clilen;
	char receive_buffer[MAX_BUFFER_SIZE];
	bzero(receive_buffer, sizeof(receive_buffer));
	char send_buffer[MAX_BUFFER_SIZE];
	bzero(send_buffer, sizeof(send_buffer));
	unsigned short op_code;
	unsigned short* op_code_ptr;
	char* filename_ptr;
	char* filename;
	char* mode_ptr;
	char* mode;
	unsigned short block_num;
	unsigned short* block_num_ptr;

/* Main echo server loop. Note that it never terminates, as there  */
/* is no way for UDP to know when the data are finished.           */

	for ( ; ; ) {

/* Initialize the maximum size of the structure holding the        */
/* client's address.                                               */

		clilen = sizeof(struct sockaddr);

/* Receive data on socket sockfd, up to a maximum of MAXMESG       */
/* bytes, and store them in mesg. The sender's address is stored   */
/* in pcli_addr and the structure's size is stored in clilen.      */
		
		n = recvfrom(sockfd, receive_buffer, MAX_BUFFER_SIZE, 0, &pcli_addr, &clilen);
		
/* n holds now the number of received bytes, or a negative number  */
/* to show an error condition. Notice how we use progname to label */
/* the source of the error.                                        */

		if (n < 0)
			{
			 printf("%s: recvfrom error\n",progname);
			 exit(3);
			}

		op_code_ptr = (unsigned short*) receive_buffer;
		op_code = ntohs(*op_code_ptr);

		filename_ptr = receive_buffer + FILENAME_OFFSET;
		strcpy(filename, filename_ptr);

		mode_ptr = receive_buffer + FILENAME_OFFSET + strlen(filename_ptr) + 1;
		printf("mode: %s\n", mode_ptr);		

/* Logic for when WRQ is received. 								  */

		if(op_code == 2) {
			// Create file in Server directory to write to
			printf("WRQ Packet received\n");
			FILE* write_file;
			write_file = fopen(filename, "w");

			// Check if file was created succesfully
			if(write_file == NULL) {
				fprintf(stderr, "Error: File could not be opened.\n");
				return -1;
			}

			printf("File %s created, ready for writing\n", filename);

			// Send first ACK packet back to Client

			// Set the opcode portion of the ACK packet
			printf("Building ACK packet\n");
			op_code = OP_CODE_ACK;
			op_code_ptr = (unsigned short*) send_buffer;
			*op_code_ptr = htons(op_code);
			
			// Set the Block Number portion of the ACK packet
			block_num = 0;
			block_num_ptr = (unsigned short*) send_buffer + BLOCK_OFFSET;
			*block_num_ptr = htons(block_num);

			printf("ACK packet built with opcode: %d, and block #: %d\n", 
					ntohs(*op_code_ptr), ntohs(*block_num_ptr));

			if (sendto(sockfd, send_buffer, MAX_BUFFER_SIZE, 0, &pcli_addr, clilen) != MAX_BUFFER_SIZE) {
				printf("%s: sendto error\n",progname);
			 	exit(4);
			}

			printf("Sent ACK packet\n");
		}

		return 0;
	}
}

/* Main driver program. Initializes server's socket and calls the  */
/* dg_echo function that never terminates.                         */

main(argc, argv)
int     argc;
char    *argv[];
{
	
/* General purpose socket structures are accessed using an         */
/* integer handle.                                                 */
	
	int                     sockfd;
	
/* The Internet specific address structure. We must cast this into */
/* a general purpose address structure when setting up the socket. */

	struct sockaddr_in      serv_addr;

/* argv[0] holds the program's name. We use this to label error    */
/* reports.                                                        */

	progname=argv[0];

/* Create a UDP socket (an Internet datagram socket). AF_INET      */
/* means Internet protocols and SOCK_DGRAM means UDP. 0 is an      */
/* unused flag byte. A negative value is returned on error.        */

	if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
		{
		 printf("%s: can't open datagram socket\n",progname);
		 exit(1); 
		}

/* Abnormal termination using the exit call may return a specific  */
/* integer error code to distinguish among different errors.       */
		
/* To use the socket created, we must assign to it a local IP      */
/* address and a UDP port number, so that the client can send data */
/* to it. To do this, we fisrt prepare a sockaddr structure.       */

/* The bzero function initializes the whole structure to zeroes.   */
	
	bzero((char *) &serv_addr, sizeof(serv_addr));
	
/* As sockaddr is a general purpose structure, we must declare     */
/* what type of address it holds.                                  */
	
	serv_addr.sin_family      = AF_INET;
	
/* If the server has multiple interfaces, it can accept calls from */
/* any of them. Instead of using one of the server's addresses,    */
/* we use INADDR_ANY to say that we will accept calls on any of    */
/* the server's addresses. Note that we have to convert the host   */
/* data representation to the network data representation.         */

	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

/* We must use a specific port for our server for the client to    */
/* send data to (a well-known port).                               */

	serv_addr.sin_port        = htons(SERV_UDP_PORT);

/* We initialize the socket pointed to by sockfd by binding to it  */
/* the address and port information from serv_addr. Note that we   */
/* must pass a general purpose structure rather than an Internet   */
/* specific one to the bind call and also pass its size. A         */
/* negative return value means an error occured.                   */

	if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
	       { 
		printf("%s: can't bind local address\n",progname);
		exit(2);
	       }

/* We can now start the echo server's main loop. We only pass the  */
/* local socket to dg_echo, as the client's data are included in   */
/* all received datagrams.                                         */

	dg_echo(sockfd);

/* The echo function in this example never terminates, so this     */
/* code should be unreachable.                                     */

}
