#include <sys/socket.h>
#include <sys/unistd.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <iostream>
#include <csignal>
#include <stdlib.h>
#include <stdio.h>
#include <netdb.h>
#include <limits.h>
#include <string.h>

using namespace std;

#define PORT_NUMBER 20001
#define MESSAGE_SIZE 140
#define VERSION 457

struct packet{
	short version;
	short length;
	char message[MESSAGE_SIZE];
};	

int run_server()
{
	char host_name[HOST_NAME_MAX]; //character array for host name
	gethostname(host_name, sizeof(host_name)); //read host name into char array
	struct hostent* host_info = gethostbyname(host_name); //get IP address
	struct sockaddr_in server_address = {}; //initialize server sockaddr structure
	server_address.sin_family = AF_INET; //set family
	server_address.sin_port = htons(PORT_NUMBER); //convert and set port
	bcopy(host_info->h_addr, (char*)&server_address.sin_addr, host_info->h_length); //set ip address

	char ip_address[INET_ADDRSTRLEN]; //character array for ip address
	inet_ntop(AF_INET, &server_address.sin_addr, ip_address, sizeof(ip_address));	//convert binary IP to string
	printf("Welcome to Chat!\n");
	printf("Waiting for a connection on %s port %i\n", ip_address, PORT_NUMBER);

	int file_descriptor = socket(AF_INET, SOCK_STREAM, 0); //create socket
	if(file_descriptor == -1){
		printf("Error: Could not create server socket.\n");
		return -1;
	}

	int bind_result = bind(file_descriptor, (struct sockaddr*)&server_address, sizeof(server_address)); //bind socket to port
	if(bind_result == -1){
		printf("Error: Could not bind server socket.\n");
		return -1;
	}

	int listen_result = listen(file_descriptor, 1); //start listening
	if(listen_result == -1){
		printf("Error: Could not begin listening.\n");
		return -1;
	}

	socklen_t server_address_len = sizeof(server_address); //get length of socket
	int connection_descriptor = accept(file_descriptor, (struct sockaddr*)&server_address, &server_address_len); //connect to client
	if(connection_descriptor == -1){
		printf("Error: Could not accept.\n");
		return -1;
	}

	printf("Found a friend! You receive first.\n");

	while(true){
		struct packet receive_packet = {}; //buffer to receive message
		char* receive_array = reinterpret_cast<char*>(&receive_packet); //cast the packet to a char* so that it can be used by recv
		int receive_result = recv(connection_descriptor, receive_array, sizeof(receive_packet), 0); //receive message
		receive_packet.version = ntohs(receive_packet.version); //change version back to host byte order
		receive_packet.length = ntohs(receive_packet.length); //change length back to host by order
		if(receive_result == -1){
			printf("Error: Could not receive from client.\n");
			return -1;
		}
		printf("Friend: %s\n", receive_packet.message); //print message from client

		bool message_sent = false;
		while(message_sent == false){ //prompt for a message until you get one of a valid size
			char send_buffer[MESSAGE_SIZE]; //buffer to send message
			memset(send_buffer, 0, sizeof(send_buffer)); //set the buffer to zeros
			string temp;
			printf("You: ");
			getline(cin, temp);
			if(temp.length() > MESSAGE_SIZE-1){
				printf("Error: Input too long.\n");
			}
			else{
				for(unsigned int i = 0; i < temp.size(); i++){ //copy string into char array
					send_buffer[i] = temp.at(i);
				}
				struct packet send_packet = {}; //create packet to send
				send_packet.version = htons(VERSION); //set version
				send_packet.length = htons(temp.length()); //set length
				bcopy(send_buffer, send_packet.message, sizeof(send_packet.message)); //set message
				const char* send_array = reinterpret_cast<const char*>(&send_packet);
				int send_result = send(connection_descriptor, send_array, sizeof(send_packet), 0); //send message
				if(send_result == -1){
					printf("Error: Could not send to server.\n");
					return -1;
				}
				message_sent = true;
			}
			temp.clear(); //clear the temp string
		}
	}
		
	close(connection_descriptor);
	close(file_descriptor);
	return 0;
}
	
