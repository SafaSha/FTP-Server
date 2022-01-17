/*
 * File: myftpd.c
 * Authors: KC and SS (adapted and extended from unit notes)
 * Date: 5/10/21
 * Version: 1
 *
 * This program implements the server side of simple FTP protocol
 * using TCP transport protocol.
 */


//NOTE: WE MUST LOG ALL SERVER/CLIENT SESSIONS (I.E. PRINT TO LOG FILE)

#include  <stdlib.h>   
#include  <stdio.h>    
#include  <string.h>     
#include  <errno.h>    
#include  <signal.h>     
#include  <syslog.h>
#include  <sys/types.h>  
#include  <sys/socket.h> 
#include  <sys/wait.h>   
#include  <netinet/in.h> 
#include <unistd.h>
#include <sys/stat.h>   
#include <dirent.h>         
#include <arpa/inet.h>
#include <netdb.h>             
#include <stddef.h>
#include <fcntl.h>
#include <strings.h>  
#include <syslog.h>
#include <time.h>  
#include <errno.h>

#include  "sftpd.h"   
  

#define   SERV_TCP_PORT   41115   //default server listening port
#define   MAX_BUF   256
#define BUFFER_SIZE 1024

//handler for catching zombie child processes 
void claim_children()
{
     pid_t pid=1;
   
     while(pid>0) 
     { 
         pid = waitpid(0, (int *)0, WNOHANG);  
     } 
}

//turn process into daemon
void daemon_init(void)
{       
     pid_t   pid;
     struct sigaction act;

     if((pid = fork()) < 0) 
     {
          perror("fork"); exit(1); 
     } 
     
     else if (pid > 0) 
     {
          printf("Hay, you'd better remember my PID: %d\n", pid);
          exit(0);                 
     }
     //child turns into daemon:
     setsid();                   
     chdir("/");                   
     umask(0);                    

     //set up daemon to handle zombie child signals:
     act.sa_handler = claim_children; 
     sigemptyset(&act.sa_mask);      
     act.sa_flags   = SA_NOCLDSTOP;   
     sigaction(SIGCHLD,(struct sigaction *)&act,(struct sigaction *)0);  
}


//work for server to do #1:
//change cwd of server
int changecwd(char *s)
{
	//s must have the directory path with no newline etc. chars on the end
	//does not currently support spaces between folder names
	//does support '.' and '..'
 	
	if(chdir(s)<0)
     	{
     		return -1;
     	}

     	return 0;
}

//work for server to do #2:
//get cwd contents
int getcwdcontent(char *s)
{
	//fills 's' with list of cwd contents, separated by newlines
	DIR *folder; //pointer to cwd
    	//make ptr to a dirent structure, has file i-node numbers and names:
    	struct dirent *entry; 

    	folder = opendir("."); //open cwd
    	if(folder == NULL) 
   	{
        	return -1; //error can't open cwd
    	}
    	//read through files:
    	else
    	{
		strcat(s,"\n");//put a space at start of entries
		//returns false if EOF reached:
    		while((entry=readdir(folder)))
    		{
    		
    		//Exclude . and .. from list      
        	if( (strcmp(entry->d_name, ".")) && (strcmp(entry->d_name, "..")) ) {
        	       		//put '\n' on ends of file names and add them all into char buffer s:
        		strcat(entry->d_name,"\n");
        		strcat(s,entry->d_name);   
        	
        	   }

   		}//end of while
    		closedir(folder);
    	}
    	
    	return 0;
}

//work for server to do #3:
//download file (txt or binary) from server cwd
int download(char *fname, char *file)
{
	struct stat fileStat;
	memset(file,0,sizeof(file));
	//size of file:
	stat(fname, &fileStat);
	int sizeOfFile = fileStat.st_size;
	int fd;
	fd = open(fname, O_RDONLY , 0777);
	//check to see if file can be opened:
	if(fd < 0) return (-1);

	//read file contents into file buffer:
	int n, nr;

	for (n=0; n<sizeOfFile; n += nr) 
	{
		if((nr = read(fd, file+n, sizeOfFile-n)) <= 0) {perror("Read to file server side: "); return (-2);}
		
	}

	close(fd);
  	return n;       
}

//work for server to do #4:
//get cwd path
int getcwdpath(char *s)
{
    //fill 's' with cwd path, else returns -1:
    if(getcwd(s, MAX_BLOCK_SIZE) == NULL)
    {
       return -1;
    }
    return 0;	
}

