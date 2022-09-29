#include "TRAPZ_TCPConnection.h"

//server mode
//blocks until connected
TRAPZ_TCPConnection::TRAPZ_TCPConnection(string							host_addr,
										 int							host_port,
										 bool							use_async = true)
{
	m_mode = Mode::SERVER;
	m_hostAddr = host_addr;
	m_hostPort = host_port;
	m_asyncFlag = use_async;

	m_asyncFlagSemaphore = new TRAPZ_Semaphore(1, 1, "Semaphore of async flag.");
	m_receivingQueueSemaphore = new TRAPZ_Semaphore(1, 1, "Semaphore of the receiving queue.");
	m_sendingQueueSemaphore = new TRAPZ_Semaphore(1, 1, "Semaphore of the sending queue");
	m_threadsRunningFlagSemaphore = new TRAPZ_Semaphore(1, 1, "Semaphore of the threads running flag.");
	m_threadsActiveFlagSemaphore = new TRAPZ_Semaphore(1, 1, "Semaphore of the threads active flag.");
	m_sendingQueueEntrySemaphore = new TRAPZ_Semaphore(0, TRAPZ_TCPConnection_MAX_SENDQUEUE + 1, "Semaphore of the threads running flag.");

	m_callbackMessageReceived = [](string a){};
	m_callbackConnectionClosed = [](){};
	m_callbackConnectionEstablished = [](){};

	initServerMode(host_addr, host_port);

	m_sendingThread = std::thread(&sendingThreadFunction, this);
	m_receivingThread = std::thread(&receivingThreadFunction, this);

	changeAsyncFlag(use_async);

	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		WSACleanup();
		exit(EXIT_FAILURE);
	}
}

//client mode
//blocks until connected
TRAPZ_TCPConnection::TRAPZ_TCPConnection(string							host_addr,
										 int							host_port,
										 string							remote_addr,
										 int							remote_port,
										 bool							use_async = true)
{
	m_mode = Mode::CLIENT;
	m_hostAddr = host_addr;
	m_hostPort = host_port;
	m_remoteAddr = remote_addr;
	m_remotePort = remote_port;
	m_asyncFlag = use_async;

	m_asyncFlagSemaphore = new TRAPZ_Semaphore(1, 1, "Semaphore of async flag.");
	m_receivingQueueSemaphore = new TRAPZ_Semaphore(1, 1, "Semaphore of the receiving queue.");
	m_sendingQueueSemaphore = new TRAPZ_Semaphore(1, 1, "Semaphore of the sending queue");
	m_threadsRunningFlagSemaphore = new TRAPZ_Semaphore(1, 1, "Semaphore of the threads running flag.");
	m_threadsActiveFlagSemaphore = new TRAPZ_Semaphore(1, 1, "Semaphore of the threads active flag.");
	m_sendingQueueEntrySemaphore = new TRAPZ_Semaphore(0, TRAPZ_TCPConnection_MAX_SENDQUEUE + 1, "Semaphore of the threads running flag.");

	m_callbackMessageReceived = [](string a){};
	m_callbackConnectionClosed = [](){};
	m_callbackConnectionEstablished = [](){};

	initClientMode(host_addr, host_port, remote_addr, remote_port);

	m_sendingThread = std::thread(&sendingThreadFunction, this);
	m_receivingThread = std::thread(&receivingThreadFunction, this);

	changeAsyncFlag(use_async);

	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		WSACleanup();
		exit(EXIT_FAILURE);
	}
}

TRAPZ_TCPConnection::~TRAPZ_TCPConnection()
{
	cleanup();
}

void TRAPZ_TCPConnection::changeAsyncFlag(bool flag)
{
	m_asyncFlagSemaphore->enter();

	m_asyncFlag = flag;

	if (m_asyncFlag){
		startThreads();
	}
	else{
		stopThreads();
	}

	m_asyncFlagSemaphore->leave();
}

void TRAPZ_TCPConnection::setCallback_MessageReceived(std::function<void(string)> callback_function)
{
	m_callbackMessageReceived = callback_function;
}

