#ifndef AWGET_H
#define AWGET_H

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <stdio.h>
#include <iostream>
#include <string>
#include <cstring>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <limits.h>
#include <netdb.h>
#include <fstream>
#include <vector>
#include <boost/algorithm/string.hpp>
#include <ctime>

#define DEFAULT_CHAINFILE "chaingang.txt"
#define MAX_CHARS_PER_LINE 512

struct packet_header {
    short urlLength;
    short chainListLength;
};

struct packet {
    char* url;
    char* chainList;
};

struct file_packet_header {
    unsigned short seqNum;
    unsigned short lastPacket;
    unsigned short dataLength;
};

struct file_packet {
    char* fileData;
};

#endif