//deciphers which service is requested from the client and performs appropriate action
//uses sftp header to figure out what client wants
void serve_a_client(int sd,char ipv4addr[])
{   
    
    while(1)//keep serving client until client is finished
    {
    
    int nr;
    int nw;
    char rbuf[MAX_BLOCK_SIZE];
    char wbuf[MAX_BLOCK_SIZE];
    
    struct sftpheader *clientMsgHeader;
    clientMsgHeader = malloc(sizeof(struct sftpheader));
    initialise_header(clientMsgHeader);
    
    //read from client process:
    if((nr = readn(sd, rbuf, sizeof(rbuf), clientMsgHeader)) < 0) 
    {
         return;  //break connection for read error
    }

    //prepare return message:
         
    struct sftpheader *returnMsgHeader;
    returnMsgHeader = malloc(sizeof(struct sftpheader)); 
    initialise_header(returnMsgHeader);            
    returnMsgHeader->opcode = clientMsgHeader->opcode; 
             	    	
    switch(returnMsgHeader->opcode)
    {
         case 'C'://pwd(), client requests dir path of server
         {       
         	if ((log_client(ipv4addr, "Client attempted pwd()")) < 0) {perror("LOG_ERR: ");
       		//return (LOG_ERR);
       		}
		
         	int pathOK = getcwdpath(wbuf);

         	if(pathOK== -1)
         	{
         		returnMsgHeader->ackcode = '5';
         		returnMsgHeader->msglen = 0; //no payload
         		
         		//tell client server can't send dir path: 	  
        		if((nw = writen(sd, wbuf, 0, returnMsgHeader)) < 0) 
        		{
   				return;  //write error
        		}
        		
        		if ((log_client(ipv4addr, "Displaying current directoty path has been failed")) < 0) {perror("LOG_ERR: ");
       			//return (LOG_ERR);
       			}
        		
         	}
         	else
         	{
         		returnMsgHeader->ackcode = '0';
         		returnMsgHeader->msglen = (short)strlen(wbuf);
         		
         		//tell client server is ready for the file contents: 	  
        		if((nw = writen(sd, wbuf, strlen(wbuf), returnMsgHeader)) < 0) 
        		{
   				return;  //write error
        		}
        		
        		if ((log_client(ipv4addr, "Current directory path has been displayed successfully")) < 0) {perror("LOG_ERR: ");
       			//return (LOG_ERR);
       			}
        		
        		}
        		
        }
        break;
 			
 	case 'S'://put(), client wants to upload file to server cwd
 	{       
 		if ((log_client(ipv4addr, "Client attempted put command")) < 0) {perror("LOG_ERR: ");
       		//return (LOG_ERR);
       		}	
        	//create file in server cwd using filename read into rbuf:
	
		int nfd=open(rbuf, O_WRONLY | O_CREAT,0777);
		memset(rbuf,0,sizeof(rbuf));

         	if(nfd>0)
         	{
         		returnMsgHeader->ackcode = '0'; 
         		returnMsgHeader->msglen = 0; //no payload
         		
         		
         		//tell client server is ready for the file contents: 	  
        		if((nw = writen(sd, wbuf, 0, returnMsgHeader)) < 0) 
        		{
   				return;  //write error
        		}
 
         		//read in the file contents from client:
         		char file[MAX_BLOCK_SIZE];
         		struct sftpheader *fileHeader; 
         		fileHeader = malloc(sizeof(struct sftpheader)); 
         		initialise_header(fileHeader); 
   
         		int n, len;

    			//read the header:
    			int headOK = readHeader(sd, fileHeader); //this sets all header variables    

    			if(headOK<0) perror("header: ");//read error or incorrect header format
 
    			//read the message payload if required:
    			if(fileHeader->msglen > 0)
    			{   
 
 		   	len = fileHeader->msglen;  
   		 	//read len number of bytes to buf: 
    			for (n=0; n < len; n += nr) 
    			{
        			//if ((nr = read(sd, file+n, len-n)) <= 0) 
        			if ((nr = read(sd, file, len)) <= 0) 
        	{
            		perror("Reading from client socket: ");
            	}
            	
            	fflush(stdout);
            	write(nfd,file,fileHeader->msglen);//save contents locally:
    	}
	
    }
 		file[0]='\0';//empty array of char
        	//fclose(fPtr);
        	 close(nfd);  
         	fflush(stdout);	
         		
       			if ((log_client(ipv4addr, "File successfully transferred to server")) < 0) {perror("LOG_ERR: ");
       			//return (LOG_ERR);
       			}

        		free(fileHeader);
         	}
		else
		{
			returnMsgHeader->ackcode = '2'; //can't upload file
         		returnMsgHeader->msglen = 0; //no payload
			
			if ((log_client(ipv4addr, "File has not transferred to server")) < 0) {perror("LOG_ERR: ");
       			//return (LOG_ERR);
       			}
			
			//tell client server can't accept file contents: 	  
        		if((nw = writen(sd, wbuf, 0, returnMsgHeader)) < 0) 
        		{
   				return;  //write error
        		}       		
		}
		
	 }	
         break;  
         	        	
         case 'R'://client request to download file from server cwd
         {
         	if ((log_client(ipv4addr, "Client attempted get()")) < 0) {perror("LOG_ERR: ");
         	//return (LOG_ERR);
         	}
         	
         	char file[MAX_BLOCK_SIZE];  
         	struct stat fileStat;
		//size of file:
		stat(rbuf, &fileStat);
		int sizeOfFile = fileStat.st_size;
		int fdread = open(rbuf, O_RDONLY , 0777);
		memset(rbuf,0,sizeof(rbuf));

		int n, nr, nw;
		char *buffer = malloc(sizeof(char) * sizeOfFile);

		for (n=0; n<sizeOfFile; n += nr) {
			if((nr = read(fdread, buffer+n, sizeOfFile-n)) <= 0) {perror("Read: "); return;}
		}

		fflush(stdout);
		returnMsgHeader->ackcode = '0';        		
         	returnMsgHeader->msglen = (short)sizeOfFile;
 	        int headOK = writeHeader(sd, returnMsgHeader);
		for (n=0; n<sizeOfFile; n += nw) {
			if ((nw = write(sd, buffer+n, sizeOfFile-n)) <= 0) {perror("write: ");return;}
		}
		
		if ((log_client(ipv4addr, "Flie transfered to the client")) < 0) {perror("LOG_ERR: ");
         	//return (LOG_ERR);
         	}

	free(buffer);
	fflush(stdout);
	close(fdread);

	  }
         break;
                      
         case 'U'://client request to change cwd path
         {      
              	if ((log_client(ipv4addr, "Client attempted cd()")) < 0) {perror("LOG_ERR: ");
         	//return (LOG_ERR);
         	}
	
         	int changeOK = changecwd(rbuf);//the dir path in rbuf must have no newline on the end!
         	if(changeOK== -1)
         	{
         		returnMsgHeader->ackcode = '6';
         		returnMsgHeader->msglen = 0; //no payload
         		
         		//tell client server couldn't change current dir: 	  
        		if((nw = writen(sd, wbuf, 0, returnMsgHeader)) < 0) 
        		{
   				return;  //write error
        		}
        		
        		if ((log_client(ipv4addr, "Changing directory has been failed")) < 0) {perror("LOG_ERR: ");
         		//return (LOG_ERR);
         		}
         	}
         	else
         	{
         		returnMsgHeader->ackcode = '0';
         		returnMsgHeader->msglen = 0; //no payload
         		
         		//tell client server is ready for the file contents: 	  
        		if((nw = writen(sd, wbuf, 0, returnMsgHeader)) < 0) 
        		{
   				return;  //write error
        		}
        		if ((log_client(ipv4addr, "Current directory has changed successfully")) < 0) {perror("LOG_ERR: ");
         			//return (LOG_ERR);
         		}
         	}
         }       
         break;
         
         case 'F'://client request to display cwd contents
 	 {        
 	        if ((log_client(ipv4addr, "Client attempted dir()")) < 0) {perror("LOG_ERR: ");
         	//return (LOG_ERR);
         	}		
         	int displayOK = getcwdcontent(wbuf);

         	if(displayOK== -1)
         	{
         		returnMsgHeader->ackcode = '7';
         		returnMsgHeader->msglen = 0; //no payload
         		
         		//tell client server couldn't get current dir contents: 	  
        		if((nw = writen(sd, wbuf, 0, returnMsgHeader)) < 0) 
        		{
   				return;  //write error
        		}
        		if ((log_client(ipv4addr, "server couldn't get current dir contents")) < 0) {perror("LOG_ERR: ");
         		//return (LOG_ERR);
         		}
         	}
         	else
         	{
         		returnMsgHeader->ackcode = '0';
         		returnMsgHeader->msglen = (short)strlen(wbuf);
         		
         		//return cwd contents and header: 	  
        		if((nw = writen(sd, wbuf, strlen(wbuf), returnMsgHeader)) < 0) 
        		{
   				return;  //write error
        		}
        		if ((log_client(ipv4addr, "Current dir contents has been displayed successfully")) < 0) {perror("LOG_ERR: ");
         		//return (LOG_ERR);
         		}
         	}
         }                         
         break;    
        
        	
        default: 
         return; //cannot resolve opcode
         break;      		

      }
      
      free(clientMsgHeader);
      free(returnMsgHeader);
      
      }

}

