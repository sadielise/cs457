#include "project3.h"

int NUM_NODES = 0;
int DEBUG = 0;
int MAX_CONNECTION_LENGTH = 8;
int MANAGER_SOCKET;
vector<int> ROUTER_SOCKETS;
map<int, struct router_node> ROUTERS;
map<int, vector<neighbor>> ROUTER_NEIGHBORS;
ifstream TOPOLOGY_FILE;
ofstream MANAGER_FILE;

int print_help_message() {
    cout << endl << "manager help:" << endl << endl;
    cout << "$manager <input file>" << endl;
    cout << "input file: network topology description" << endl;
	exit(EXIT_FAILURE);
    return -1;
}

void copy_neighbors_to_routers() {
	for(auto const& neighbor_map : ROUTER_NEIGHBORS) {
		for(neighbor n : neighbor_map.second) {
			ROUTERS[neighbor_map.first].neighbors[n.id] = n;
			ROUTERS[neighbor_map.first].has_neighbor[n.id] = true;
		}
		ROUTERS[neighbor_map.first].num_neighbors = neighbor_map.second.size();
	}
}

vector<string> split(const string &s, char delim) {
    stringstream ss(s);
    string item;
    vector<string> tokens;
    while (getline(ss, item, delim)) {
        tokens.push_back(item);
    }
    return tokens;
}

/* Code for this method obtained from: http://stackoverflow.com/questions/16077299/how-to-print-current-time-with-milliseconds-using-c-c11 */
string get_time(){
	const ptime now = microsec_clock::local_time();
	time_duration td = now.time_of_day();
	const long hours = td.hours();
	string hrs;
	if(hours < 10){ hrs = "0"; }
	hrs += to_string(hours);
	const long minutes = td.minutes();
	string mins;
	if(minutes < 10){ mins = "0"; }
	mins += to_string(minutes);
	const long seconds = td.seconds();
	string secs;
	if(seconds < 10){ secs = "0"; }
	secs += to_string(seconds);
	const long milliseconds = td.total_milliseconds() - ((hours * 3600 + minutes * 60 + seconds) * 1000);
	string ms;
	if(milliseconds < 10){ ms = "00"; }
	if(milliseconds < 100){ ms = "0"; }
	ms += to_string(milliseconds);
	string time = "Time " + hrs + ":" + mins + ":" + secs + ":" + ms + "\t";
	return time;
}

void create_routers() {
	for(int i = 0; i < NUM_NODES; i++) {
		struct router_node new_router = {};
		new_router.id = i;
		new_router.udp_port = BASE_UDP_PORT + i; // All UDP ports = BASE_UDP_PORT + router id
		ROUTERS[i] = new_router;
	}
}

void print_topology_to_file(int num_nodes, vector<string>* topology){
	MANAGER_FILE << get_time() << "Topology: " << endl;
	MANAGER_FILE << "\t\t\tSOURCE\tDEST\tCOST" << endl;
	for(unsigned int i = 0; i < (*topology).size(); i++){
		vector<string> elements = split((*topology).at(i), ' ');
		for(unsigned int j = 0; j < elements.size(); j++){
			if(j == 0){ MANAGER_FILE << "\t\t\t"; }
			MANAGER_FILE << elements.at(j) << "\t";
		}
		MANAGER_FILE << endl;
	}
	MANAGER_FILE << endl;
}

void print_topology(int num_nodes, vector<string>* topology){
	cout << "Number of nodes: " << num_nodes << endl;
	for(unsigned int i = 0; i < (*topology).size(); i++){
		cout << (*topology).at(i) << endl;
	}
}

vector<string> read_topology_file(string* filename){

  TOPOLOGY_FILE.open(*filename);
	TOPOLOGY_FILE >> NUM_NODES;
	
	char connection[MAX_CONNECTION_LENGTH];
	TOPOLOGY_FILE.getline(connection, MAX_CONNECTION_LENGTH);
	
	create_routers();

	vector<string> topology;
	bool end_of_topology = false;
	while(end_of_topology == false){
		char connection[MAX_CONNECTION_LENGTH];
		TOPOLOGY_FILE.getline(connection, MAX_CONNECTION_LENGTH);
		if(connection[0] == '-'){ end_of_topology = true; }
		else{
			string line(connection);
			trim(line);
			vector<string> elements = split(line, ' ');
			
			int router_id = stoi(elements.at(0));
			int neighbor_id = stoi(elements.at(1));
			int cost = stoi(elements.at(2));
			
			ROUTER_NEIGHBORS[router_id].push_back(neighbor(neighbor_id, cost, BASE_UDP_PORT + neighbor_id));
			ROUTER_NEIGHBORS[neighbor_id].push_back(neighbor(router_id, cost, BASE_UDP_PORT + router_id));
			
			topology.push_back(line);
		}
	}
	
	copy_neighbors_to_routers();
	
	return topology;
}

