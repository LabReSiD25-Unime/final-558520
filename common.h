#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <dirent.h>
#include <sys/stat.h>
#include <time.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <sys/select.h>

#define BUFFER_SIZE 1024
#define PORT 21
#define ROOT_DIR "./ftp_root"

#endif