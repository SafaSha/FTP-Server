/*
 * File: sftp.h
 * Authors: KC and SS (adapted from unit notes)
 * Date: 5/10/21
 * Version: 1
 *
 * This program defines a simple file transfer protocol.
 */


#define MAX_BLOCK_SIZE (1024*20)    //total maximum number of bytes for any communication

struct sftpheader {
  short headerlen;  		//number of bytes in sftp header 
  char opcode;		        //char opcode for what type of correspondance required
  char ackcode;		//opcode for acknowledgement of a message
  short msglen; 		//number of bytes in message payload
  				//payload could be length of the filename/names OR
  				//length of the directory path OR
   				//length of the file for upload or download
};


/*
 * purpose:  read a stream of bytes from "fd" to "buf" and set struct sftpheader variables
 * pre:      1) size of buf bufsize = MAX_BLOCK_SIZE
 * post:     1) buf contains the byte stream message without header, or null if the header was sent without a message payload
 *           2) bufsize is number of bytes in msg (may be zero)
 *           3) header has at least header size and opcode variables set, optionally will have the ackcode and msglen variables set
 *           4) return value >  0  : number of bytes read total
 *                           =  0  : connection closed
 *                           = -1  : header could not be resolved
 *                           = -2  : read error
 *                           = -3  : buffer too small
 */           
int readn(int fd, char *buf, int bufsize, struct sftpheader *header);


/*
 * purpose:  write "nbytes" bytes including sftp header from "buf" to "fd" 
 * pre:      1) nbytes <= MAX_BLOCK_SIZE
 *           2) struct sftp header must have at least header size and opcode set, optionally 
 *              ackcode and/or msglen also set
 * post:     1) nbytes bytes from buf written to fd total
 *           2) return value = nbytes : number ofbytes written
 *                           = -2     : header incorrect format
 *                           = -4     : write error
 *                           = -3     : too many bytes to send 
 */           
int writen(int fd, char *buf, int nbytes, struct sftpheader *header);


/*
 * purpose:  send an sftp header 
 * pre:      1) fd is socket file descriptor, struct header contains set header variables (minimum must have opcode set, optionally can have ackcode and msglen set)
 * post:     1) return value >= 0   : ok      
 *                           = -1  : header format incorrect  
 *                           = -2  : write error 
 */  
int writeHeader(int fd, struct sftpheader *header);


/*
 * purpose:  Read an sftp header returned from byte stream 
 * pre:      1) fd is socket file descriptor to read from
 * post:     1) struct header contains set variables from byte stream
 *           2) return value = 0   : ok
 *                           = -1  : could not resolve header    
 *                           = -2  : read error     
 */  
int readHeader(int fd, struct sftpheader *header);

/*
 * purpose:  Initialise a sftp header struct 
 * pre:      1) enough memory is allocated for the struct
 * post:     1) opcode and ackcode set to null, message length = -1,headerlength=0
 */  
void initialise_header(struct sftpheader *header);

#define LOG_FILE "/tmp/client_log"

/*
 * Purpose: Create a log in /tmp/client_log with a client ip address specified by ipv4addr
 *		 and a message specified by msg
 * Return:
 *	1) -1 if /tmp/client_log could not be opened or,
 *	2) 1 if entry was logged successfully
 */
int log_client(char * ipv4addr, char *msg);