void receive_router_packet(int accept_socket, int i){
	struct packet receive_packet = {};
	char* receive_array = reinterpret_cast<char*>(&receive_packet);
	int receive_result = recv(accept_socket, receive_array, sizeof(receive_packet), 0);
	if(receive_result == -1){
		cout << "Error: Could not receive from router." << endl;
	}
	if(DEBUG){ cout << "Received from router " << i << ": " << receive_packet.message << endl; }
	MANAGER_FILE << get_time() << "Received from router " << i << ": " << receive_packet.message << endl;
}

bool receive_ready_packet(int accept_socket){
	struct packet receive_packet = {};
	char* receive_array = reinterpret_cast<char*>(&receive_packet);
	int receive_result = recv(accept_socket, receive_array, sizeof(receive_packet), 0);
	if(receive_result == -1){
		cout << "Error: Could not receive from router." << endl;
	}
	char ready[] = "ready";
	if(strcmp(ready, receive_packet.message) == 0){
		return true;
	}
	else{
		return false;
	}
}

void send_message_to_router(int accept_socket, int router_id){
	struct packet_header pack_head = {};
	pack_head.num_routers = NUM_NODES;
	send(accept_socket, reinterpret_cast<char*>(&pack_head), sizeof(pack_head), 0);
	struct router_node r = ROUTERS[router_id];
	send(accept_socket, reinterpret_cast<char*>(&r), sizeof(r), 0);
	if(DEBUG){ cout << "Sent topology to router " << router_id << endl; }
	MANAGER_FILE << get_time() << "Sent topology to router " << router_id << endl << endl;
}

int accept_router_connection(int manager_socket){
	struct sockaddr_in client_address = {};
	socklen_t client_address_len = sizeof(client_address);
	int accept_socket = accept(manager_socket, (struct sockaddr*)&client_address, &client_address_len);
	if(accept_socket == -1){
		cout << "Error: Could not accept." << endl;
		return -1;
	}
	return accept_socket;
}

void close_router_connections(){
	for(unsigned int i = 0; i < ROUTER_SOCKETS.size(); i++){
		close(ROUTER_SOCKETS.at(i));
	}
}

void confirm_router_ready(int i){
	if(receive_ready_packet(ROUTER_SOCKETS.at(i)) == true){
		if(DEBUG){ cout << "Router " << i << " ready" << endl; }
		MANAGER_FILE << get_time() << "Received from router " << i << ": ready" << endl;
	}
}

void send_instruction_to_router(int source_id, string destination_id){
	int source_socket = ROUTER_SOCKETS.at(source_id);
	struct packet instruction = {};
	string message = "send packet to router " + destination_id;
	for(unsigned int i = 0; i < message.size(); i++){
		instruction.message[i] = message.at(i);
	}
	send(source_socket, reinterpret_cast<char*>(&instruction), sizeof(instruction), 0);
	if(DEBUG){ cout << "Instruction: " << source_id << " " << destination_id << endl; }
	MANAGER_FILE << get_time() << "Sent message to router " << source_id << ": " << message << endl;
}

void send_quit_to_routers(){
	MANAGER_FILE << endl;
	string message = "Quit";
	struct packet instruction = {};
	for(unsigned int i = 0; i < message.size(); i++){
		instruction.message[i] = message.at(i);
	}
	for(int i = 0; i < NUM_NODES; i++){
		send(ROUTER_SOCKETS.at(i), reinterpret_cast<char*>(&instruction), sizeof(instruction), 0);
		if(DEBUG){ cout << "Sent quit to router " << i << endl; }
		MANAGER_FILE << get_time() << "Sent quit to router " << i << endl;
	}

	MANAGER_FILE << endl;
}	