int run_client(int port, char* ip)
{
	struct sockaddr_in client_address = {}; //initialize client sockaddr structure
	client_address.sin_family = AF_INET; //set family
	client_address.sin_port = htons(port); //convert and set port
	struct in_addr in_addr_temp = {}; //setup in_addr for ip number
	inet_pton(AF_INET, ip, &in_addr_temp); //set s_addr in in_addr_temp
	client_address.sin_addr = in_addr_temp; //add in_addr to sockaddr_in

	int file_descriptor = socket(AF_INET, SOCK_STREAM, 0); //create socket
	if(file_descriptor == -1){
		printf("Error: Could not create server socket.\n");
		return -1;
	}

	printf("Connecting to a server... ");

	int connect_result = connect(file_descriptor, (struct sockaddr*)&client_address, sizeof(client_address)); //connect to server
	if(connect_result == -1){
		close(file_descriptor);			
		printf("\nError: Could not connect to server.\n");
		return -1;
	}

	printf("Connected!\n"); 
	printf("Connected to a friend! You send first.\n");


	while(true){
bool message_sent = false;
		while(message_sent == false){ //prompt for a message until you get one of a valid size
			char send_buffer[MESSAGE_SIZE]; //buffer to send message
			memset(send_buffer, 0, sizeof(send_buffer)); //set the buffer to zeros
			string temp;
			printf("You: ");
			getline(cin, temp);
			if(temp.length() > MESSAGE_SIZE-1){
				printf("Error: Input too long.\n");
			}
			else{
				for(unsigned int i = 0; i < temp.size(); i++){ //copy string into char array
					send_buffer[i] = temp.at(i);
				}
				struct packet send_packet = {}; //create packet to send
				send_packet.version = htons(VERSION); //set version
				send_packet.length = htons(temp.length()); //set length
				bcopy(send_buffer, send_packet.message, sizeof(send_packet.message)); //set message
				const char* send_array = reinterpret_cast<const char*>(&send_packet);
				int send_result = send(file_descriptor, send_array, sizeof(send_packet), 0); //send message
				if(send_result == -1){
					printf("Error: Could not send to server.\n");
					return -1;
				}
				message_sent = true;
			}
			temp.clear(); //clear the temp string
		}

		struct packet receive_packet = {}; //buffer to receive message
		char* receive_array = reinterpret_cast<char*>(&receive_packet); //cast the packet to a char* so that it can be used by recv
		int receive_result = recv(file_descriptor, receive_array, sizeof(receive_packet), 0); //receive message
		receive_packet.version = ntohs(receive_packet.version); //change version back to host byte order
		receive_packet.length = ntohs(receive_packet.length); //change length back to host by order
		if(receive_result == -1){
			printf("Error: Could not receive from client.\n");
			return -1;
		}
		printf("Friend: %s\n", receive_packet.message); //print message from client
	}
	
	close(file_descriptor);
	return 0;
}

int is_valid_port(char* value)
{
	int index = 0;
	int port = 0;
	while(value[index] != '\0'){
		if(!(isdigit(value[index]))){
			return -1;
		}
		index++;
	}

	port = atoi(value);
	if(port > 65535){
		return -1;
	}

	return port;
}

bool is_valid_ip(char* value)
{
	int index = 0;
	while(value[index] != '\0'){
		if((!(isdigit(value[index]))) && (!(value[index] == '.'))){
			return false;
		}
		index++;
	}

	return true;
}

void sig_handler(int signal)
{
	exit(0);
}

int main(int argc, char* argv[])
{

	// handle signals
	signal(SIGINT/SIGTERM/SIGKILL, sig_handler);

	// check number of arguments
	if(argc == 1){
		return run_server();
	}
	
	else if(argc == 2){
		if(string(argv[1]) == "-h"){	
			printf("CS457 P1 Project 1: Simple Chat Program\n");
			printf("To run this program as a server, run with no arguments.\n");
			printf("\t./chat");
			printf("To run this program as a client, run with the following arguments:\n");
			printf("\t./chat -p <port number> -s <IP address of server>\n");
		}

		else{
			printf("Error: Incorrect argument.\n");			
			return -1;
		}
	}

	else if(argc == 5)
	{
		if(string(argv[1]) == "-p")
		{
			int port = is_valid_port(argv[2]);
			if(port == -1){
				printf("Error: Invalid port number.\n");
				return -1;
			}

			if(string(argv[3]) != "-s"){
				printf("Error: Incorrect argument.\n");
				return -1;
			}
			
			if(!(is_valid_ip(argv[4]))){
				printf("Error: Invalid IP address.\n");
				return -1;
			}

			return run_client(port, argv[4]);
		}
		
		else if(string(argv[1]) == "-s"){
			if(!(is_valid_ip(argv[2]))){
				cout << "Error: Invalid IP address.\n";
				return -1;
			}

			if(string(argv[3]) != "-p"){
				cout << "Error: Incorrect argument.\n";
				return -1;
			}

			int port = is_valid_port(argv[4]);
			if(port == -1){
				cout << "Error: Invalid port number.\n";
				return -1;
			}

			return run_client(port, argv[2]);
		}

		else{
			cout << "Error: Incorrect argument.\n";
			return -1;
		}
	}

	else{
		cout << "Error: Incorrect number of arguments.\n";
		return -1;
	}

}













