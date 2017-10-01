#include "awget.h"
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
#include <signal.h>

using namespace std;
using namespace boost;

#define DEFAULT_CHAINFILE "chaingang.txt"
#define MAX_CHARS_PER_LINE 512

#define DEBUG 0

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
    cout << endl << "awget help:" << endl << endl;
    cout << "$ awget <URL> [-c chainfile]" << endl;
    cout << "URL: url of file to get" << endl;
    cout << "-c: name of chainfile (will look for local file chaingang.txt if not provided)" << endl;
	exit(EXIT_FAILURE);
    return -1;
}

int chooseRandomNumber(int max){
    srand(time(NULL));
    int num = rand() % max;
    return num;
}

string* getIP(struct packet* p, int location){
    vector<string> strings;
    split(strings, p->chainList, is_any_of(",:"));
    string* IP = new string();
    *IP = strings[location * 2].c_str();
    return IP;
}
		
int getPort(struct packet* p, int location){
    vector<string> strings;
    split(strings, p->chainList, is_any_of(",:"));
    int port = atoi(strings[((location * 2) + 1)].c_str());
    return port;
}

struct packet_header* createHeaderPacket(string* url, string* chainList){
    struct packet_header* header_packet = (struct packet_header*) malloc(sizeof(struct packet_header));
    // initialize packet header
    header_packet->urlLength = (*url).size();
    header_packet->chainListLength = (*chainList).size();
    return header_packet;
}

struct packet* createRequestPacket(string* url, string* chainList){
    struct packet* request_packet = (struct packet*) malloc(sizeof(struct packet));
    // initialize packet data
    request_packet->url = new char[(*url).size()];
    request_packet->chainList = new char[(*chainList).size()];
    strcpy(request_packet->url, (*url).c_str());
    strcpy(request_packet->chainList, (*chainList).c_str());
    return request_packet;
}

int getNumSS(string* chainfile){
    ifstream file;
    file.open(*chainfile);
    if(!file.good()) {
        cout << "Could not read file: " << *chainfile << endl;
        exit(EXIT_FAILURE);
    }
    
    char numSSBuf[MAX_CHARS_PER_LINE];
    file.getline(numSSBuf, MAX_CHARS_PER_LINE);
    int numSS = numSSBuf[0] - '0';
    
    if(DEBUG) { cout << "# of SS's: " << numSS << endl; }

    file.close();

    return numSS;
}

string* readChainFile(string* chainfile, int numSS){

    ifstream file;
    file.open(*chainfile);
    if(!file.good()) {
        cout << "Could not read file: " << *chainfile << endl;
        exit(EXIT_FAILURE);
    }
    
    char tempBuf[MAX_CHARS_PER_LINE];
    file.getline(tempBuf, MAX_CHARS_PER_LINE);

    // string to hold the chain list info
    string* chainList = new string();

    // read each line and split by space, add each string to chainList
    char buf[MAX_CHARS_PER_LINE];
    cout << "chainlist is" << endl;
    for(int i = 0; i < numSS; i++) {
        file.getline(buf, MAX_CHARS_PER_LINE);
        cout << buf << endl;
        vector<string> machines;
        split(machines, buf, is_any_of(" "));
        for(unsigned int j = 0; j < machines.size(); j++){
            *chainList += machines[j];
            if(j == 0) { *chainList += ":"; }
            if(j == 1 && i != (numSS - 1)) { *chainList += ","; }
        }
    }
	
    file.close();

    return chainList;
}

