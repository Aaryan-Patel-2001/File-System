# File-System
This project implements File-System on a x86_64 architecture using C programming, originally developed on a Linux Enviornment. 
File System supports open, close, read and write operations. 
Functionality is tested by running unit tests on several workloads 
A fully associative cache improves the response time by 87% for some of the workloads.
Note: mdadm.c implements the File System, cache.c implements cache for the file system. 
Jbod.h, mdadm.h, and cache.h contains definitions and declarations of some of the functions and variables used in the implementation files. 
