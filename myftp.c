/*
 * File: myftp.c
 * Authors: KC and SS (adapted and extended from unit notes)
 * Date: 5/10/21
 * Version: 1
 *
 * This program implements the client side of simple FTP protocol
 * using TCP transport protocol.
 */

#include  <stdlib.h>
#include  <stdio.h>
#include  <sys/types.h>        
#include  <sys/socket.h>
#include  <sys/stat.h>
#include  <netinet/in.h>       
#include  <netdb.h>           
#include  <string.h>
#include  <unistd.h>
#include  <dirent.h>
#include <arpa/inet.h>
#include <netdb.h>             
#include <stddef.h>
#include <fcntl.h>
#include <strings.h>
#include  "sftp.h"           

#define   SERV_TCP_PORT  41115 

//pwd command
void pwd(int sd)
{
	//write request to server:
	
	char buf[MAX_BLOCK_SIZE]; 
	int nw, nr;
	struct sftpheader *wheader;
	wheader= malloc(sizeof(struct sftpheader));
	initialise_header(wheader);
	wheader->opcode = 'C';
	  	  
        if((nw = writen(sd, buf, 0, wheader)) < 0) //no msg payload - buf size = 0 
        {
            printf("client: send error\n"); 
            exit(1); 
        }
	free(wheader);
	
	//read server response:
	
        struct sftpheader *returnHeader;
	returnHeader= malloc(sizeof(struct sftpheader));
	initialise_header(returnHeader);
		
        if((nr = readn(sd, buf, MAX_BLOCK_SIZE, returnHeader)) <0) 
        {
            printf("client: receive pwd error\n"); 
            exit(1);
        }  
        
        if(returnHeader->ackcode=='0')
        {       	
        	buf[nr] = '\0';
        	printf("%s\n", buf); 
        }
        else
        {
        	printf("Couldn't find server pwd\n");
        }
        
        free(returnHeader);  
}

//lpwd command
void lpwd(char *s)
{
    if(getcwd(s, MAX_BLOCK_SIZE) == NULL)
    {
       s = "Can't find current working directory\n";
    }	
}

//dir command
void dir(int sd)
{
	//write request to server:
	
	char buf[MAX_BLOCK_SIZE]; 
	int nw, nr;
	struct sftpheader *wheader;
	wheader = malloc(sizeof(struct sftpheader));
	initialise_header(wheader);
	wheader->opcode = 'F';
	 	  
        if((nw = writen(sd, buf, 0, wheader)) < 0) //no msg payload - buf size = 0 
        {
            printf("client: send error\n"); 
            exit(1); 
        }
	free(wheader);
	
	//read server response:
	
        struct sftpheader *returnHeader;
	returnHeader = malloc(sizeof(struct sftpheader));
	initialise_header(returnHeader);

        if((nr = readn(sd, buf, MAX_BLOCK_SIZE, returnHeader)) < 0) 
        {
            printf("client: receive dir error\n"); 
            exit(1);
        }

  	if(returnHeader->ackcode=='0')
        {       	
        	buf[nr] = '\0'; 
        	printf("%s\n", buf); 
        	memset(buf, 0, sizeof buf);
        	
        	
        }
        else
        {
        	printf("Couldn't find server directory contents\n");
        }        
        free(returnHeader);
}

//ldir command
void ldir(char *s)
{    	
	memset(s,0,sizeof(s));	 //to ensure nothing in array   
      	DIR *folder; //pointer to cwd
    	//make ptr to a dirent structure, has file i-node numbers and names:
    	struct dirent *entry; 

    	folder = opendir("."); //open cwd
    	if(folder == NULL) 
   	{
        	s = "Unable to read directory\n"; 
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
   		}
    		closedir(folder);
    	}
}

//change server cwd - cd <path> supports '.' '..' notation:
void cd(char *s, int sd)
{
	//write request to server:
	
	int pathlen = strlen(s) - 3;
        s+=3; //remove first 3 chars ('cd ') from buff by adding 3 to pointer	
	int nw, nr;
	struct sftpheader *wheader;
	wheader = malloc(sizeof(struct sftpheader));
	initialise_header(wheader);
	wheader->opcode = 'U';
	wheader->msglen = (short)pathlen;
	 	  
        if((nw = writen(sd, s, pathlen, wheader)) < 0) 
        {
            printf("client: send error\n"); 
            exit(1); 
        }
	free(wheader);
	s-=3;//return pointer position
	     	
	//read server response:
	
	char buf[MAX_BLOCK_SIZE]; 
        struct sftpheader *returnHeader;
	returnHeader = malloc(sizeof(struct sftpheader));
	initialise_header(returnHeader);

        if((nr = readn(sd, buf, MAX_BLOCK_SIZE, returnHeader)) < 0) 
        {
            printf("client: receive cd error\n"); 
            exit(1);
        }
  	if(returnHeader->ackcode=='0')
        {       	
        	//all good
        }
        else
        {
        	printf("Couldn't change server directory\n");
        }        
        free(returnHeader);
  	
}

