
//Sample program at client side for echo transmit-receive - CSS 432 - Autumn 2022




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




#define SERV_UDP_PORT 514171 //REPLACE WITH YOUR PORT NUMBER
#define SERV_HOST_ADDR "127.0.0.1" //REPLACE WITH SERVER IP ADDRESS


/* A pointer to the name of this program for error reporting.      */

char *progname;

/* Size of maximum message to send.                                */

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
const static int DATA_OFFSET = 2;

/* Mode values. 												   */
const static char MODE_OCTET[]  = "octet";

/* dg_wrq opens a file, reads the contents and sends them to the   */
/* server address pointed at by pserv_addr and waits for an ack    */
/* message back from the server.                                   */                  

dg_wrq(sockfd, pserv_addr, servlen, filename)
int             sockfd;
struct sockaddr *pserv_addr;
int servlen;
char* filename;
{

/* Various counter and buffer variables. The extra byte in the     */
/* receive buffer is used to add a null to terminate the string,   */
/* as the network routines do not use nulls but I/O functions do.  */
/* Also declare the file object to be read from.				   */

	int     n;
	char    send_buffer[MAX_BUFFER_SIZE], recvline[MAX_BUFFER_SIZE + 1];
	bzero(send_buffer, sizeof(send_buffer));
	char receive_buffer[MAX_BUFFER_SIZE];
	bzero(receive_buffer, sizeof(receive_buffer));
	unsigned short op_code;
	unsigned short* op_code_ptr;
	char* filename_ptr;
	char* mode_ptr;
	unsigned short block_num;
	unsigned short* block_num_ptr;
	char* data_ptr;

	FILE* 	read_file;

/* Attempt to open the read file, if this cannot be done, return   */
/* immediately with error code -1, otherwise file is open and      */
/* ready for reading.											   */

	read_file = fopen(filename, "rb");

	if(read_file == NULL) {
		fprintf(stderr, "Error: File could not be opened.\n");
		return -1;
	}

/*	Initiate WRQ by sending a WRQ packet to the server			   */
	
	printf("Building WRQ packet\n");

	// Set the opcode portion of the WRQ packet
	op_code = OP_CODE_WRQ;
	op_code_ptr = (unsigned short*) send_buffer;
	*op_code_ptr = htons(op_code);

	// Set the filename portion of the WRQ packet
	filename_ptr = send_buffer + FILENAME_OFFSET;
	strcpy(filename_ptr, filename);


	// Set the mode portion of the WRQ packet
	mode_ptr = send_buffer + FILENAME_OFFSET + strlen(filename) + 1;
	strcpy(mode_ptr, MODE_OCTET);

	printf("WRQ packet built with opcode: %d, filename: %s, and mode: %s\n",
			ntohs(*op_code_ptr),filename_ptr,mode_ptr);

	// Send WRQ packet to Server
	if (sendto(sockfd, send_buffer, MAX_BUFFER_SIZE, 0, pserv_addr, servlen) != MAX_BUFFER_SIZE)
			{
			 printf("%s: sendto error on socket\n",progname);
			 exit(3);
			}

	printf("Sent WRQ packet\n");

	// Receive ACK packet from Server
	n = recvfrom(sockfd, receive_buffer, MAX_BUFFER_SIZE, 0, NULL, NULL);

	if(n < 0) {
		printf("%s: recvfrom error\n",progname);
		exit(3);
	}

	op_code_ptr = (unsigned short*) receive_buffer;
	op_code = ntohs(*op_code_ptr);

	block_num_ptr = (unsigned short*) receive_buffer + BLOCK_OFFSET;
	block_num = ntohs(*block_num_ptr);

	if(op_code == 4) {
		printf("ACK packet received with ");
		printf("opcode: %d, and ", op_code);
		printf("block #: %d\n", block_num);
	}

	// Make and send DATA packet
	bzero(send_buffer, sizeof(send_buffer));
	op_code = OP_CODE_DATA;
	op_code_ptr = (unsigned short*) send_buffer;
	*op_code_ptr = htons(op_code);

	block_num = 1;
	block_num_ptr = (unsigned short*) send_buffer + BLOCK_OFFSET;
	*block_num_ptr = htons(block_num);

	data_ptr = block_num_ptr + DATA_OFFSET;
	char* file_data;
	int file_length;

	fseek(read_file, 0, SEEK_END);
	file_length = ftell(read_file);
	rewind(read_file);

	file_data = (char*) malloc(file_length * sizeof(char));
	fread(file_data, file_length, 1, read_file);
	fclose(read_file);

	bcopy(file_data, data_ptr, file_length);
	free(file_data);
	
	printf("DATA packet built with opcode: %d, and block number: %d\n",
			ntohs(*op_code_ptr), ntohs(*block_num_ptr));
	printf("DATA in packet: ");
	for(int i = 0; i < file_length; i++) {
		printf("%c", data_ptr[i]);
	}
	printf("\n");
	
	// Send DATA packet to server
	if (sendto(sockfd, send_buffer, MAX_BUFFER_SIZE, 0, pserv_addr, servlen) != MAX_BUFFER_SIZE)
			{
			 printf("%s: sendto error on socket\n",progname);
			 exit(3);
			}

	printf("Sent WRQ packet\n");
}

/* The main program sets up the local socket for communication     */
/* and the server's address and port (well-known numbers) before   */
/* calling the dg_cli main loop.                                   */

main(argc, argv)
int     argc;
char    *argv[];
{	
	char* filename;

	int                     sockfd;
	
/* We need to set up two addresses, one for the client and one for */
/* the server.                                                     */
	
	struct sockaddr_in      cli_addr, serv_addr;
	progname = argv[0];

/* Initialize first the server's data with the well-known numbers. */

	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family      = AF_INET;
	
/* The system needs a 32 bit integer as an Internet address, so we */
/* use inet_addr to convert the dotted decimal notation to it.     */

	serv_addr.sin_addr.s_addr = inet_addr(SERV_HOST_ADDR);
	serv_addr.sin_port        = htons(SERV_UDP_PORT);

/* Create the socket for the client side.                          */
	
	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
	       {
		printf("%s: can't open datagram socket\n",progname);
		exit(1);
	       }

/* Initialize the structure holding the local address data to      */
/* bind to the socket.                                             */

	bzero((char *) &cli_addr, sizeof(cli_addr));
	cli_addr.sin_family      = AF_INET;
	
/* Let the system choose one of its addresses for you. You can     */
/* use a fixed address as for the server.                          */
       
	cli_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	
/* The client can also choose any port for itself (it is not       */
/* well-known). Using 0 here lets the system allocate any free     */
/* port to our program.                                            */


	cli_addr.sin_port        = htons(0);
	
/* The initialized address structure can be now associated with    */
/* the socket we created. Note that we use a different parameter   */
/* to exit() for each type of error, so that shell scripts calling */
/* this program can find out how it was terminated. The number is  */
/* up to you, the only convention is that 0 means success.         */

	if (bind(sockfd, (struct sockaddr *) &cli_addr, sizeof(cli_addr)) < 0)
		{
		 printf("%s: can't bind local address\n",progname);
		 exit(2);
		}

/* Call the main client loop. We need to pass the socket to use    */
/* on the local endpoint, and the server's data that we already    */
/* set up, so that communication can start from the client.        */
	filename = argv[2];
	dg_wrq(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr), filename);

/* We return here after the client sees the EOF and terminates.    */
/* We can now release the socket and exit normally.                */

	close(sockfd);
	exit(0);
}























