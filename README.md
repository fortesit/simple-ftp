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
```
airman2:~ Sit$ ./shell
[3150 shell:/Users/Sit]$ mkdir test
[3150 shell:/Users/Sit]$ cd test
[3150 shell:/Users/Sit/test]$ touch new_file
[3150 shell:/Users/Sit/test]$ cat | cat >> new_file
Hello
^Z
[3150 shell:/Users/Sit/test]$

[3150 shell:/Users/Sit/test]$ ping -c 2 8.8.8.8 | cat >> new_file
[3150 shell:/Users/Sit/test]$ jobs
[1]: cat | cat >> new_file
[3150 shell:/Users/Sit/test]$ fg 1
Job wake up: cat | cat >> new_file
World
^C[3150 shell:/Users/Sit/test]$ cat new_file
Hello
PING 8.8.8.8 (8.8.8.8): 56 data bytes
64 bytes from 8.8.8.8: icmp_seq=0 ttl=49 time=32.598 ms
64 bytes from 8.8.8.8: icmp_seq=1 ttl=49 time=31.467 ms
 
--- 8.8.8.8 ping statistics ---
2 packets transmitted, 2 packets received, 0.0% packet loss
round-trip min/avg/max/stddev = 31.467/32.032/32.598/0.566 ms
 
World
[3150 shell:/Users/Sit/test]$ exit
airman2:~ Sit$
```
