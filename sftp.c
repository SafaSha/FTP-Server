/*
 * File: sftp.c
 * Authors: KC and SS (adapted and extended from unit notes)
 * Date: 5/10/21
 * Version: 1
 *
 * This program implements a simple file transfer protocol
 */

#include  <sys/types.h>
#include  <netinet/in.h> // struct sockaddr_in, htons(), htonl(), 
#include  <unistd.h>
#include  <stdio.h>
#include  <stdlib.h>
#include  <string.h>
#include  "sftp.h"

int readn(int fd, char *buf, int bufsize, struct sftpheader *header)
{
    int n, nr, len,b;
    memset(buf,0, sizeof(buf));
    //check buffer size:
    if (bufsize < MAX_BLOCK_SIZE){return (-3);} //buffer too small 
    
    //read the header:
    int headOK = readHeader(fd, header); //this sets all header variables   
 
    if(headOK<0){return headOK;}//read error or incorrect header format
    	
    //read the message payload if required:
    if(header->msglen > 0)
    {      
    	len = header->msglen;  
    	//char *tuf=malloc(sizeof(char)*len);  
    	//if(tuf==NULL)printf("shit\n");
    	//read len number of bytes to buf: 
    	for (n=0; n < len; n += nr) 
    	{  	
        	if ((nr = read(fd, buf+n, len-n)) <= 0) 
        	//if ((nr = read(fd, buf, len)) <= 0)
        	{
            		return (-2);//read error
            	}
    	fflush(stdout);
    	}
    }
    else
    {
    	if((b=read(fd, buf, 1))<=0) {
    	perror("read from socket: ");}
    }

    if(header->msglen<0){return 0;} //no payload
    else{return (len); } //size of payload
}

int writen(int fd, char *buf, int nbytes, struct sftpheader *header)
{
    int n, nw;

    if (nbytes > MAX_BLOCK_SIZE) 
    {
         return (-3);    // too many bytes to send in one go 
    }

    //send the sftp header:
    int headOK = writeHeader(fd, header);	
    if(headOK<0) {return headOK; }//header incorrect format or write error
	
    //send the nbytes msg payload if required:	
    if(header->msglen > 0)
    {	
    	for(n=0; n<nbytes; n += nw) 
    	{
         	if((nw = write(fd, buf+n, nbytes-n)) <= 0)  
         	{
         		return (-2);//write error
         	}
    	}
    }
    
    return (n); //returns number of payload bytes
}