//lcd <path> supports '.' '..' notation:
//note this does not support folders with spaces in the name
void lcd(char *s)
{
        //remove first 4 chars ('lcd ') from buff by adding 4 to pointer (I know I shouldn't):
        s+=4;
 	
	if(chdir(s)<0)
     	{
     		perror("Can't find directory\n"); 
     	}
     	
     	s-=4;//return pointer position	
}

//get command (file has to be in the server's cwd, or have a valid relative or full path):
void get(char *filename, int sd)
{
	//write request to server:
	
	int fnamelen = strlen(filename) - 4;
        filename+=4; //remove first 4 chars ('get ') from filename by adding 4 to pointer	
	int nw, nr;
	struct sftpheader *wheader;
	wheader = malloc(sizeof(struct sftpheader));
	initialise_header(wheader);
	wheader->opcode = 'R';
	wheader->msglen = (short)fnamelen;
	 	  
        if((nw = writen(sd, filename, fnamelen, wheader)) < 0) 
        {
            printf("client: send error\n"); 
            exit(1); 
        }
	free(wheader);
	     	
	//read server response:
	
	char file[MAX_BLOCK_SIZE]; 
        struct sftpheader *returnHeader;
	returnHeader = malloc(sizeof(struct sftpheader));
	initialise_header(returnHeader);

        if((nr = readn(sd, file, MAX_BLOCK_SIZE, returnHeader)) < 0) 
        {
            printf("client: receive download error\n"); 
            exit(1);
        }
       fflush(stdout);
  	if(returnHeader->ackcode=='0')
        {       	
        	//save the file in local cwd:
        	//FILE * fPtr;
        	//fPtr = fopen(filename, "w");
        	int nfd=open(filename, O_WRONLY | O_CREAT,0777);
        	if(nfd <0)
    		{
        		printf("Unable to create file locally.\n");
        		exit(1);
   		}
 		
 		write(nfd,file,returnHeader->msglen);
 	        fflush(stdout);
 		file[0]='\0';//empty array of char
        	 close(nfd);  
        }
        else
        {
        	printf("Couldn't download file\n");
        } 
            
        free(returnHeader);
  	filename-=4;//return pointer position
}

//put command - upload file to server cwd:
void put(char *filename, int sd)
{	
	//write request to server:
	
	int fnamelen = strlen(filename) - 4;
        filename+=4; //remove first 4 chars ('put ') from filename by adding 4 to pointer	
	int nw, nr;
	struct sftpheader *wheader;
	wheader = malloc(sizeof(struct sftpheader));
	initialise_header(wheader);
	wheader->opcode = 'S';
	wheader->msglen = (short)fnamelen;
	 	  
        if((nw = writen(sd, filename, fnamelen, wheader)) < 0) 
        {
            printf("client: send error\n"); 
            exit(1); 
        }
	free(wheader);
	     	
	//read server response:
	char resp[MAX_BLOCK_SIZE];
        struct sftpheader *returnHeader;
	returnHeader = malloc(sizeof(struct sftpheader));
	initialise_header(returnHeader);

        if((nr = readn(sd, resp, MAX_BLOCK_SIZE, returnHeader)) < 0) 
        {
            printf("client: receive uplaod error\n"); 
            exit(1);
        }
   
  	if(returnHeader->ackcode=='0')
        {       	
        	//send the file contents:
        	struct stat fileStat;
		char file[MAX_BLOCK_SIZE]; 
		
		//size of file:
		stat(filename, &fileStat);
		int sizeOfFile = fileStat.st_size;
 
		//read file contents to file buffer:
        	int fdc=open(filename, O_RDONLY ,0777);
        	
        	//if(fPtr == NULL)
    		if(fdc<=0)
    		{
        		printf("Unable to open file\n");
        		exit(1);
   		}
 		read(fdc,file,sizeOfFile);
 		
 		//send file contents:
 		struct sftpheader *fheader;
		fheader = malloc(sizeof(struct sftpheader));
		initialise_header(fheader);
		fheader->opcode = 'B';
		fheader->msglen = sizeOfFile;
	 	  
        	if((nw = writen(sd, file, sizeOfFile, fheader)) < 0) 
        	{
            		printf("client: send error\n"); 
            		exit(1); 
        	}

		memset(file,0,sizeof(file));
		free(fheader);
        	close(fdc);
        }
        else
        {
        	printf("Cannot upload file to server\n");
        }        
        free(returnHeader);
  	filename-=4;//return pointer position
}


