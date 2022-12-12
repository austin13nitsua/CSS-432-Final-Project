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



// Hardcoded port number and Server IP address
#define SERV_UDP_PORT 514171 
#define SERV_HOST_ADDR "127.0.0.1" 

char *progname;

// Maximum packet size
#define MAX_BUFFER_SIZE 516

// Opcode definitions
const static unsigned short OP_CODE_RRQ 	= 1;
const static unsigned short OP_CODE_WRQ 	= 2;
const static unsigned short OP_CODE_DATA 	= 3;
const static unsigned short OP_CODE_ACK 	= 4;
const static unsigned short OP_CODE_ERROR 	= 5;

// Offset values
const static int FILENAME_OFFSET = 2;
const static int BLOCK_OFFSET = 2;
const static int DATA_OFFSET = 2;

// Mode values
const static char MODE_OCTET[]  = "octet";

// Routine which handles TFTP protocol exchange for a write request
// (sending a file from Client directory to Server directory)
dg_wrq(sockfd, pserv_addr, servlen, filename)
int             sockfd;
struct sockaddr *pserv_addr;
int servlen;
char* filename;
{
	// Variable declarations
	int n;											// Used in recvfrom calls to know packet size
	char send_buffer[MAX_BUFFER_SIZE];				// Byte array used to send packets
	bzero(send_buffer, sizeof(send_buffer));		
	char receive_buffer[MAX_BUFFER_SIZE];			// Byte array used to receive packets
	bzero(receive_buffer, sizeof(receive_buffer));	
	unsigned short op_code;							// Variable used to store the op_code number for packets
	unsigned short* op_code_ptr;					// Pointer to the op_code bytes in a buffer
	char* filename_ptr;								// Pointer to the file name bytes in a buffer
	char* mode_ptr;									// Pointer to the mode bytes in a buffer
	unsigned short block_num;						// Variable used to store block number for transfer sequencing
	unsigned short* block_num_ptr;					// Pointer to the block number bytes in a buffer
	char* data_ptr;									// Pointer to the data section in a buffer
	FILE* 	read_file;								// File object used to read a file in the Client directory

// Attempt to open the read file, if this cannot be done, return immediately 
// with error code -1, otherwise file is open and ready for reading											   */
	read_file = fopen(filename, "rb");
	if(read_file == NULL) {
		fprintf(stderr, "Error: File could not be opened.\n");
		return -1;
	}

// Initiate WRQ by sending a WRQ packet to the server
	//printf("Building WRQ packet\n");

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

	//printf("WRQ packet built with opcode: %d, filename: %s, and mode: %s\n",
			//ntohs(*op_code_ptr),filename_ptr,mode_ptr);

	// Send WRQ packet to Server
	if (sendto(sockfd, send_buffer, MAX_BUFFER_SIZE, 0, pserv_addr, servlen) != MAX_BUFFER_SIZE)
			{
			 printf("%s: sendto error on socket\n",progname);
			 exit(3);
			}

	printf("Sent WRQ packet\n");

	// Receive ACK packet back from Server
	n = recvfrom(sockfd, receive_buffer, MAX_BUFFER_SIZE, 0, NULL, NULL);

	if(n < 0) {
		printf("%s: recvfrom error\n",progname);
		exit(3);
	}

	// Decode op code
	op_code_ptr = (unsigned short*) receive_buffer;
	op_code = ntohs(*op_code_ptr);

	// Decode block number
	block_num_ptr = (unsigned short*) receive_buffer + BLOCK_OFFSET;
	block_num = ntohs(*block_num_ptr);

	/*
	if(op_code == 4) {
		printf("ACK packet received with ");
		printf("opcode: %d, and ", op_code);
		printf("block #: %d\n", block_num);
	}
	*/

	// Make and send DATA packet
	bzero(send_buffer, sizeof(send_buffer));

	// Set op code
	op_code = OP_CODE_DATA;
	op_code_ptr = (unsigned short*) send_buffer;
	*op_code_ptr = htons(op_code);

	// Set block number
	block_num = 1;
	block_num_ptr = (unsigned short*) send_buffer + BLOCK_OFFSET;
	*block_num_ptr = htons(block_num);

	// Set data pointer
	data_ptr = block_num_ptr + DATA_OFFSET;
	char* file_data;
	int file_length;

	// Obtain file information and read file
	fseek(read_file, 0, SEEK_END);
	file_length = ftell(read_file);
	rewind(read_file);
	printf("file_length: %d\n", file_length);

	file_data = (char*) malloc(file_length * sizeof(char));
	fread(file_data, file_length, 1, read_file);
	fclose(read_file);

	// Copy file data into DATA packet
	bcopy(file_data, data_ptr, file_length);
	free(file_data);
	
	/*
	printf("DATA packet built with opcode: %d, and block number: %d\n",
			ntohs(*op_code_ptr), ntohs(*block_num_ptr));
	printf("DATA in packet: ");
	for(int i = 0; i < file_length; i++) {
		printf("%c", data_ptr[i]);
	}
	printf("\n");
	*/
	
	// Send DATA packet to server
	int buffer_size = 4 + file_length;
	//int buffer_size = 0 + sizeof(*op_code_ptr) + sizeof(*block_num_ptr) + file_length + 4;
	if (sendto(sockfd, send_buffer, buffer_size, 0, pserv_addr, servlen) != buffer_size)
			{
			 printf("%s: sendto error on socket\n",progname);
			 exit(3);
			}

	printf("Sent WRQ packet\n");
}

// Main routine which sets up socket and calls another routine to handle
// the requested operation
main(argc, argv)
int     argc;
char    *argv[];
{	
	char* filename;
	int sockfd;
	struct sockaddr_in      cli_addr, serv_addr;
	progname = argv[0];
	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family      = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr(SERV_HOST_ADDR);
	serv_addr.sin_port        = htons(SERV_UDP_PORT);

	// Create Client-side socket
	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		printf("%s: can't open datagram socket\n",progname);
		exit(1);
	}
	bzero((char *) &cli_addr, sizeof(cli_addr));
	cli_addr.sin_family      = AF_INET;
	cli_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	cli_addr.sin_port        = htons(0);
	
	// Bind the socket to the address
	if (bind(sockfd, (struct sockaddr *) &cli_addr, sizeof(cli_addr)) < 0)
		{
		 printf("%s: can't bind local address\n",progname);
		 exit(2);
		}

	// Filename is the second command line argument
	filename = argv[2];

	// Call WRQ routine to handle the request
	dg_wrq(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr), filename);

	// Return here after the called routine terminates, close the socket and exit
	close(sockfd);
	exit(0);
}























