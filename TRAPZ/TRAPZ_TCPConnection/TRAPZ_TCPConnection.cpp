#include "TRAPZ_TCPConnection.h"

//server mode
//blocks until connected
TRAPZ_TCPConnection::TRAPZ_TCPConnection(string	host_addr,int host_port, bool use_async)
{
	m_mode = Mode::SERVER;
	m_hostAddr = host_addr;
	m_hostPort = host_port;
	m_asyncFlag = use_async;

	m_receivingQueueSemaphore = new TRAPZ_Semaphore(1, 1, "Semaphore of the receiving queue.");
	m_sendingQueueSemaphore = new TRAPZ_Semaphore(1, 1, "Semaphore of the sending queue");
	m_threadsRunningFlagSemaphore = new TRAPZ_Semaphore(1, 1, "Semaphore of the threads running flag.");
	m_threadsActiveFlagSemaphore = new TRAPZ_Semaphore(1, 1, "Semaphore of the threads active flag.");
	m_sendingQueueEntrySemaphore = new TRAPZ_Semaphore(0, TRAPZ_TCPConnection_MAX_SENDQUEUE + 1, "Semaphore of the threads running flag.");

	m_callbackMessageReceived = [](string a){}; //TODO test = 0
	m_callbackMessageSent = [](string a){};
	m_callbackConnectionClosed = [](){};
	m_callbackConnectionEstablished = [](){};

	initServerMode(host_addr, host_port);

	m_sendingThread = std::thread(&TRAPZ_TCPConnection::sendingThreadFunction, this);
	m_receivingThread = std::thread(&TRAPZ_TCPConnection::receivingThreadFunction, this);

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
TRAPZ_TCPConnection::TRAPZ_TCPConnection(string	host_addr, int host_port, string	remote_addr, int remote_port, bool use_async)
{
	m_mode = Mode::CLIENT;
	m_hostAddr = host_addr;
	m_hostPort = host_port;
	m_remoteAddr = remote_addr;
	m_remotePort = remote_port;
	m_asyncFlag = use_async;

	m_receivingQueueSemaphore = new TRAPZ_Semaphore(1, 1, "Semaphore of the receiving queue.");
	m_sendingQueueSemaphore = new TRAPZ_Semaphore(1, 1, "Semaphore of the sending queue");
	m_threadsRunningFlagSemaphore = new TRAPZ_Semaphore(1, 1, "Semaphore of the threads running flag.");
	m_threadsActiveFlagSemaphore = new TRAPZ_Semaphore(1, 1, "Semaphore of the threads active flag.");
	m_sendingQueueEntrySemaphore = new TRAPZ_Semaphore(0, TRAPZ_TCPConnection_MAX_SENDQUEUE + 1, "Semaphore of the threads running flag.");

	m_callbackMessageReceived = [](string a){};
	m_callbackConnectionClosed = [](){};
	m_callbackConnectionEstablished = [](){};

	initClientMode(host_addr, host_port, remote_addr, remote_port);

	m_sendingThread = std::thread(&TRAPZ_TCPConnection::sendingThreadFunction, this);
	m_receivingThread = std::thread(&TRAPZ_TCPConnection::receivingThreadFunction, this);

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
	if (m_callbackConnectionClosedDoneOneFlag){
		if (m_callbackConnectionClosedRecallFlag){
			m_callbackConnectionClosed();
		}
	}
	else{
		m_callbackConnectionClosed();
		m_callbackConnectionClosedDoneOneFlag = true;
	}
}

void TRAPZ_TCPConnection::changeAsyncFlag(bool flag)
{
	m_asyncFlag = flag;

	if (m_asyncFlag){
		startThreads();
	}
	else{
		stopThreads();
	}
}

void TRAPZ_TCPConnection::setCallback_MessageReceived(std::function<void(const string&)> callback_function)
{
	m_callbackMessageReceived = callback_function;
}

void TRAPZ_TCPConnection::setCallback_MessageSent(std::function<void(const string&)> callback_function)
{
	m_callbackMessageSent = callback_function;
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
	//m_asyncFlagSemaphore->enter(); //not needed because main process cant call changeAsyncFlag() while executing this method
	switch (m_asyncFlag)
	{
	case true://async receiving
		//up sending semaphore by one for each element in the queue, blocks if queue is full (TRAPZ_TCPConnection_MAX_SENDQUEUE)
		m_sendingQueueEntrySemaphore->enter();
		 
		//enter queue critical section
		m_sendingQueueSemaphore->enter();

			//add to sending queue
			m_sendingQueue.push_back(data);

		//leave queue critical section
		m_sendingQueueSemaphore->leave();

		break;
	case false://blocking receiving
		//send data over tcp socket
		sendString(data);
		break;
	}
	//m_asyncFlagSemaphore->leave();
}

string TRAPZ_TCPConnection::receiveData()
{
	string data = "";

	//m_asyncFlagSemaphore->enter();
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

		data += receiveStream();

		break;
	}

	//m_asyncFlagSemaphore->leave();

	return data;
}