void TRAPZ_TCPConnection::setCallback_ConnectionClosed(std::function<void(void)> callback_function, bool recall)
{
	m_callbackConnectionClosedRecallFlag = recall;
	m_callbackConnectionClosed = callback_function;
}

void TRAPZ_TCPConnection::setCallback_ConnectionEstablished(std::function<void(void)> callback_function, bool recall)
{
	m_callbackConnectionEstablishedRecallFlag = recall;
	m_callbackConnectionClosed = callback_function;
}

void TRAPZ_TCPConnection::sendData(string data)
{
	m_asyncFlagSemaphore->enter();
	switch (m_asyncFlag)
	{
	case true://async receiving
		//enter queue critical section
		m_sendingQueueSemaphore->enter();

			//wait until there is space in queue
			while (m_sendingQueue.size() == TRAPZ_TCPConnection_MAX_SENDQUEUE)
			{
				m_sendingQueueSemaphore->leave();
				std::this_thread::sleep_for(std::chrono::microseconds(10));
				m_sendingQueueSemaphore->enter();
			}

			//add to sending queue
			//TODO

		//leave queue critical section
		m_sendingQueueSemaphore->leave();

		//up sending semaphore by one for each element in the queue
		m_sendingQueueEntrySemaphore->leave();
		break;
	case false://blocking receiving
		//send data over tcp socket
		break;
	}
	m_asyncFlagSemaphore->leave();
}

string TRAPZ_TCPConnection::receiveData()
{
	string data = "";

	m_asyncFlagSemaphore->enter();
	switch (m_asyncFlag)
	{
	case true://async receiving
		//get all data from the queue and put into one string
		m_receivingQueueSemaphore->enter();
		
		for (string message : m_receivingQueue)
		{
			data += message;
		}
		//clear out receiving queue
		m_receivingQueue.clear();

		m_receivingQueueSemaphore->leave();
		break;
	case false://blocking receiving

		//get data from queue (if async mode was changed and queue wasnt cleared out)
		m_receivingQueueSemaphore->enter();
		if (m_receivingQueue.size() > 0)
		{
			for (string message : m_receivingQueue)
			{
				data += message;
			}
			//clear out receiving queue
			m_receivingQueue.clear();
		}
		m_receivingQueueSemaphore->leave();

		//receive every message from the socket and add to data

		break;
	}
	m_asyncFlagSemaphore->leave();

	return data;
}

TRAPZ_TCPConnection::Additional_Data* TRAPZ_TCPConnection::getData()
{
	return &m_additionalData;
}

void TRAPZ_TCPConnection::sendMessage(string message)
{

}

void TRAPZ_TCPConnection::initClientMode(string host_addr, int host_port, string remote_addr, int remote_port)
{
	closesocket(m_socket);

	//create socket
	m_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	//check if socket is valid
	if (m_socket == INVALID_SOCKET)
	{
		cleanup();
		throwMessage(WSAGetLastErrorString());
	}
	//configure socket
	setSocketOption(SOL_SOCKET, SO_REUSEADDR, 1, m_socket);	//enable use of addr on multiple sockets
	setSocketOption(IPPROTO_TCP, TCP_NODELAY, 1, m_socket);	//no delay on the socket, no min size of buffer to send
	setSocketOption(SOL_SOCKET, SO_RCVBUF, TRAPZ_TCPConnection_RCVBUF_SIZE, m_socket); //set receiving buffer size 
	setSocketOption(SOL_SOCKET, SO_SNDBUF, TRAPZ_TCPConnection_SNDBUF_SIZE, m_socket); //set receiving buffer size

	//bind socket to specified addr and port
	struct sockaddr_in mSockAddress;
	mSockAddress.sin_family = AF_INET;

	mSockAddress.sin_addr.s_addr = inet_addr(host_addr.c_str());
	mSockAddress.sin_port = htons(host_port);
	if (bind(m_socket, (struct sockaddr*)&mSockAddress, sizeof(mSockAddress)) == SOCKET_ERROR)
	{
		cleanup();
		throwMessage(WSAGetLastErrorString());
	}

	//connect socket
	setSocketBlocking();

	mSockAddress.sin_addr.s_addr = inet_addr(remote_addr.c_str());
	mSockAddress.sin_port = htons(remote_port);

	//loop until connected (blocks thread)
	bool connected = false;
	while (!connected)
	{
		if (connect(m_socket, (const sockaddr*)&mSockAddress, sizeof(mSockAddress)) == SOCKET_ERROR)
		{
			if (WSAGetLastError() != WSAETIMEDOUT)
			{
				throw std::runtime_error("Error while connecting.");
			}
		} else
		{
			connected = true;
			m_callbackConnectionEstablished();
		}
	}
}

