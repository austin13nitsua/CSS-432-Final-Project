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

// Hardcoded unique port number
#define SERV_UDP_PORT   514171 


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

// Routine which handles TFTP protocol exchange between server and client
dg_client_rq(sockfd)
int            sockfd;
{	
	struct sockaddr pcli_addr;
	
	// Variable declarations
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
	char* data_ptr;


	// Main loop which handles data exchange
	for ( ; ; ) {
		clilen = sizeof(struct sockaddr);		
		n = recvfrom(sockfd, receive_buffer, MAX_BUFFER_SIZE, 0, &pcli_addr, &clilen);
		
		if (n < 0) {
			 printf("%s: recvfrom error\n",progname);
			 exit(3);
		}

		op_code_ptr = (unsigned short*) receive_buffer;
		op_code = ntohs(*op_code_ptr);

		filename_ptr = receive_buffer + FILENAME_OFFSET;
		strcpy(filename, filename_ptr);

		mode_ptr = receive_buffer + FILENAME_OFFSET + strlen(filename_ptr) + 1;
		//printf("mode: %s\n", mode_ptr);		

		// Logic for when WRQ is received. 								  
		if(op_code == 2) {
			// Create file in Server directory to write to
			printf("WRQ Packet received\n");
			FILE* write_file;
			write_file = fopen(filename, "wb");

			// Check if file was created succesfully
			if(write_file == NULL) {
				fprintf(stderr, "Error: File could not be opened.\n");
				return -1;
			}

			printf("File %s created, ready for writing\n", filename);

			// Initialize block number to be used in loop
			block_num = 0;

			// Loop to receive DATA packets and send back ACKs
			while(1) {
				// Start building ACK packet
				// Set the opcode portion of the ACK packet
				op_code = OP_CODE_ACK;
				op_code_ptr = (unsigned short*) send_buffer;
				*op_code_ptr = htons(op_code);
				
				// Set the block number portion of the ACK packet
				block_num_ptr = (unsigned short*) send_buffer + BLOCK_OFFSET;
				*block_num_ptr = htons(block_num);

				//printf("ACK packet built with opcode: %d, and block #: %d\n", 
						//ntohs(*op_code_ptr), ntohs(*block_num_ptr));

				// Send ACK packet to Client
				if (sendto(sockfd, send_buffer, MAX_BUFFER_SIZE, 0, &pcli_addr, clilen) != MAX_BUFFER_SIZE) {
					printf("%s: sendto error\n",progname);
					exit(4);
				}
				printf("Sent ACK #%d\n", block_num);

				// Receive DATA packet from Client
				n = recvfrom(sockfd, receive_buffer, MAX_BUFFER_SIZE, 0, &pcli_addr, &clilen);
				if (n < 0) {
					printf("%s: recvfrom error\n",progname);
					exit(3);
				}

				// Decode op code from received packet
				op_code_ptr = (unsigned short*) receive_buffer;
				op_code = ntohs(*op_code_ptr);

				// Decode block number from received packet
				block_num_ptr = (unsigned short*) receive_buffer + BLOCK_OFFSET;
				block_num = ntohs(*block_num_ptr);
				printf("Received DATA #%d\n", block_num);
				
				// Write data from packet to local file
				data_ptr = block_num_ptr + DATA_OFFSET;
				fwrite(data_ptr, 1, n, write_file);
				
				/* PRINT OUT DATA
				printf("DATA in packet: ");
				for(int i = 0; i < n; i++) {
					printf("%c", data_ptr[i]);
				}
				*/
				//printf("\n");
				//printf("DATA Packet received with opcode: %d, and block #: %d\n", op_code, block_num);
				
				// If DATA packet size is less than 516 bytes then commence end of transfer
				if(n < 516) {
					// Set the opcode portion of the ACK packet
					op_code = OP_CODE_ACK;
					op_code_ptr = (unsigned short*) send_buffer;
					*op_code_ptr = htons(op_code);
                    
					// Set the block number portion of the ACK packet
					block_num_ptr = (unsigned short*) send_buffer + BLOCK_OFFSET;
					*block_num_ptr = htons(block_num);

					// Send ACK packet to Client
					if (sendto(sockfd, send_buffer, MAX_BUFFER_SIZE, 0, &pcli_addr, clilen) != MAX_BUFFER_SIZE) {
						printf("%s: sendto error\n",progname);
						exit(4);
					}
					printf("Sent ACK #%d\n", block_num);
					printf("File transfer complete!\n");
					fclose(write_file);
					return 0;
				}
				block_num++;
			}
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
	int                     sockfd;
	struct sockaddr_in      serv_addr;
	progname=argv[0];

	if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
		{
		 printf("%s: can't open datagram socket\n",progname);
		 exit(1); 
		}
	
	bzero((char *) &serv_addr, sizeof(serv_addr));	
	serv_addr.sin_family      = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port        = htons(SERV_UDP_PORT);

	if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
	       { 
		printf("%s: can't bind local address\n",progname);
		exit(2);
	       }

	dg_client_rq(sockfd);
}
