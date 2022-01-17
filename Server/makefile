# makefile 
# the filename must be either Makefile or makefile
# call this by make clean to get rid of object files, then call make <name>
# or just make and if makes the object files and links them again

	
myftpd: myftpd.o sftpd.o
	gcc myftpd.o sftpd.o -o myftpd

myftpd.o: myftpd.c sftpd.h
	gcc -c myftpd.c


sftp.o: sftpd.c sftpd.h
	gcc -c sftpd.c

clean: 
	rm *.o