void TRAPZ_TCPConnection::initServerMode(string host_addr, int host_port)
{
	closesocket(m_socket);
	closesocket(m_listeningSocket);

	//create listening socket
	m_listeningSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	//check if socket is valid
	if (m_listeningSocket == INVALID_SOCKET)
	{
		cleanup();
		throwMessage(WSAGetLastErrorString());
	}
	//configure socket
	setSocketOption(SOL_SOCKET, SO_REUSEADDR, 1, m_listeningSocket);	//enable use of addr on multiple sockets
	setSocketOption(IPPROTO_TCP, TCP_NODELAY, 1, m_listeningSocket);	//no delay on the socket, no min size of buffer to send

	//bind socket to specified addr and port
	struct sockaddr_in mSockAddress;
	mSockAddress.sin_family = AF_INET;

	mSockAddress.sin_addr.s_addr = inet_addr(host_addr.c_str());
	mSockAddress.sin_port = htons(host_port);
	if (bind(m_listeningSocket, (struct sockaddr*)&mSockAddress, sizeof(mSockAddress)) == SOCKET_ERROR)
	{
		cleanup();
		throwMessage(WSAGetLastErrorString());
	}

	//listen for 1 connection
	if (listen(m_listeningSocket, 1) == SOCKET_ERROR)
	{
		cleanup();
		throwMessage(WSAGetLastErrorString());
	}

	sockaddr_in addr;
	int size = sizeof(sockaddr);
	//accept 1 connection
	m_socket = accept(m_listeningSocket, (struct sockaddr*)&addr, &size);
	if (m_socket == INVALID_SOCKET)
	{
		cleanup();
		throwMessage(WSAGetLastErrorString());
	} else
	{
		//configure socket
		setSocketOption(SOL_SOCKET, SO_REUSEADDR, 1, m_socket);	//enable use of addr on multiple sockets
		setSocketOption(IPPROTO_TCP, TCP_NODELAY, 1, m_socket);	//no delay on the socket, no min size of buffer to send
		setSocketOption(SOL_SOCKET, SO_RCVBUF, TRAPZ_TCPConnection_RCVBUF_SIZE, m_socket); //set receiving buffer size 
		setSocketOption(SOL_SOCKET, SO_SNDBUF, TRAPZ_TCPConnection_SNDBUF_SIZE, m_socket); //set receiving buffer size 
	}
}

int TRAPZ_TCPConnection::sendingThreadFunction()
{
	bool running;

	m_threadsRunningFlagSemaphore->enter();
	running = m_threadsRunningFlag;
	m_threadsRunningFlagSemaphore->leave();

	//used to terminate the thread completely
	while (running)
	{
		bool active;

		m_threadsActiveFlagSemaphore->enter();
		active = m_threadsActiveFlag;
		m_threadsActiveFlagSemaphore->leave();

		//used to start and stop threads without creating and deleting them
		while (active)
		{
			//down semaphore for each sent element
			m_sendingQueueEntrySemaphore->enter();

			//enter queue critical section
			m_sendingQueueSemaphore->enter();
			string message = m_sendingQueue.front();
			//leave queue critical section
			m_sendingQueueSemaphore->enter();

			//send message
			sendMessage(m_sendingQueue.front());

			//enter queue critical section
			m_sendingQueueSemaphore->enter();
			m_sendingQueue.pop_front();
			//leave queue critical section
			m_sendingQueueSemaphore->enter();
		
			//update active flag
			m_threadsActiveFlagSemaphore->enter();
			active = m_threadsActiveFlag;
			m_threadsActiveFlagSemaphore->leave();
		}

		//update running flag
		m_threadsRunningFlagSemaphore->enter();
		running = m_threadsRunningFlag;
		m_threadsRunningFlagSemaphore->leave();
	}
	return 0;
}

