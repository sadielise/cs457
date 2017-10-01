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
#include <sstream>
#include <signal.h>
#include <thread>

using namespace std;
using namespace boost;

#define DEFAULT_PORT 55556

#define DEBUG 0

struct packet_header {
    unsigned short urlLength;
    unsigned short chainListLength;
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

struct packet* deserializePacket(char* buf, int urlLength, int chainListLength) {
    struct packet* request_packet = (struct packet*) malloc(sizeof(struct packet));
    request_packet->url = new char[urlLength];
    request_packet->chainList = new char[chainListLength];
    
    for(int i = 0; i < urlLength; i++) {
        request_packet->url[i] = buf[i];
    }
    for(int i = urlLength; i < (urlLength + chainListLength); i++) {
        request_packet->chainList[i - urlLength] = buf[i];
    }
    
    return request_packet;
}

int stripIPAndPort(struct packet* p, string ip) {
    string chainListString = "";
    char* chainList = p->chainList;
    
    for(char* i = chainList; *i; i++) { chainListString += *i; }
    
    vector<string> machines;
    split(machines, chainListString, is_any_of(","));
    
    for(unsigned int j = 0; j < machines.size(); j++) {
        size_t found = machines.at(j).find(ip);
        if(found != string::npos) {
            machines.erase(machines.begin() + j);
            break;
        }
    }
    
    string newChainList = "";
    
    for(unsigned int j = 0; j < machines.size(); j++) {
        newChainList += machines.at(j);
        if(j < machines.size() - 1) { newChainList += ","; }
    }
    
    p->chainList = new char[newChainList.size()];
    strcpy(p->chainList, newChainList.c_str());
    
    return machines.size();
}

int chooseRandomNumber(int max){
    srand(time(NULL));
    int num = rand() % max;
    return num;
}

string getIP(struct packet* p, int location){
    vector<string> strings;
    split(strings, p->chainList, is_any_of(",:"));
    string IP = strings[location * 2].c_str();
    return IP;
}
		
int getPort(struct packet* p, int location){
    vector<string> strings;
    split(strings, p->chainList, is_any_of(",:"));
    int port = atoi(strings[((location * 2) + 1)].c_str());
    return port;
}

string getFileName(char* url){
    string fileName = "";
    bool found = false;
    unsigned int i = strlen(url) - 1;
    
    while((i > 0) && (found == false)) {
        if(url[i] == '/') { found = true; }
        else { i--; }
    }
    
    if(found == true){
        i++;
        while(i < strlen(url)) {
            fileName += url[i];
            i++;
        }
    } else {
        fileName = "index.html";
    }
    
    return fileName;
}		

void printPacketInfo(struct packet_header* header_packet, struct packet* request_packet) {
    cout << "URL Length: " << header_packet->urlLength << endl;
    cout << "URL: " << request_packet->url << endl;
    cout << "ChainList Length: " << header_packet->chainListLength << endl;
    cout << "ChainList: " << request_packet->chainList << endl;
}

int printHelpMessage() {
	cout << endl << "ss help:" << endl << endl;
	cout << "-p: port number to listen on" << endl;
	exit(EXIT_FAILURE);
}

char* getHostIP(int port){
    struct sockaddr_in addr;
  	struct hostent *host;
    char* ip = (char*) malloc((sizeof(char))* INET_ADDRSTRLEN);
    char hostname[HOST_NAME_MAX];
    
    if(gethostname(hostname, HOST_NAME_MAX) != 0) {
        cout << "Error: Could not find hostname" << endl;
        exit(EXIT_FAILURE);
    }
    
    host = gethostbyname(hostname);
    
    if(!host) {
        cout << "Error: Could not find host" << endl;
        exit(EXIT_FAILURE);
    }
    
    bzero((char *)&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    bcopy(host->h_addr, (char *)&addr.sin_addr, host->h_length);
    inet_ntop(AF_INET, &addr.sin_addr.s_addr, ip, INET_ADDRSTRLEN);

    return ip;
}

int bindAndListen(int port){
    struct sockaddr_in addr;
  	struct hostent *host;
    char ip[INET_ADDRSTRLEN];
    char hostname[HOST_NAME_MAX];
    int sock;
    
    if(gethostname(hostname, HOST_NAME_MAX) != 0) {
        cout << "Error: Could not find hostname" << endl;
        exit(EXIT_FAILURE);
    }
    
    host = gethostbyname(hostname);
    
    if(!host) {
        cout << "Error: Could not find host" << endl;
        exit(EXIT_FAILURE);
    }
    
    bzero((char *)&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    bcopy(host->h_addr, (char *)&addr.sin_addr, host->h_length);
    inet_ntop(AF_INET, &addr.sin_addr.s_addr, ip, INET_ADDRSTRLEN);
    
    // 2. prints out hostname and port 
    cout << "SS " << ip << ":" << port << endl;
      
    if(DEBUG) { cout << "About to create and bind socket" << endl; }
    
    // 3. create and bind socket
    if((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        cout << "Error: Could not create socket" << endl;
        exit(EXIT_FAILURE);
    }
    if(bind(sock, (struct sockaddr*) &addr, sizeof(addr)) < 0) {
        cout << "Error: Could not bind socket" << endl;
        exit(EXIT_FAILURE);
    }
    
    if(DEBUG) { cout << "Socket created and binded" << endl; }
    
    // 4. listen on socket
    listen(sock, 1);
    
    if(DEBUG) { cout << "listening on socket..." << endl; }

    return sock;
}

struct packet_header* receiveRequestPacketHeader(int sock){
    // receive packet header
    struct packet_header* header_packet = (struct packet_header*) malloc(sizeof(struct packet_header));
    recv(sock, reinterpret_cast<char*>(header_packet), sizeof((*header_packet)), 0);
    
    if(DEBUG) { 
        cout << "received packet header" << endl;
        cout << "URL Length: " << header_packet->urlLength << endl;
        cout << "ChainList Length: " << header_packet->chainListLength << endl;
    }
    
    return header_packet;
}

struct packet* receiveRequestPacket(int sock, struct packet_header* header_packet){
    // recv packet data
    char* buf = (char*) malloc((sizeof(char)) * (header_packet->urlLength + header_packet->chainListLength));

    recv(sock, buf, ((sizeof(char)) * (header_packet->urlLength + header_packet->chainListLength)), 0);
    
    if(DEBUG) {
        cout << "Received: ";
        for(int i = 0; i < (header_packet->urlLength + header_packet->chainListLength); i++) {
            cout << buf[i];
        }
        cout << endl;
    }

    // receive packet
    struct packet* request_packet = deserializePacket(buf, header_packet->urlLength, header_packet->chainListLength);
    
    cout << "Request: " << request_packet->url << endl;
    cout << "chainlist is" << endl;
    cout << request_packet->chainList << endl;
    
    return request_packet;
}

void handleEmptyChainList(struct packet_header* header_packet, struct packet* request_packet, int prev_sock){
    cout << "Chainlist is empty. Issuing wget for file: " << request_packet->url << endl;
    
    // 1. the ss uses the system call system() to isue a wget to retrieve the file specified in the URL
    char wgetCommand[header_packet->urlLength + 4];
    strcpy(wgetCommand, "wget ");
    strcat(wgetCommand, request_packet->url);
    
    if(DEBUG) {
        cout << "Command: ";
        for(unsigned int i = 0; i < strlen(wgetCommand); i++) {
            cout << wgetCommand[i];
        }
        cout << endl;
    }
    
    system(wgetCommand);
    
    // 2. Reads the file in small chunks and transmits it back to the previous SS. The previous SS also receives the file in chunks
    string fileName = getFileName(request_packet->url);
    if(DEBUG) { cout << "File Name: " << fileName << endl; }
    
    cout << "Received file: " << fileName << endl;

    FILE *file_ptr;
    file_ptr = fopen(fileName.c_str(), "rb");
    
    if(!file_ptr) {
        cout << "Could not read file: " << fileName << endl;
        exit(EXIT_FAILURE);
    }
    
    if(DEBUG) {
        fseek(file_ptr, 0L, SEEK_END);
        size_t sz = ftell(file_ptr);
        cout << "File Size: " << sz << endl;
        rewind(file_ptr);
    }
    
    cout << "Relaying file..." << endl;
    
    size_t byteCount;
    char fileData[60000];
    int seqNum = -1;
    while((byteCount = fread(fileData, 1, sizeof(fileData), file_ptr)) > 0) {
        seqNum++;
        
        if(DEBUG) { cout << "Byte Count: " << byteCount << endl; }
        
        // create file packet header
        struct file_packet_header* f_packet_header = (struct file_packet_header*) malloc(sizeof(struct file_packet_header));			
        f_packet_header->seqNum = seqNum;
        f_packet_header->dataLength = byteCount;
        if(feof(file_ptr) || byteCount < 60000) { f_packet_header->lastPacket = 1; }
        else { f_packet_header->lastPacket = 0; }
        
        if(DEBUG) {
            cout << "Sending Data Packet to Last SS:" << endl;
            cout << "SeqNum: " << f_packet_header->seqNum << endl;
            cout << "last pack? " << f_packet_header->lastPacket << endl;
            cout << "data length: " << f_packet_header->dataLength << endl;
        }
        
        // convert to network byte order
        f_packet_header->seqNum = htons(f_packet_header->seqNum);
        f_packet_header->dataLength = htons(f_packet_header->dataLength);
        f_packet_header->lastPacket = htons(f_packet_header->lastPacket);
        
        // send packet header
        send(prev_sock, reinterpret_cast<const char*>(f_packet_header), sizeof(*f_packet_header), 0);
        
        // send packet data
        send(prev_sock, fileData, sizeof(fileData), 0);
    }
    
    fclose(file_ptr);
        
    // 3. Once the file is completely transmitted, the ss should tear down the connection
    close(prev_sock);
    
    cout << "Done. Goodbye!" << endl;
    
    // 4. Erase the local copy and go back to listening for more requests
    char rmCommand[3 + fileName.size()];
    strcpy(rmCommand, "rm ");
    strcat(rmCommand, fileName.c_str());
    system(rmCommand);
}

void handleNonEmptyChainList(struct packet_header* header_packet, struct packet* request_packet, int numRemainingMachines, int prev_sock){

    // 1. Uses a random algorithm such as rand() to select the next SS from the list
    int random = chooseRandomNumber(numRemainingMachines);
    string nextSSIP = getIP(request_packet, random);
    int nextSSPort = getPort(request_packet, random);
    cout << "Next SS: " << nextSSIP << ":" << nextSSPort << endl;
    
    struct sockaddr_in addr2;
    int s2;
        
    header_packet->chainListLength = strlen(request_packet->chainList);
    if(DEBUG) { cout << "new chain list length: " << header_packet->chainListLength << endl; }
        
    addr2.sin_family = AF_INET;
    inet_pton(AF_INET, nextSSIP.c_str(), &addr2.sin_addr.s_addr);
    addr2.sin_port = htons(nextSSPort);
        
    if((s2 = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        cout << "Error: Could not create socket." << s2 << endl;
        exit(EXIT_FAILURE);
    }
      
    if(connect(s2, (struct sockaddr *)&addr2, sizeof(addr2)) < 0) {
        cout << "Error: Could not connect to SS." << endl;
        close(s2);
        exit(EXIT_FAILURE);
    }
        
    if(DEBUG) { cout << "Connected to SS!" << endl; }
       
    // send packet header
    send(s2, reinterpret_cast<const char*>(header_packet), sizeof((*header_packet)), 0);
    if(DEBUG) { cout << "Sent packet header" << endl; }
       
    // send packet data
    char packet_data[header_packet->urlLength + header_packet->chainListLength];
    strcpy(packet_data, request_packet->url);
    strcat(packet_data, request_packet->chainList);
    if(DEBUG) { cout << "Sent packet data" << endl; }
        
    send(s2, packet_data, sizeof(packet_data), 0);
        
    if(DEBUG) { cout << "Sent request packet." << endl; }
       
    cout << "waiting for file..." << endl;
    
    vector<struct file_packet_header> data_packet_headers;
    vector<struct file_packet> data_packets;
    bool done = false;
    
    while(done == false) {
        struct file_packet_header f_pack_header;
        recv(s2, reinterpret_cast<char*>(&f_pack_header), sizeof(f_pack_header), 0);
        
        f_pack_header.seqNum = ntohs(f_pack_header.seqNum);
        f_pack_header.lastPacket = ntohs(f_pack_header.lastPacket);
        f_pack_header.dataLength = ntohs(f_pack_header.dataLength);
        
        data_packet_headers.push_back(f_pack_header);
        
        if(DEBUG) {
            cout << "Receiving Data Pack from SS:" << endl;
            cout << "SeqNum: " << f_pack_header.seqNum << endl;
            cout << "last pack? " << f_pack_header.lastPacket << endl;
            cout << "data length: " << f_pack_header.dataLength << endl;
        }
        
        if(f_pack_header.lastPacket != 0) { done = true; }
        
        char* buf = (char*) malloc(sizeof(char) * f_pack_header.dataLength);
        int size = recv(s2, buf, f_pack_header.dataLength, MSG_WAITALL);
        if(DEBUG) { cout << "Received: " << size << endl; }
        
        struct file_packet f_packet;
        f_packet.fileData = new char[size];
        f_packet.fileData = buf;
        data_packets.push_back(f_packet);
    }
    
    // 3. Wait till you receive the file from the next SS
    if(DEBUG) { 
        cout << "Done receiving data packets ";
        cout << " header size: " << data_packet_headers.size();
        cout << " data size: " << data_packets.size() << endl;
    }
        
    // 2. Remove the current ss details from the chain list and send the url and chainlist to the next ss
    // NOTE: this is done above
    
    cout << "Relaying File..." << endl;
    
    // 4. Reads the file in small chunks and transmits it back to the previous SS. The previous SS also receives the file in small chunks
    for(unsigned int i = 0; i < data_packet_headers.size(); i++) {
        if(DEBUG) {
            cout << "Sending Data Packet to prev host:" << endl;
            cout << "SeqNum: " << data_packet_headers.at(i).seqNum << endl;
            cout << "last pack? " << data_packet_headers.at(i).lastPacket << endl;
            cout << "data length: " << data_packet_headers.at(i).dataLength << endl;
        }
        
        size_t numBytesToSend = data_packet_headers.at(i).dataLength;
        
        data_packet_headers.at(i).seqNum = htons(data_packet_headers.at(i).seqNum);
        data_packet_headers.at(i).lastPacket = htons(data_packet_headers.at(i).lastPacket);
        data_packet_headers.at(i).dataLength = htons(data_packet_headers.at(i).dataLength);
        
        // send packet header to prev SS
        send(prev_sock, reinterpret_cast<const char*>(&data_packet_headers.at(i)), sizeof(data_packet_headers.at(i)), 0);
        
        // send packet data to prev SS
        size_t bSent = send(prev_sock, data_packets.at(i).fileData, numBytesToSend, 0);
        if(DEBUG) { cout << "Data Length Sent: " << bSent << endl; }
    }
    
    // 5. Once the file is completely transmitted, the ss should tear down the connection
    close(prev_sock);
    close(s2);
    
    cout << "Done. Goodbye!" << endl;
    
    
    // Erase the local copy and go back to listening for more requests
    string fileName = getFileName(request_packet->url);
    ifstream f(fileName.c_str());
    if(f.good()) {
        char rmCommand[3 + fileName.size()];
        strcpy(rmCommand, "rm ");
        strcat(rmCommand, fileName.c_str());
        system(rmCommand);
    }
}

void sendAndReceivePackets(int port, int prev_sock) {

    // receive packet header
    struct packet_header* header_packet = receiveRequestPacketHeader(prev_sock);
    // receive packet
    struct packet* request_packet = receiveRequestPacket(prev_sock, header_packet);		
    
    if(DEBUG) { printPacketInfo(header_packet, request_packet); }
    
    // get host IP
    char* ip = getHostIP(port);
    string ipString(ip);
    int numRemainingMachines = stripIPAndPort(request_packet, ipString);
    
    // 7. If the chain list is empy:
    if(strlen(request_packet->chainList) == 0) {
        handleEmptyChainList(header_packet, request_packet, prev_sock);
    } else {  // 8. If the chain list is not empty:
        handleNonEmptyChainList(header_packet, request_packet, numRemainingMachines, prev_sock);
    }
}

void sig_handler(int signal){
	exit(0);
}

// USE: $ss [-p port]
int main(int argc, char* argv[]) {

		// handle signals
		signal(SIGINT/SIGTERM/SIGKILL, sig_handler);

    // 1. ss takes in one optional argument (port)
    int port;
    if(argc == 1) {
        port = DEFAULT_PORT;
    } else {
        if(argc != 3) {
            return printHelpMessage();
        }
        if(strcmp(argv[1], "-p") != 0) {
          return printHelpMessage();  
        }
        port = atoi(argv[2]);
    }

		// bind and listen
		int sock= bindAndListen(port);

		// WHILE LOOP
		while(true){
				// ACCEPT USING THE SOCK FROM BIND AND LISTEN (create a new sock_addr)
				int new_sock;
				struct sockaddr_in addr;
				bzero((char *)&addr, sizeof(addr));
    		addr.sin_family = AF_INET;
    		addr.sin_port = htons(port);
				socklen_t addr_size = sizeof(addr);
    		if((new_sock = accept(sock, (struct sockaddr*)&addr, &addr_size)) < 0){
        		cout << "Error: Could not accept connection" << endl;
        		exit(EXIT_FAILURE);
    		}
    
    		if(DEBUG) { cout << "connected!" << endl; }
				
				// START THREAD, JOIN THREAD, RUN MAIN SS WITH SOCKET FROM ACCEPT
				thread run_ss(sendAndReceivePackets, port, new_sock);
				run_ss.join();
		}
    
    return 0;
}
