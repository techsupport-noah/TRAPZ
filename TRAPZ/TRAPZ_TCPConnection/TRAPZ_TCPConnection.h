/* Setup */
#define TRAPZ_COLLECTADDITIONALDATA 1

/* Config */
#define TRAPZ_TCPConnection_MAX_SENDQUEUE 1000
#define TRAPZ_TCPConnection_RCVBUF_SIZE 4096
#define TRAPZ_TCPConnection_SNDBUF_SIZE 4096

/* C++ includes */
#include <string>		//includes std::string
#include <functional>	//includes std::function
#include <thread>		//includes std::thread
#include <deque>		//includes std::deque
#include <WinSock2.h>	//includes SOCKET
#include <stdexcept>	//includes std::runtime_error

/* Custom includes */
#include "../TRAPZ_Semaphore/TRAPZ_Semaphore.h"

/* Using */
using std::string;

/* Forward declarations */


class TRAPZ_TCPConnection{

public:
									/* Server mode */
									TRAPZ_TCPConnection					(string							host_addr			,			
																		 int							host_port			, 
																		 bool							use_async			= true);
									/* Client mode */
									TRAPZ_TCPConnection					(string							host_addr			,
																		 int							host_port			,
																		 string							remote_addr			,
																		 int							remote_port			,
																		 bool							use_async			= true);
									~TRAPZ_TCPConnection				();

	void							changeAsyncFlag						(bool							flag);

	void							setCallback_MessageReceived			(std::function<void(string)>	callback_function);
	void							setCallback_ConnectionClosed		(std::function<void(void)>		callback_function,
																		 bool							recall				= true); //recalls each time the connection gets closed
	void							setCallback_ConnectionEstablished	(std::function<void(void)>		callback_function,
																		 bool							recall				= true); //recalls each time the connection gets established (reconnects)

	void							sendData							(string							data);

	//If there is no data, "" is returned
	string							receiveData							();

#ifdef TRAPZ_COLLECTADDITIONALDATA
	struct Additional_Data
	{
		int			Count_ConnectionAttempts = 0;
		int			Count_ConnectionRequests = 0;
		int			Count_ConnectionDisconnects = 0;
		int			Count_SentBytes = 0;
		int			Count_ReceivedBytes = 0;
	};
	Additional_Data*					getData								();
	/* Additional Data */
	
	Additional_Data					m_additionalData;
#endif

private:

	enum Mode{
		SERVER,
		CLIENT
	};

	void							sendMessage							(string							message);

	void							initClientMode						(string							host_addr,
																		 int							host_port,
																		 string							remote_addr,
																		 int							remote_port);
	void							initServerMode						(string							host_addr,
																		 int							host_port);

	bool							m_asyncFlag;
	TRAPZ_Semaphore*				m_asyncFlagSemaphore;

	/* Intern functions */
	int								sendingThreadFunction				();
	int								receivingThreadFunction				();

	void							startThreads						();
	void							stopThreads							();
	void							cleanup								();

	/* Socket utility */
	void							setSocketBlocking();
	void							setSocketNonBlocking();
	void							setSocketOption						(int							level,
																		 int							option,
																		 int							value,
																		 SOCKET							socket);
	string							WSAGetLastErrorString				();
	void							throwMessage						(string							message);

	/* Thread handles. */
	std::thread						m_receivingThread;
	std::thread						m_sendingThread;
	
	/* Thead data. */
	bool							m_threadsRunningFlag = false;
	TRAPZ_Semaphore*				m_threadsRunningFlagSemaphore;
	bool							m_threadsActiveFlag = false;
	TRAPZ_Semaphore*				m_threadsActiveFlagSemaphore;
	TRAPZ_Semaphore*				m_sendingQueueEntrySemaphore;

	/* Thread queues. */
	TRAPZ_Semaphore*				m_receivingQueueSemaphore;
	std::deque<string>				m_receivingQueue;
	TRAPZ_Semaphore*				m_sendingQueueSemaphore;
	std::deque<string>				m_sendingQueue;

	/* Callback handles. */
	std::function<void(string)>		m_callbackMessageReceived;
	bool							m_callbackConnectionClosedRecallFlag;
	std::function<void(void)>		m_callbackConnectionClosed;
	bool							m_callbackConnectionEstablishedRecallFlag;
	std::function<void(void)>		m_callbackConnectionEstablished;

	/* Intern Data */
	SOCKET							m_socket;
	SOCKET							m_listeningSocket;
	string							m_hostAddr							= "unset";
	string							m_hostPort							= "unset";
	string							m_remoteAddr						= "unset";
	string							m_remotePort						= "unset";
	Mode							m_mode;

	


};