/**send an sftp header 
The first 2 bytes of buf are always the size of the header and the third byte is always the opcode, if these are not set in struct header param, return error.
A header may optionally have a byte for an acknowledgement code and/or 2 bytes for the msg payload length. 
*/
int writeHeader(int fd, struct sftpheader *header)
{ 
   //first perform checks on the header struct to ensure it is in correct format, also sets header length

   //switch uses the opcode in the struct header param:
   switch (header->opcode)
   {
	case 'S'://prep to upload file
	  	if(header->ackcode=='0')//prepared to receive file
	  	{
	  		header->msglen = -1; 
	  		header->headerlen=4;    
	  	}
	  	else if(header->ackcode=='1' || header->ackcode=='2')//can't receive file
	  	{	  		
	  		header->msglen = -1;  
	  		header->headerlen=4;  
	  	}	  			
	  	else
	  	{
	  		if(header->msglen <= 0){return -1;} //need to know size of filename!
			header->headerlen=5;    
			header->ackcode=='\0';	
		}
	  	break;
	
	case 'B'://upload: file attached 	
		if(header->msglen <= 0){return -1;} //need to know size of file!   		
		header->headerlen= 5;
		header->ackcode == '\0';
	  	break;
	
	case 'R'://download: file attached
	  	if(header->ackcode=='0')//file is attached
	  	{
	  		if(header->msglen <= 0){return -1;} //need to know size of file!  
	  		header->headerlen=6;
	  	}
	  	else if(header->ackcode=='3' || header->ackcode=='4')//can't send file
	  	{	  		
	  		header->msglen = -1;    
	  		header->headerlen=4;
	  	}	  			
	  	else
	  	{
	  		if(header->msglen <= 0){return -1;} //need to know size of filename!   	
			header->headerlen=5;	
			header->ackcode=='\0';
	  	}	  
	  	break;	

	
	case 'C'://get server cwd path
	  	if(header->ackcode=='0')//can send dir path
	  	{
	  		if(header->msglen <= 0){return -1;} //need to know size of dir path!   
	  		header->headerlen=6;
	  	}
	  	else if(header->ackcode=='5')//can't send dir path
	  	{	  		
	  		header->msglen = -1;  
	  		header->headerlen=4;  	  		
	  	}
	  	else
	  	{
	  		header->msglen = -1;    	
    			header->headerlen=3;	
    			header->ackcode=='\0';
	  	}
	  	break;
	
	case 'U'://change server cwd   			
	  	if(header->ackcode=='0')//dir has been changed
	  	{
	  		header->msglen = -1;  
	  		header->headerlen=4;  	
	  	}
	  	else if(header->ackcode=='6')//can't change dir
	  	{	  		
	  		header->msglen = -1;  
	  		header->headerlen=4;  	  		
	  	}
	  	else
	  	{
	  		if(header->msglen <= 0){return -1;} //need to know size of dir path!  
	  		header->headerlen=5;	
	  		header->ackcode == '\0';	
	  	}	  			
	  	break;
	
	case 'F'://get server cwd contents
	  	if(header->ackcode=='0')//cwd contents attached
	  	{
			if(header->msglen <= 0){return -1;} //need to know size of dir contents!  
	  		header->headerlen=6;	
	  	}
	  	else if(header->ackcode=='7')//can't send cwd contents
	  	{	  		
	  		header->msglen = -1;  
	  		header->headerlen=4;  	  		
	  	}
	  	else
	  	{
	  		header->msglen = -1;    	
    			header->headerlen=3;	
    			header->ackcode == '\0';
	  	}	
	  	break;
	
	case '\0'://opcode not set 
	  	return -1;
	  	break;
	  	  		
	default:
	  	return -1;
	  	break;
   }
      	//printf("writing.... opcode is '%c', ackcode is '%c', message length is '%d', header length is '%d'",  header->opcode, header->ackcode, header->msglen, header->headerlen);
   	//send the 2 byte header length: 
	short data_size = header->headerlen;
    	data_size = htons(data_size);  //converts to network byte order
    	//send the 2-bytes in data_size variable:
    	if (write(fd, (char *) &data_size, 1) != 1) return (-2);      
    	if (write(fd, (char *) (&data_size)+1, 1) != 1) return (-2);     
		
    	//send the 1 byte opcode:
    	if (write(fd, (char *) &header->opcode, 1) != 1) return (-2);      
		
    	//send the 1 byte ackcode if required:
    	if(header->ackcode != '\0')
    	{
    		if (write(fd, (char *) &header->ackcode, 1) != 1) return (-2);      
    	}

    	//send the 2 byte msg length if required:
    	if(header->msglen > 0 && header->msglen<=MAX_BLOCK_SIZE)
    	{
    		short msg_size = header->msglen;
    		msg_size = htons(msg_size);  //converts to network byte order
    		//send the 2-bytes in msg_size variable:
    		if (write(fd, (char *) &msg_size, 1) != 1) return (-2);      
    		if (write(fd, (char *) (&msg_size)+1, 1) != 1) return (-2);        	
    	}

	return 0;
}