//call with ./myftp on command line
int main(int argc, char *argv[])
{
     int sd, n, nr, nw, i=0;
     char buf[MAX_BLOCK_SIZE]; 
     char host[60];
     unsigned short port;
     struct sockaddr_in ser_addr; 
     struct hostent *hp;	  
     char msg[MAX_BLOCK_SIZE];//to hold messages

     //get server host name and port number: 
     
     //use default server port and ip
     if (argc==1) 
     { 
          gethostname(host, sizeof(host));
          //above copies the hostname into the char array 'host'
          port = SERV_TCP_PORT;
     } 
     // use the given host name for server
     else if (argc == 2) 
     { 
          strcpy(host, argv[1]);
          port = SERV_TCP_PORT;
     } 
     // use given host and port for server
     else if (argc == 3) 
     { 
         strcpy(host, argv[1]);
         int n = atoi(argv[2]);
         if (n >= 1024 && n < 65536) 
         {
             port = n;
         }
         else 
         {
             printf("Error: server port number must be between 1024 and 65535\n");
             exit(1);
         }
     } 
     //too many cmd args entered
     else 
     { 
         printf("Too many command line arguments. Terminating.\n"); 
         exit(1); 
     }

     // get host address & build a server socket address: 
     bzero((char *) &ser_addr, sizeof(ser_addr));
     ser_addr.sin_family = AF_INET;
     ser_addr.sin_port = htons(port);//convert address to Network Byte Order 
     if((hp = gethostbyname(host)) == NULL)
     {
           printf("host %s not found\n", host); 
           exit(1);   
     }
     ser_addr.sin_addr.s_addr = * (u_long *) hp->h_addr;

     //create TCP socket & connect socket to server address: 
     sd = socket(PF_INET, SOCK_STREAM, 0);
     
     if(connect(sd, (struct sockaddr *) &ser_addr, sizeof(ser_addr))<0) 
     { 
          perror("client connect"); 
          exit(1);
     }
    
     while (++i) //forever until quit
     {
          printf("Available Commands:\n\n1- pwd\n2- lpwd\n3- dir\n4- ldir\n5- cd <directory_pathname>\n6- lcd <directory_pathname>");
	  printf("\n7- get <filename>\n8- put <filename>\n9- quit\n");	

          //display prompt and get user input:
          printf("\n> ");
          fgets(buf, sizeof(buf), stdin); 
          nr = strlen(buf);      
          //swap newline for null char on input:
          if (buf[nr-1] == '\n') { buf[nr-1] = '\0'; --nr; }
	   
	  //find out what user wants:
	  char *strs[9] = {"quit", "pwd", "lpwd","dir","ldir","cd ", "lcd ", "get ", "put "};
	  for(int index=0; index<9; index++)
	  {
	  	//for requests not involving paths or files:
	  	if(strcmp(buf, strs[index])==0) 
          	{
                  switch (index)
                  {
               	case 0:
               	 printf("Session Terminated.\n"); 
               	 exit(0);
               	
               	case 1:
               	 pwd(sd);
               	 break;
               	 
               	case 2:
               	 lpwd(msg);              	 
               	 printf("%s\n",msg);               	              	
               	 break;
               	 
               	case 3:
               	 dir(sd);
               	 break;
               	 
               	case 4:
               	 ldir(msg);
               	 printf("%s\n",msg);
               
               	 memset(msg, 0, sizeof msg);
               	 break;  
               	
               	default:
               	 break;   
                  }              	            	
          	}
          	//for lcd, get or put:
          	if(strncmp(buf, strs[index],4)==0)
          	{
          	   switch (index)
                  {
               	case 6:
               	 lcd(buf);
               	 memset(buf,0,sizeof(buf));
               	 break;
               	
               	case 7:
               	 get(buf, sd);
               	 memset(buf,0,sizeof(buf));
               	 break;
               	 
               	case 8:
               	 put(buf, sd);
               	 memset(buf,0,sizeof(buf));
               	 break;
               	 
                      default:
               	 break;               	              	
                  }            
          	}
          	//for cd:
		if(strncmp(buf, strs[index],3)==0 && index==5)
		{
		   cd(buf, sd);	
		   memset(buf,0,sizeof(buf));	  
		}
		
	  }//end for each string in array
	  
	  //if the command isn't recognised program will just display prompt again 
     }

}
