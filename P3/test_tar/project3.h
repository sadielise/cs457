#ifndef PROJECT3_H
#define PROJECT3_H

#include <stdio.h>
#include <stdlib.h>
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
#include <sys/wait.h>
#include <fcntl.h>
#include <cstdlib>
#include <sys/stat.h>
#include <map>
#include <sstream>
#include <chrono>
#include <thread>
#include <tuple>
#include <chrono>
#include <ctime>
#include <sys/time.h>
#include <boost/date_time/posix_time/posix_time.hpp>

using namespace std;
using namespace boost;
using namespace boost::posix_time;

#define PORT_NUMBER 55555
#define MESSAGE_SIZE 140
#define VERSION 457
#define BASE_UDP_PORT 50000

/* This struct is used to send the number of
nodes to a router */
struct packet_header {
	int num_routers;
};

/* This struct is used to send basic messages
between manager/router */
struct packet{
	char message[MESSAGE_SIZE];
};

/* This struct is used to send messages between
routers */
struct router_packet{
	int source_id;
	char message[MESSAGE_SIZE];
};

/* This struct is used by routers to store
neighbor information */
struct neighbor {
	int id;
	int cost;
	int udp_port;
	neighbor(){}
	neighbor(int _id, int _cost, int _udp_port) {
		id = _id;
		cost = _cost;
		udp_port = _udp_port;
	}
};

/* This struct is used to pass router information
from manager to router and from router to router */
struct router_node {
	int id;
	int num_neighbors;
	int udp_port;
	struct neighbor neighbors[100];
	bool has_neighbor[100] = {false};
	router_node(){}
	router_node(int _id, int _num_neighbors, int _udp_port) {
		id = _id;
		num_neighbors = _num_neighbors;
		udp_port = _udp_port;
	}
};

#endif