void send_router_instructions(){

	MANAGER_FILE << endl << get_time() << "Sending instructions to routers..." << endl << endl;
	bool end_of_instructions = false;
	int instruction_number = 0;
	while(end_of_instructions == false){
		char connection[MAX_CONNECTION_LENGTH];
		TOPOLOGY_FILE.getline(connection, MAX_CONNECTION_LENGTH);
		if(connection[0] == '-'){ 
			end_of_instructions = true; 
			this_thread::sleep_for(chrono::milliseconds(10000));
			send_quit_to_routers();
		}
		else{
			string line(connection);
			trim(line);
			vector<string> elements = split(line, ' ');			
			int source_id = stoi(elements.at(0));
			string destination_id = elements.at(1);
			instruction_number = instruction_number + 1;
			send_instruction_to_router(source_id, destination_id);
			this_thread::sleep_for(chrono::milliseconds(5000));
		}
	}
}

void connect_to_routers(int manager_socket){
	pid_t pid;
	for(int i = 0; i < NUM_NODES; i++){
		pid = fork();
		if(pid == 0) { // child process
			system("./router");
			_exit(0);
		} else if(pid > 0) { // parent_process
			MANAGER_FILE << get_time() << "Starting router " << i << "..." << endl;
			int accept_socket = accept_router_connection(manager_socket);
			if(accept_socket == -1) return;
			MANAGER_FILE << get_time() << "Connected to router " << i << endl;
			ROUTER_SOCKETS.push_back(accept_socket);
			receive_router_packet(accept_socket, i);
			send_message_to_router(accept_socket, i);
		} else {
			cout << "fork failed" << endl;
			exit(-1);
		}
	}

	for(int i = 0; i < NUM_NODES; i++){
		confirm_router_ready(i);
	}
	
	cout << "Waiting for routers to finish..." << endl;
	
	send_router_instructions();
}

int start_listening()
{
	struct sockaddr_in manager_address = {}; //initialize server sockaddr structure

	int manager_socket = socket(AF_INET, SOCK_STREAM, 0); //create manager socket
	if(manager_socket == -1){
		cout << "Error: Could not create server socket." << endl;
		return -1;
	}

	manager_address.sin_family = AF_INET; //set family
	manager_address.sin_port = htons(PORT_NUMBER); //convert and set port
	manager_address.sin_addr.s_addr = INADDR_ANY;
	
	int bind_result = bind(manager_socket, (struct sockaddr*)&manager_address, sizeof(manager_address)); //bind socket to port
	if(bind_result == -1){
		printf("Error: Could not bind server socket.\n");
		return -1;
	}

	int listen_result = listen(manager_socket, NUM_NODES); //start listening
	if(listen_result == -1){
		printf("Error: Could not begin listening.\n");
		return -1;
	}
	if(DEBUG){ cout << "Manager is listening..." << endl; }
	MANAGER_FILE << get_time() << "Listening for routers..." << endl << endl;

	return manager_socket;
}

void sig_handler(int signal){
	exit(0);
}

int main(int argc, char* argv[]) {

	cout << "Manager starting..." << endl;

	// handle signals
	signal(SIGINT/SIGTERM/SIGKILL, sig_handler);

	// check args
	if(argc != 2){ return print_help_message(); }

	// create the output file
	string output_file = "manager.out";
	remove(output_file.c_str()); // clear the file if it already exists
	MANAGER_FILE.open(output_file.c_str(), ios::out | ios::app);
	chmod(output_file.c_str(), 0666);
	MANAGER_FILE << "-----------------------MANAGER-----------------------" << endl << endl;
	
	// read topology file
	string filename = argv[1];
	if(DEBUG){ cout << "Reading topology file..." << endl; }
	vector<string> topology = read_topology_file(&filename);
	if(DEBUG){ print_topology(NUM_NODES, &topology); }
	print_topology_to_file(NUM_NODES, &topology);

	// start listening for routers
	MANAGER_SOCKET = start_listening();

	// connect to and run routers
	connect_to_routers(MANAGER_SOCKET);

	int status;
	int tempN = NUM_NODES;
	while(tempN > 0){
		wait(&status);
		tempN--;
	}

	// close router connections
	close_router_connections();
	
	// close manager 
	close(MANAGER_SOCKET);

	MANAGER_FILE << get_time() << "Exiting manager..." << endl << endl;

	// close manager file
	MANAGER_FILE.close();
	
	cout << "Program exiting successfully!" << endl;
	cout << "Manager and Router output logged in corresponding files." << endl;
	
	exit(0);
}
