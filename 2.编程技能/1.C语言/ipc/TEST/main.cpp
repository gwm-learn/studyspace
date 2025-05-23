#include <string>
#include <iostream>
#include <unistd.h>
#include <stdlib.h>

// #include "m_shm.h"
// #include "m_fifo.h"
// #include "m_socket.h"
// #include "m_message_queue.h"

typedef struct usage_info_t
{
	bool shm;
	bool fifo;
	bool message_queue;
	bool socket;
	int ipc_num;
	int send_or_recv;
	int shm_key;
	int socket_port;
	int message_queue_key;
	std::string fifo_name;
}usage_info;

void usage()
{
	std::cout << "Usage:          ./test -F[M/H/S] -n 0[1]" << std::endl;
	std::cout << "shm:            ./test -S shm_key ..." << std::endl;
	std::cout << "fifo:           ./test -F fifo_name ..." << std::endl;
	std::cout << "socket:         ./test -P socket_port ..." << std::endl;
	std::cout << "message_queue:  ./test -M message_queue_key ..." << std::endl;
	std::cout << "NOTE: if start one ipc, default ipc to send. with -n 0 --> send, 1 --> recv" << std::endl;
	std::cout << "NOTE: if start two ipc, first ipc to recv, second ipc to send." << std::endl;
}

int main(int argc, char *argv[])
{
	if(argc < 2)
	{
		usage();
		return 0;
	}

	usage_info use;
	use.shm = false;
	use.fifo = false;
	use.socket = false;
	use.message_queue = false;
	use.ipc_num = 0;
	use.send_or_recv = 0;

	int ch;
	while((ch = getopt(argc, argv, "S:F:P:M:n:")) != -1)
	{
		switch(ch)
		{
			case 'S':
				use.shm = true;
				use.ipc_num++;
				if(use.ipc_num > 2)
				{
					usage();
					return 0;
				}
				use.shm_key = atoi(optarg);
				std::cout << "shm_key is " << optarg << std::endl;
				break;
			case 'F':
				use.fifo = true;
				use.ipc_num++;
				if(use.ipc_num > 2)
				{
					usage();
					return 0;
				}
				use.fifo_name = optarg;
				std::cout << "fifo_name is " << optarg << std::endl;
				break;
			case 'P':
				use.socket = true;
				use.ipc_num++;
				if(use.ipc_num > 2)
				{
					usage();
					return 0;
				}
				use.socket_port = atoi(optarg);
				std::cout << "socket_port is " << optarg << std::endl;
				break;
			case 'M':
				use.message_queue = true;
				use.ipc_num++;
				if(use.ipc_num > 2)
				{
					usage();
					return 0;
				}
				use.message_queue_key = atoi(optarg);
				std::cout << "message_queue_key is " << optarg << std::endl;
				break;
			case 'n':
				use.send_or_recv = atoi(optarg);
				std::cout << "flag is " << optarg << std::endl;
				break;
			case '?':
				std::cout << "unknown option" << std::endl;
				usage();
				return 0;
			default:
				std::cout << "default" << std::endl;
				usage();
				return 0;
		}
	}

	
}