/**
Read an sftp header returned from byte stream 
Must fill out variables in header struct using the bytestream.
The first 2 bytes of buf are always the size of the header and the third byte is always the opcode, if this is not the case, return error.
There may be a byte in header stream for an acknowledgement code and/or 2 bytes for the msg length.
*/
int readHeader(int fd, struct sftpheader *header)
{
    header->msglen = -1;
    int setACK=0;
    int setMSG=0;
    
    //Read header size:
    short header_data_size; 
    if (read(fd, (char *) &header_data_size, 1) != 1) return (-2);
    if (read(fd, (char *) (&header_data_size)+1, 1) != 1) return (-2);
    header->headerlen = ntohs(header_data_size); //convert to host byte order 
    
    //Read the opcode in header i.e. read in third byte sent:
    if (read(fd, (char *) &header->opcode, 1) != 1) return (-2); 
           
   //Perform switch on opcode to determine whether to read the optional header contents:
   switch (header->opcode)
   {
	case 'S'://prep to upload file
	  	if (header->headerlen== 5)//client request
		{		
			setMSG=1;
		}
		else if(header->headerlen==4)//server return message
	  	{
			setACK=1;	  		  		
	  	}
		else
	  	{
	  		return -1; //incorrect header format
	  	}	  	
	  	break;
	
	case 'B'://upload: file attached 
		if (header->msglen == 5)
		{	
			setMSG=1;	
		}
		else
	  	{	  		
	  		return -1; //incorrect header format	  		
	  	}
	  	break;
	
	case 'R'://download: file attached
		if (header->headerlen==5)//client request
		{		
			setMSG=1;	
		}
	  	else if(header->headerlen==6)//server can return file and it is attached
	  	{	  			
			setACK=1;	
			setMSG=1;						
	  	}
	  	else if(header->headerlen==4) //server can't send file
	  	{	  		
			setACK=1;	  			
	  	}	  			
	  	else
	  	{
	  		return -1; //incorrect header format
	  	}	  
	  	break;
	
	case 'C'://get server cwd path
		if (header->headerlen==3)//client request
		{		
			return 0;
		}
	  	else if(header->headerlen==6)//server can return dir path and it is attached
	  	{	  			
			setACK=1;	
			setMSG=1;							
	  	}
	  	else if(header->headerlen==4) //server can't send dir path
	  	{	  		
	 		setACK=1;			 			
	  	}	  			
	  	else
	  	{
	  		return -1; //incorrect header format
	  	}	  
	  	break;
	
	case 'U'://change server cwd 
		if (header->headerlen==5)//client request
	  	{	  		
		  	setMSG=1;	
	  	}
		else if(header->headerlen==4) //server return response
	  	{	  		
	 		setACK=1;			 			
	  	}	  	
		else
	  	{
	  		return -1; //incorrect header format
	  	}	  	
	  	break;
	
	case 'F'://get server cwd contents
		if (header->headerlen==3)//client request
	  	{	  		
   			return 0;				  	
	  	}
		else if(header->headerlen==6)//server can return dir contents and it is attached
	  	{	  			
			setACK=1;	
			setMSG=1;							
	  	}
	  	else if(header->headerlen==4) //server can't send dir contents
	  	{	  		
	 		setACK=1;			 			
	  	}	  			
	  	else
	  	{
	  		return -1; //incorrect header format
	  	}	  	
	  	break;

	case '\0'://opcode not set
	  	return -1;
	  	break;
	  		
	default:
	  	return -1;
	  	break;
   }
   
   if(setACK==1)
   {
      	//read in and set the header ackcode variable:
	if (read(fd, (char *) &header->ackcode, 1) != 1) return (-2); 
   }
   
   if(setMSG==1)
   {
   	//read in and set the header msg length variable:
	short data_size; 
    	if (read(fd, (char *) &data_size, 1) != 1) return (-3);
    	if (read(fd, (char *) (&data_size)+1, 1) != 1) return (-4);
    	header->msglen= ntohs(data_size); //convert to host byte order 
   }	
 //  printf("reading.... opcode is '%c', ackcode is '%c', message length is '%d', header length is '%d'",  header->opcode, header->ackcode, header->msglen, header->headerlen);  		
	return 0;
}

void initialise_header(struct sftpheader *header)
{
	header->opcode = '\0';
	header->ackcode = '\0';
	header->msglen = -1;
	header->headerlen = 0;
}