int sendGetRequest(struct packet_header* packet_header, struct packet* request_packet, string* ssIP, int ssPort) {
    struct sockaddr_in addr;
    int send_sock;
    
    addr.sin_family = AF_INET;
    inet_pton(AF_INET, (*ssIP).c_str(), &addr.sin_addr.s_addr);
    addr.sin_port = htons(ssPort);
    
    if((send_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        cout << "Error: Could not create socket." << send_sock << endl;
        exit(EXIT_FAILURE);
    }
    
    cout << "next SS is " << *ssIP << ":" << ssPort << endl;
    
    if(connect(send_sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        cout << "Error: Could not connect to SS." << endl;
        close(send_sock);
        exit(EXIT_FAILURE);
    }
    
    if(DEBUG) { cout << "Connected to SS!" << endl; }
   
    // send packet header
    send(send_sock, reinterpret_cast<const char*>(packet_header), sizeof(*packet_header), 0);
    if(DEBUG) { cout << "Sent packet header" << endl; }
   
    // send packet data
    char packet_data[packet_header->urlLength + packet_header->chainListLength];
    strcpy(packet_data, request_packet->url);
    strcat(packet_data, request_packet->chainList);
    
    send(send_sock, packet_data, sizeof(packet_data), 0);
    
    if(DEBUG) { cout << "Sent request packet." << endl; }
    
    cout << "waiting for file..." << endl;
    
    return send_sock;
}

void receiveFile(int rec_sock, string filename){
    vector<struct file_packet_header> data_packet_headers;
    vector<struct file_packet> data_packets;
    bool done = false;
    
    while(done == false) {
        struct file_packet_header f_pack_header;
        recv(rec_sock, reinterpret_cast<char*>(&f_pack_header), sizeof(f_pack_header), 0);
        
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
        int size = recv(rec_sock, buf, (sizeof(char) * f_pack_header.dataLength), MSG_WAITALL);
        if(DEBUG) { cout << "Received: " << size << endl; }
        
        struct file_packet f_packet;
        f_packet.fileData = new char[size];
        f_packet.fileData = buf;
        data_packets.push_back(f_packet);
    }
    
    cout << "Received File. Writing file to: " << filename << endl;
    
    FILE *file_ptr;
    file_ptr = fopen(filename.c_str(), "wb");

    for(unsigned int i = 0; i < data_packets.size(); i++) {
        fwrite(data_packets.at(i).fileData, data_packet_headers.at(i).dataLength, 1, file_ptr);
    }
    
    fclose(file_ptr);
    
    cout << "Done. Goodbye!" << endl;
}

void sendAndReceiveFile(string* url, string* chainfile) {

    cout << "Request: " << *url << endl;
    if(DEBUG) { cout << "CHAINFILE: " << chainfile << endl; }

    // get numSS
    int numSS = getNumSS(chainfile);
    // read chainfile
    string* chainFileString = readChainFile(chainfile, numSS);
    // create header packet
    struct packet_header* header_packet = createHeaderPacket(url, chainFileString);
    // create request packet
    struct packet* request_packet = createRequestPacket(url, chainFileString);

    // get random number
    int randomNum = chooseRandomNumber(numSS);
    if(DEBUG) { cout << "Random Number: " << randomNum << endl; }

    // get IP address
    string* IP = getIP(request_packet, randomNum);
    if(DEBUG) { cout << "IP: " << *IP << endl; }

    // get port
    int port = getPort(request_packet, randomNum);
    if(DEBUG) { cout << "Port: " << port << endl; }
    if(DEBUG) { cout << "Packet: " << endl;  printPacketInfo(header_packet, request_packet); }

    // send packets to ss
    int send_sock = sendGetRequest(header_packet, request_packet, IP, port);

    // receive packets from ss
    string filename = getFileName(request_packet->url);
    receiveFile(send_sock, filename);
}

void sig_handler(int signal){
	exit(0);
}

// USE: $awget <URL> [-c chainfile]
int main(int argc, char* argv[]) {

		// handle signals
		signal(SIGINT/SIGTERM/SIGKILL, sig_handler);

    string url;
    string chainfile;
    
    if(argc < 2) {
        return printHelpMessage();
    }
    if(argc == 2) {
        url = argv[1];
        chainfile = DEFAULT_CHAINFILE;
    } else if(argc == 4) {
        url = argv[1];
        chainfile = argv[3];
    } else {
        return printHelpMessage();
    }
    
    sendAndReceiveFile(&url, &chainfile);

    return 0;
}
