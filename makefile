# makefile 
# the filename must be either Makefile or makefile
# call this by make clean to get rid of object files, then call make <name>
# or just make and if makes the object files and links them again

myftp: myftp.o sftp.o
	gcc myftp.o sftp.o -o myftp
	
myftpd: myftpd.o sftp.o
	gcc myftpd.o sftp.o -o myftpd

myftpd.o: myftpd.c sftp.h
	gcc -c myftpd.c

myftp.o: myftp.c sftp.h
	gcc -c myftp.c 

sftp.o: sftp.c sftp.h
	gcc -c sftp.c

clean: 
	rm *.o

