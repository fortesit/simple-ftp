#Simple FTP

##Information
Version: 1.0

GitHub repository: https://github.com/fortesit/simple-ftp

Author: Sit King Lok

Last modified: 2014-09-30 22:11

 
##Description
 A simple FTP with the following features:
 1. List files (i.e. ls)
 2. Download file (i.e. get [FILENAME])
 3. Upload file (i.e. put [FILENAME])
 4. User login (i.e. auth [USER] [PASSWORD])
 5. Multi-thread(Multi-user) support
 6. Multi-platform support

##Required files:
 MakeFile

 myftp.h

 myftpclient.c

 myftpserver.c

 access.txt

##Usage(Server)
```
 mkdir filedir
 make all
 ./server_{linux|unix} [PORT]
```

##Usage(Client)
```
 make all
 ./client_{linux|unix}
```

##Platform
Linux(e.g.Ubuntu)/SunOS

##Note
 You need to have a password file (access.txt) in order to login.

 The sample file provided contain the test account.

 User:alice

 Password:pass1
 
##Example
On server
```
sit@sit-laptop:~/Desktop/simple-ftp$ mkdir filedirsit@sit-laptop:~/Desktop/simple-ftp$ make allgcc -o client_linux myftpclient.cgcc -D Linux -o server_linux myftpserver.c -lpthreadsit@sit-laptop:~/Desktop/simple-ftp$ ./server_linux 2525

```

On client
```
sit@sit-laptop:~/Desktop/simple-ftp$ echo "Hello World" > file.txtsit@sit-laptop:~/Desktop/simple-ftp$ ./client_linux Client> open 127.0.0.1 2525Server connection accepted.Client> auth alice pass1Authentication granted.Client> ls---- file list start ----ObjC.pdf---- file list end ----Client> put file.txtFile uploaded.Client> get ObjC.pdfFile downloaded.Client> quitThank yousit@sit-laptop:~/Desktop/simple-ftp$ ls ObjC.pdfObjC.pdfsit@sit-laptop:~/Desktop/simple-ftp$ cat filedir/file.txt Hello World

```