//this should be called with ./myftpd <initial cwd>
//if no cwd entered by user uses the default directory
int main(int argc, char *argv[])
{
     int sd;
     int nsd;
     int n;  
     pid_t pid;
     unsigned short port = SERV_TCP_PORT;
     socklen_t cli_addrlen;  
     struct sockaddr_in ser_addr, cli_addr;    
     char parentcwd[MAX_BLOCK_SIZE];
	
     //set the cwd when server starts:
     if (argc == 1) 
     { 
     /*
        //create home directory if it doesn't already exist:
     	mkdir("Server_Default", 0777);//r,w,x permission for all    
     	if(chdir("Server_Default")<0)
     	{
     		perror("Can't find home directory\n"); 
     		exit(1);
     	}
     	*/
     	
     	//set parent cwd path variable:
     	int ok = getcwdpath(parentcwd);
     	if(ok<0)
     	{
     		perror("Can't find home directory\n"); 
     		exit(1);
     	}
     	
     }
     else if (argc == 2) 
     {
         char dirName[MAX_BUF];
         strcpy(dirName, argv[1]);         
         //check the directory at path entered by the user exists:       
	 if(chdir(dirName)<0)
     	 {
     		perror("Can't find the directory\n"); 
     		exit(1);
     	 }
     } 
     else 
     {
          perror("Too many cmd line args\n"); 
     	  exit(1);
     }
     
     //turn process into daemon:
     daemon_init(); 


	// log server startup to log file
	FILE *file;
	if((file = fopen(LOG_FILE, "a+")) < 0){
		perror("log file");
		return 1;
	}
	time_t now = time(NULL);
	char currentPath[256];
	if(getcwd(currentPath, sizeof(currentPath)) == NULL)
	{
		perror("main - getcwd");
		return 2;
	}
	fprintf(file, "Server started in directory %s at %s", currentPath, ctime(&now));
	fclose(file);

     //set up listening socket that uses TCP:
     if((sd = socket(PF_INET, SOCK_STREAM, 0)) < 0) 
     {
           perror("server:socket"); 
           exit(1);
     } 

     //construct the server internet address:
     bzero((char *)&ser_addr, sizeof(ser_addr));
     ser_addr.sin_family = AF_INET;
     ser_addr.sin_port = htons(port);
     ser_addr.sin_addr.s_addr = htonl(INADDR_ANY);//accept client on any network interface

     //bind server address to socket sd
     if (bind(sd, (struct sockaddr *) &ser_addr, sizeof(ser_addr))<0)
     {
           perror("server bind"); 
           exit(1);
     }

     //set socket to listen, set queue limit to 5:
     listen(sd, 5);


	//int nsd; // child socket
	socklen_t cli_addr_len; // hold the length of client address length
	struct sockaddr_in clientAddress; // hold the client address struct
	char client_ip[INET_ADDRSTRLEN]; // hold client ipv4 address



     //set up children to serve clients:
     while(1) 
     {
          //wait to connect to a client:
          cli_addrlen = sizeof(cli_addr);
          nsd = accept(sd, (struct sockaddr *) &cli_addr, &cli_addrlen);
          if (nsd < 0) 
          {
               if (errno == EINTR)//only keep going if intercepted signal is for child dying
                    continue;
               perror("server:accept"); exit(1);
          }
          		else{
			printf("client accepted!\n");

/* storing client ip address in */				
			struct sockaddr_in* cli_ipv4 = (struct sockaddr_in*)&cli_addr;
			struct in_addr ipAddr = cli_ipv4->sin_addr;
			
			inet_ntop( AF_INET, &ipAddr, client_ip, INET_ADDRSTRLEN );
			printf("client IPv4 address is: %s\n", client_ip);
		}
          
          
          
          
          
          
          

          //create child:
          if((pid=fork()) < 0) 
          {
              perror("fork"); 
              exit(1);
          } 
          else if (pid > 0) //in parent
          { 
              close(nsd);//close connection 
              continue; //wait for next client 
          }
          
	  //get child to navigate to parent cwd:
	  int ok = changecwd(parentcwd);
	  if(ok<0)
     	  {
     		perror("Can't find home directory\n"); 
     		exit(1);
     	  }
     	  
          //child to serve the current connected client and then die:
          close(sd); 
          
          // log client connected
	  char *msg = "connected";
	  if((log_client(client_ip, msg)) < 0)
	  {
		puts("Error writing to log file - client disconnect.");
	  }
          
          serve_a_client(nsd,client_ip);
          exit(0);
     }
}