int TRAPZ_TCPConnection::receivingThreadFunction()
{
	bool running;

	m_threadsRunningFlagSemaphore->enter();
	running = m_threadsRunningFlag;
	m_threadsRunningFlagSemaphore->leave();

	while (running)
	{
		bool active;

		m_threadsActiveFlagSemaphore->enter();
		active = m_threadsActiveFlag;
		m_threadsActiveFlagSemaphore->enter();

		while (active)
		{
			//receive

			//put in queue

			m_threadsActiveFlagSemaphore->enter();
			active = m_threadsActiveFlag;
			m_threadsActiveFlagSemaphore->enter();
		}

		//update running flag
		m_threadsRunningFlagSemaphore->enter();
		running = m_threadsRunningFlag;
		m_threadsRunningFlagSemaphore->leave();
	}
	return 0;
}

void TRAPZ_TCPConnection::startThreads()
{
	m_threadsActiveFlagSemaphore->enter();
	m_threadsActiveFlag = true;
	m_threadsActiveFlagSemaphore->leave();
}

void TRAPZ_TCPConnection::stopThreads()
{
	m_threadsActiveFlagSemaphore->enter();

	m_sendingQueueSemaphore->enter();

	//wait until all data has been sent
	while (m_sendingQueue.size() > 0)
	{
		m_threadsActiveFlagSemaphore->leave();
		m_sendingQueueSemaphore->leave();
		std::this_thread::sleep_for(std::chrono::microseconds(10));
		m_threadsActiveFlagSemaphore->enter();
		m_sendingQueueSemaphore->enter();
	}
	m_sendingQueueSemaphore->leave();

	m_threadsActiveFlag = false;
	m_threadsActiveFlagSemaphore->leave();
}

void TRAPZ_TCPConnection::cleanup()
{
	if (m_mode == Mode::SERVER)
	{
		//close listening socket
	}
	//close socket

	WSACleanup();

	//stop and join threads
}

void TRAPZ_TCPConnection::setSocketBlocking()
{
	u_long val = 0;
	int result;
	result = ioctlsocket(m_socket, FIONBIO, &val);
	if (result == SOCKET_ERROR)
	{
		cleanup();
		throwMessage(WSAGetLastErrorString());
	}
}

void TRAPZ_TCPConnection::setSocketNonBlocking()
{
	u_long val = 1;
	if (ioctlsocket(m_socket, FIONBIO, &val) == SOCKET_ERROR)
	{
		cleanup();
		throwMessage(WSAGetLastErrorString());
	}
}

void TRAPZ_TCPConnection::setSocketOption(int level, int option, int value, SOCKET socket)
{
	if (setsockopt(socket, level, option, (const char*)&value, sizeof(int)) == SOCKET_ERROR)
	{
		cleanup();
		throwMessage(WSAGetLastErrorString());
	}
}

string TRAPZ_TCPConnection::WSAGetLastErrorString()
{
	string data = "";
	FormatMessageW(	  FORMAT_MESSAGE_ALLOCATE_BUFFER	//automatic allocation of buffer
					| FORMAT_MESSAGE_FROM_SYSTEM		//search for system messages
					| FORMAT_MESSAGE_IGNORE_INSERTS,	//dont change insert definitions (%1, ...)
					NULL,								//no line width restrictions
					WSAGetLastError(),					//error id
					0,									//use language based on order (language neutral, threads langid, user default langid, system default langid, us english)
					(LPWSTR)&data,
					0,
					NULL);
	return data;
}

void TRAPZ_TCPConnection::throwMessage(string message)
{
	throw std::runtime_error(message);
}






















