#include "SocketWrapperPCH.hpp"
#include "TCPSocket.hpp"

// Connects this TCP socket to a remote address
int TCPSocket::Connect(const SocketAddress& inAddress)
{
	// Attempt to connect using the underlying socket API
	int err = connect(mSocket, &inAddress.mSockAddr, inAddress.GetSize());

	// If connection failed
	if (err < 0)
	{
		// Report the error to console/log
		SocketUtil::ReportError("TCPSocket::Connect");
		// Return negative error code
		return -SocketUtil::GetLastError();
	}
	// Success
	return NO_ERROR;
}

// Start listening for incoming connections
int TCPSocket::Listen(int inBackLog)
{
	// Tell the OS to start listening with a backlog
	int err = listen(mSocket, inBackLog);

	// If listen failed
	if (err < 0)
	{
		SocketUtil::ReportError("TCPSocket::Listen");
		return -SocketUtil::GetLastError();
	}
	// Success
	return NO_ERROR;
}

// Accept a new incoming connection
TCPSocketPtr TCPSocket::Accept(SocketAddress& inFromAddress)
{
	// Length of the address structure
	socklen_t length = inFromAddress.GetSize();

	// Accept incoming connection
	SOCKET newSocket = accept(mSocket, &inFromAddress.mSockAddr, &length);

	if (newSocket != INVALID_SOCKET)
	{
		// Wrap new socket in a TCPSocket smart pointer and return
		return TCPSocketPtr(new TCPSocket(newSocket));
	}
	else
	{
		// Report failure
		SocketUtil::ReportError("TCPSocket::Accept");
		// Return nullptr on failure
		return nullptr;
	}
}

// Send data over this TCP socket
int32_t TCPSocket::Send(const void* inData, size_t inLen)
{
	// Use send() API
	int bytesSentCount = send(mSocket, static_cast<const char*>(inData), inLen, 0);

	if (bytesSentCount < 0)
	{
		SocketUtil::ReportError("TCPSocket::Send");
		return -SocketUtil::GetLastError();
	}
	return bytesSentCount;
}

// Receive data from this TCP socket
int32_t TCPSocket::Receive(void* inData, size_t inLen)
{
	// Use recv() API
	int bytesReceivedCount = recv(mSocket, static_cast<char*>(inData), inLen, 0);

	if (bytesReceivedCount < 0)
	{
		SocketUtil::ReportError("TCPSocket::Receive");
		return -SocketUtil::GetLastError();
	}
	return bytesReceivedCount;
}

// Bind this socket to a local address/port
int TCPSocket::Bind(const SocketAddress& inBindAddress)
{
	// Bind socket
	int error = bind(mSocket, &inBindAddress.mSockAddr, inBindAddress.GetSize());

	if (error != 0)
	{
		SocketUtil::ReportError("TCPSocket::Bind");
		return SocketUtil::GetLastError();
	}

	return NO_ERROR;
}

// Destructor cleans up the underlying socket
TCPSocket::~TCPSocket()
{
#if _WIN32
	// Windows uses closesocket()
	closesocket(mSocket);
#else
	// Unix/Linux uses close()
	close(mSocket);
#endif
}