TRAPZ_TCPConnection::Additional_Data* TRAPZ_TCPConnection::getData()
{
	return &m_additionalData;
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
	int result = inet_pton(AF_INET, host_addr.c_str(), &mSockAddress.sin_addr.s_addr);
	if (result == 0)
	{
		cleanup();
		throwMessage("inet_pton was not given a right address");
	} else if (result == -1)
	{
		cleanup();
		throwMessage(WSAGetLastErrorString());
	}
	mSockAddress.sin_port = htons(host_port);
	if (bind(m_socket, (struct sockaddr*)&mSockAddress, sizeof(mSockAddress)) == SOCKET_ERROR)
	{
		cleanup();
		throwMessage(WSAGetLastErrorString());
	}

	//connect socket
	setSocketBlocking();

	result = inet_pton(AF_INET, host_addr.c_str(), &mSockAddress.sin_addr.s_addr);
	if (result == 0)
	{
		cleanup();
		throwMessage("inet_pton was not given a right address");
	} else if (result == -1)
	{
		cleanup();
		throwMessage(WSAGetLastErrorString());
	}
	mSockAddress.sin_port = htons(remote_port);

	//loop until connected (blocks thread)
	bool connected = false;
	while (!connected)
	{
		if (connect(m_socket, (const sockaddr*)&mSockAddress, sizeof(mSockAddress)) == SOCKET_ERROR)
		{
			if (WSAGetLastError() != WSAETIMEDOUT)
			{
				cleanup();
				throwMessage(WSAGetLastErrorString());
			}
#if TRAPZ_COLLECTADDITIONALDATA
			m_additionalData.Count_ConnectionAttempts++;
#endif
		} else
		{
			connected = true;

			if (m_callbackConnectionEstablishedDoneOneFlag){
				if (m_callbackConnectionEstablishedRecallFlag){
					m_callbackConnectionEstablished();
				}
			}
			else{
				m_callbackConnectionEstablished();
				m_callbackConnectionEstablishedDoneOneFlag = true;
			}
			
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

	int result = inet_pton(AF_INET, host_addr.c_str(), &mSockAddress.sin_addr.s_addr);
	if (result == 0)
	{
		cleanup();
		throwMessage("inet_pton was not given a right address");
	} else if (result == -1)
	{
		cleanup();
		throwMessage(WSAGetLastErrorString());
	}
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

	if (m_callbackConnectionEstablishedDoneOneFlag){
		if (m_callbackConnectionEstablishedRecallFlag){
			m_callbackConnectionEstablished();
		}
	}
	else{
		m_callbackConnectionEstablished();
		m_callbackConnectionEstablishedDoneOneFlag = true;
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

			string message;
			//enter queue critical section
			m_sendingQueueSemaphore->enter();
			message = m_sendingQueue.front();
			m_sendingQueue.pop_front();
			//leave queue critical section
			m_sendingQueueSemaphore->leave();

			//send message
			sendString(message);
			m_callbackMessageSent(message);
				
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
			string data;
			//receive
			data += receiveStream();

			//put in queue
			m_receivingQueueSemaphore->enter();
			m_receivingQueue.push_back(data);
			m_receivingQueueSemaphore->leave();

			//call callback
			m_callbackMessageReceived(data);

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

void TRAPZ_TCPConnection::sendString(string message)
{
	setSocketBlocking();

	int bytes_sent = 0;
	int bytes_tosent = message.size();
	while (bytes_sent < bytes_tosent)
	{
		string remaining_message = message.substr(bytes_sent, bytes_tosent);
		int result = send(m_socket, remaining_message.c_str(), remaining_message.size() - bytes_sent, 0);

		//check for error
		if (result == SOCKET_ERROR) {
			//check for connection related errors and real errors
			switch (result)
			{
			case WSAENETDOWN:
			case WSAEACCES:
			case WSAEINTR:
			case WSAEINPROGRESS:
			case WSAEFAULT:
			case WSAENOTSOCK:
			case WSAEOPNOTSUPP:
			case WSAEMSGSIZE:
			case WSAEINVAL:
				cleanup();
				throwMessage(WSAGetLastErrorString());
				break;
			case WSAENOBUFS:
				//retry
				bytes_sent -= result; //cancel out adding of sent bytes
				break;
			case WSANOTINITIALISED:
			case WSAENETRESET:
			case WSAENOTCONN:
			case WSAESHUTDOWN:
			case WSAEHOSTUNREACH:
			case WSAECONNABORTED:
			case WSAECONNRESET:
			case WSAETIMEDOUT:
				if (m_callbackConnectionClosedDoneOneFlag){
					if (m_callbackConnectionClosedRecallFlag){
						m_callbackConnectionClosed();
					}
				}
				else{
					m_callbackConnectionClosed();
					m_callbackConnectionClosedDoneOneFlag = true;
				}
				switch (m_mode)
				{
				case Mode::SERVER:
					initServerMode(m_hostAddr, m_hostPort);
					break;
				case Mode::CLIENT:
					initClientMode(m_hostAddr, m_hostPort, m_remoteAddr, m_remotePort);
					break;
				}
				break;
			case WSAEWOULDBLOCK:
				//wont happen as socket is blocking (just for complete handling)
				break;
			}
		}
		bytes_sent += result;	//add sent bytes from last send to all sent bytes
	}
	//sent all the data
	
}

string TRAPZ_TCPConnection::receiveStream()
{
	string data;

	setSocketNonBlocking();

	//receive every message from the socket and add to data
	int result;
	char buf[TRAPZ_TCPConnection_RCVBUF_SIZE];
	memset(buf, '\0', TRAPZ_TCPConnection_RCVBUF_SIZE);
	while (result = recv(m_socket, buf, TRAPZ_TCPConnection_RCVBUF_SIZE, 0) > 0) //loop until no data received
	{
		//write local buffer to recv buffer
		data += string(buf);
	}

	if (result == SOCKET_ERROR)
	{
		int error = WSAGetLastError();
		//if error isnt about blocking
	
		switch (error)
		{
		case WSANOTINITIALISED:
		case WSAENETDOWN:
		case WSAEFAULT:
		case WSAEINTR:
		case WSAEINPROGRESS:
		case WSAENOTSOCK:
		case WSAEOPNOTSUPP:
		case WSAEMSGSIZE:
		case WSAEINVAL:
			cleanup();
			throwMessage(WSAGetLastErrorString());
			break;
		case WSAENOTCONN:
		case WSAENETRESET:
		case WSAESHUTDOWN:
		case WSAECONNABORTED:
		case WSAETIMEDOUT:
		case WSAECONNRESET:
			if (m_callbackConnectionClosedDoneOneFlag){
				if (m_callbackConnectionClosedRecallFlag){
					m_callbackConnectionClosed();
				}
			}
			else{
				m_callbackConnectionClosed();
				m_callbackConnectionClosedDoneOneFlag = true;
			}
			switch (m_mode)
			{
			case Mode::SERVER:
				initServerMode(m_hostAddr, m_hostPort);
				break;
			case Mode::CLIENT:
				initClientMode(m_hostAddr, m_hostPort, m_remoteAddr, m_remotePort);
				break;
			}
			break;
		case WSAEWOULDBLOCK: //do nothing (return)
			break;
		}
	}
	else //connection closed --> reconnect
		if (result == 0)
		{
			switch (m_mode)
			{
			case TRAPZ_TCPConnection::SERVER:
				initServerMode(m_hostAddr, m_hostPort);
				break;
			case TRAPZ_TCPConnection::CLIENT:
				initClientMode(m_hostAddr, m_hostPort, m_remoteAddr, m_remotePort);
				break;
			}
		}
	return data;
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






















