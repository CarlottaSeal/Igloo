#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <WinSock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")

#include <algorithm>

#include "NetworkSystem.h"
#include "EngineCommon.hpp"
#include "Engine/Core/EventSystem.hpp"

extern EventSystem* g_theEventSystem;

NetworkSystem* g_theNetworkSystem = nullptr;

NetworkSystem::NetworkSystem(NetworkSystemConfig config)
    : m_config(config)
{
    m_mode = config.m_mode;
    m_serverAddress = config.m_serverAddress;
    m_serverPort = config.m_serverPort;
}

NetworkSystem::~NetworkSystem()
{
}

void NetworkSystem::Startup()
{
    WSAData data;
    if (WSAStartup(MAKEWORD(2, 2), &data))// Windows Sockets API Version 2.2 (0x00000202
    {
        ERROR_AND_DIE("Failed to start up network system!")
    }
    if (m_config.m_mode == NetworkMode::CLIENT)
    {
        //StartupClient();
        m_myClientConnection.m_socket = INVALID_SOCKET;
    }
    else if (m_config.m_mode == NetworkMode::SERVER)
    {
        //StartupServer();
    }
}

void NetworkSystem::Shutdown()
{
    if (m_myClientConnection.m_socket != INVALID_SOCKET)
    {
        closesocket(m_myClientConnection.m_socket);
        m_myClientConnection.m_socket = INVALID_SOCKET;
    }
    if (m_serverListenSocket != INVALID_SOCKET)
    {
        closesocket(m_serverListenSocket);
        m_serverListenSocket = INVALID_SOCKET;
    }
    for (auto client : m_externalClientSockets)
    {
        closesocket(client.m_socket);
    }
    m_externalClientSockets.clear();
    WSACleanup();
}

void NetworkSystem::BeginFrame()
{
    if (m_mode == NetworkMode::CLIENT)
    {
        BeginClient();
    }
    else if (m_mode == NetworkMode::SERVER)
    {
        BeginServer();
    }
}

void NetworkSystem::EndFrame()
{
    if (m_pendingDisconnect && m_mode == NetworkMode::CLIENT)
    {
        StopClient();
        m_pendingDisconnect = false;
    }
    if (m_pendingDisconnect && m_mode == NetworkMode::SERVER)
    {
        StopServer();
        m_pendingDisconnect = false;
    }
}

void NetworkSystem::SetMode(NetworkMode mode)
{
    m_mode = mode;
}

void NetworkSystem::SetClientConfig(std::string serverAddress, uint16_t serverPort)
{
    m_mode = NetworkMode::CLIENT;
    m_serverAddress = serverAddress;
    m_serverPort = serverPort;
}

void NetworkSystem::SetServerConfig(uint16_t serverPort)
{
    m_mode = NetworkMode::SERVER;
    //m_config.m_serverAddress = GetMyLocalIPAddress();
    m_serverPort = serverPort;
}

void NetworkSystem::SetServerAddress(std::string serverAddress)
{
    if (!serverAddress.empty())
        m_serverAddress = serverAddress;
}

void NetworkSystem::SetServerPort(uint16_t serverPort)
{
    m_serverPort = serverPort;
}

std::string NetworkSystem::GetMyLocalIPAddress() const
{
    char hostname[256];
    gethostname(hostname, sizeof(hostname));

    addrinfo hints = {};
    hints.ai_family = AF_INET;
    addrinfo* result = nullptr;
    if (getaddrinfo(hostname, NULL, &hints, &result) != 0)
        return "Unknown";

    sockaddr_in* addr = (sockaddr_in*)result->ai_addr;
    char ipStr[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(addr->sin_addr), ipStr, INET_ADDRSTRLEN);

    freeaddrinfo(result);
    return ipStr;
}

void NetworkSystem::StartServerOrClient()
{
    if (m_mode == NetworkMode::CLIENT)
    {
        g_theDevConsole->AddLine(Rgba8::MINTGREEN, "Start up the client!");
        StartupClient();
    }
    else if (m_mode == NetworkMode::SERVER)
    {
        g_theDevConsole->AddLine(Rgba8::MINTGREEN, "Start up the server!");
        StartupServer();
    }
}

bool NetworkSystem::HasConnected() const
{
    if (m_mode == NetworkMode::CLIENT)
    {
        return IsConnectedToServer();
    }
    if (m_mode == NetworkMode::SERVER)
    {
        return HasServerAcceptedClient();
    }
    return false;
}

bool NetworkSystem::IsConnectedToServer() const
{
    //return m_mode == NetworkMode::CLIENT && m_connectionState == 1;
    return m_connectionState == 1 && m_myClientConnection.m_socket != INVALID_SOCKET;
}

bool NetworkSystem::HasServerAcceptedClient() const
{
    return !m_externalClientSockets.empty();
}

void NetworkSystem::SendStringToServer(const std::string& message)
{
    if (m_mode != NetworkMode::CLIENT || m_myClientConnection.m_socket == INVALID_SOCKET)
        return;

    for (char c : message)
    {
        m_myClientConnection.m_outgoingDataBuffer.push_back((unsigned char)c);
    }
    m_myClientConnection.m_outgoingDataBuffer.push_back('\n');
}

void NetworkSystem::SendStringToClient(const std::string& message, int clientIndex)
{
    if (clientIndex < 0 || clientIndex >= (int)m_externalClientSockets.size())
        return;

    auto& conn = m_externalClientSockets[clientIndex];
    for (char c : message)
    {
        conn.m_outgoingDataBuffer.push_back((unsigned char)c);
    }
    conn.m_outgoingDataBuffer.push_back('\n');
}

void NetworkSystem::SendStringToRemote(const std::string& msg)
{
    if (m_mode == NetworkMode::CLIENT)
    {
        SendStringToServer(msg); 
    }
    else if (m_mode == NetworkMode::SERVER)
    {
        for (int i = 0; i < m_externalClientSockets.size(); i++)
        {
            SendStringToClient(msg, i); 
        }
    }
}

void NetworkSystem::StartupClient()
{
    //Create a client socket:
    m_myClientConnection.m_socket = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );		// SIMILAR
    m_connectionState = 0;

    //Set client connect socket to non-blocking:										
    unsigned long blockingMode = 1;									// SAME
    ioctlsocket( m_myClientConnection.m_socket, FIONBIO, &blockingMode );			// SAME
    //Begin connecting to a server:
    IN_ADDR addr;												// CLIENT-SPECIFIC
    int result = inet_pton( AF_INET, m_serverAddress.c_str(), &addr ); // server’s IP!	// CLIENT-SPECIFIC
    if (result != 1)
    {
        // 0 = invalid format, -1 = error (e.g. WSANOTINITIALISED)
        ERROR_AND_DIE("Failed to read IP address")
    }
    uint32_t serverIPAddressU32 = ntohl( addr.S_un.S_addr );				// SAME
    uint16_t serverPortU16 = m_serverPort;		// SAME
    sockaddr_in address;											// SAME
    address.sin_family = AF_INET;										// SAME
    address.sin_addr.S_un.S_addr = htonl( serverIPAddressU32 );				// SAME
    address.sin_port = htons( serverPortU16 );							// SAME
    int connectR = connect( m_myClientConnection.m_socket, (sockaddr*) (&address), (int) sizeof(address) );	// CLIENT-SPECIFIC
    if (connectR == SOCKET_ERROR)
    {
        int err = WSAGetLastError();
        if (err != WSAEWOULDBLOCK && err != WSAEINPROGRESS)
        {
            DebuggerPrintf("connect() FAILED: %d\n", err);
            //g_theDevConsole->AddLine(Rgba8::RED, "connect() FAILED: %d");
            return;
        }
    }
}

void NetworkSystem::StartupServer()
{
    //Create a listen socket
    m_serverListenSocket = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );
    //Set listen socket to non-blocking:	
    unsigned long blockingMode = 1;									
    ioctlsocket( m_serverListenSocket, FIONBIO, &blockingMode );
    //Bind the server listen socket to an incoming port:
    uint32_t myIPAddressU32 = INADDR_ANY;								
    uint16_t myListenPortU16 = m_serverPort;//static_cast<unsigned short>(atoi(m_config.m_serverAddress.c_str()));		
    sockaddr_in addr;
    addr.sin_family = AF_INET;										
    addr.sin_addr.S_un.S_addr = htonl( myIPAddressU32 );					
    addr.sin_port = htons( myListenPortU16 );						
    int result = bind( m_serverListenSocket, (sockaddr*) &addr, (int) sizeof(addr) );
    if (result == SOCKET_ERROR)
    {
        closesocket(m_serverListenSocket);
        m_serverListenSocket = INVALID_SOCKET;
        
        //DWORD errorCode = WSAGetLastError();
        ERROR_AND_DIE("Bind failed on port. Is the port already in use?")
    }
    //Listen for new incoming connections on this socket:
    listen( m_serverListenSocket, SOMAXCONN );			
}

void NetworkSystem::BeginClient()
{
    SOCKET client = static_cast<SOCKET>(m_myClientConnection.m_socket);

    if (client == INVALID_SOCKET)
    {
        m_connectionState = 0;
        return; //disconnected
    }

    if (m_connectionState ==0)
    {
        //BeginFrame, while trying to connect... check status of an ongoing connection attempt we started by calling connect() above…
        //FD stands for “File Descriptor” but really means “socket” in this context, and “set” here is just a C-style container like std::vector.
        fd_set writeSockets;	// a list of sockets that can be written-to			// CLIENT-SPECIFIC
        fd_set exceptSockets;	// a list of sockets with errors					// CLIENT-SPECIFIC
        FD_ZERO( &writeSockets );	// like std::vector.clear()					// CLIENT-SPECIFIC
        FD_ZERO( &exceptSockets );	// like std::vector.clear()					// CLIENT-SPECIFIC
        FD_SET( m_myClientConnection.m_socket, &writeSockets );	// like .push_back()		// CLIENT-SPECIFIC
        FD_SET( m_myClientConnection.m_socket, &exceptSockets );	// like .push_back()		// CLIENT-SPECIFIC
        timeval waitTime = { };										// CLIENT-SPECIFIC
        int selectReturn = select( 0, NULL, &writeSockets, &exceptSockets, &waitTime );			// CLIENT-SPECIFIC
        // select() gathers connection status information about each socket in both sets (writeable sockets, and error sockets).
        //1.	If the return code is SOCKET_ERROR the connection attempt failed and you should call connect again.
        //2.	If the return code is 0 do nothing and keep checking the socket status each frame.						
        if (selectReturn == SOCKET_ERROR)
        {
            int connectR = connect(m_myClientConnection.m_socket, (sockaddr*)&m_serverAddress, sizeof(m_serverAddress));
            if (connectR == SOCKET_ERROR)
            {
                int err = WSAGetLastError(); 
                if (err != WSAEWOULDBLOCK && err != WSAEINPROGRESS)
                {
                    //DebuggerPrintf("connect() failed: %d\n", err);
                }
                return;
            }
        }
        if (selectReturn > 0)
        {
            if (FD_ISSET( m_myClientConnection.m_socket, &exceptSockets ))				// CLIENT-SPECIFIC
            // If true (nonzero), this socket is in the list of error sockets; the connection failed!  You must call connect() again.
            {
                DisconnectClient(client); 
                m_connectionState = 0;
            }
            if (FD_ISSET( m_myClientConnection.m_socket, &writeSockets ))					// CLIENT-SPECIFIC		
            // If true (nonzero), this socket is in the list of writeable sockets; the connection succeeded!  You can now send() & recv().
            {
                g_theDevConsole->AddLine(Rgba8::GREEN, "Find a connection!");
                g_theEventSystem->FireEvent("connectsucceed");
                m_connectionState = 1;
                //SendAndReceive();
            }
        }
    }
    SendAndReceive(m_myClientConnection);

    //fire
    while (true)
    {
		auto it = std::find(m_myClientConnection.m_incomingDataBuffer.begin(), m_myClientConnection.m_incomingDataBuffer.end(), '\n');
		if (it == m_myClientConnection.m_incomingDataBuffer.end())
			return; // 没有完整字符串

		std::string cmd(m_myClientConnection.m_incomingDataBuffer.begin(), it);
		m_myClientConnection.m_incomingDataBuffer.erase(m_myClientConnection.m_incomingDataBuffer.begin(), it + 1);

		g_theDevConsole->Execute(cmd);
    }
}

void NetworkSystem::BeginServer()
{
	/*for (int i = 0; i < (int)m_externalClientSockets.size(); )
	{
		SOCKET s = m_externalClientSockets[i].m_socket;
		if (s == INVALID_SOCKET)
		{
			closesocket(s);
			m_externalClientSockets.erase(m_externalClientSockets.begin() + i);
			continue;
		}

		++i;
	}*/

    //BeginFrame, while waiting for a client to connect:
    uint64_t newClientSocket;
    newClientSocket = accept( m_serverListenSocket, NULL, NULL );				// SERVER-SPECIFIC
    // newClientSocket will continue to be set to INVALID_SOCKET until connected
    //If the above returned a valid client socket, set it to non-blocking:
    if ( newClientSocket != INVALID_SOCKET )
    {
        unsigned long blockingMode = 1;									
        ioctlsocket( newClientSocket, FIONBIO, &blockingMode );
        NetworkConnection connect;
        connect.m_socket = newClientSocket;
        m_externalClientSockets.push_back( connect );

        g_theDevConsole->AddLine(Rgba8::GREEN, "Accept a connection");
        g_theEventSystem->FireEvent("connectsucceed");
    }
    else
    {
        //DebuggerPrintf("Failed to accept connection");
        //return;
    }
    // Add this new socket to the server’s list of connected clients
    
    //BeginFrame, while connected:
	for (int i = 0; i < (int)m_externalClientSockets.size(); i++)
	{
		NetworkConnection& conn = m_externalClientSockets[i];
		SendAndReceive(conn);
	}
    // If the result is WSAWOULDBLOCK then there is nothing to send or receive right now

    for (auto& conn : m_externalClientSockets)
    {
        SOCKET senderSocket = conn.m_socket;

        while (true)
        {
			auto it = std::find(conn.m_incomingDataBuffer.begin(), conn.m_incomingDataBuffer.end(), '\n');
			if (it == conn.m_incomingDataBuffer.end())
				break; // 没有完整字符串

			std::string cmd(conn.m_incomingDataBuffer.begin(), it);
			conn.m_incomingDataBuffer.erase(conn.m_incomingDataBuffer.begin(), it + 1);

			g_theDevConsole->Execute(cmd);

			for (auto& otherConn : m_externalClientSockets)
			{
				if (otherConn.m_socket == senderSocket)
					continue;

				for (char c : cmd)
				{
					otherConn.m_outgoingDataBuffer.push_back((unsigned char)c);
				}
				otherConn.m_outgoingDataBuffer.push_back('\n');
			}
        }
    }
}

void NetworkSystem::SendAndReceive(NetworkConnection& conn)
{
    SOCKET client = static_cast<SOCKET>(conn.m_socket);
	//if (client == INVALID_SOCKET || client == SOCKET_ERROR || client > 0xFFFF)
	//	return;

    //send
    while (!conn.m_outgoingDataBuffer.empty())
    {
        char sendBuffer[2048];
        int maxSend = std::min<int>((int)conn.m_outgoingDataBuffer.size(), (int)sizeof(sendBuffer));
		if ((int)conn.m_outgoingDataBuffer.size() < maxSend)
		{
			HandleConnectionError(client);
			return;
		}

        for (int i = 0; i < maxSend; ++i)
            sendBuffer[i] = (char)conn.m_outgoingDataBuffer[i];

        int sent = send(client, sendBuffer, maxSend, 0);
        if (sent > 0)
        {
            for (int i = 0; i < sent; ++i)
                conn.m_outgoingDataBuffer.pop_front();
        }
        else if (WSAGetLastError() == WSAEWOULDBLOCK)
        {
            break; //等下一帧
        }
        else
        {
            HandleConnectionError(client);
            //DisconnectClient(client);
            return;
        }
    }

    //receive
    char recvBuffer[2048];
    int received = recv(client, recvBuffer, sizeof(recvBuffer), 0);
    if (received > 0)
    {
        for (int i = 0; i < received; ++i)
            conn.m_incomingDataBuffer.push_back((unsigned char)recvBuffer[i]);
    }
    else if (received == 0 || WSAGetLastError() != WSAEWOULDBLOCK)
    {
        //DisconnectClient(client);
        HandleConnectionError(client);
        return;
    }
}

void NetworkSystem::HandleConnectionError(uint64_t socket)
{
    if (m_mode == NetworkMode::SERVER)
    {
        DisconnectClient(socket); // kick out!
    }
    else if (m_mode == NetworkMode::CLIENT)
    {
        if (m_myClientConnection.m_socket == socket)
        {
            Disconnect(); // kick myself
        }
    }
}

void NetworkSystem::DisconnectClient(uint64_t client)  //kick out one client
{
    if (m_mode == NetworkMode::SERVER)
    {
        m_externalClientSockets.erase(
        	std::remove_if(
        		m_externalClientSockets.begin(),
        		m_externalClientSockets.end(),
        		[&](const NetworkConnection& conn) { return conn.m_socket == client; }
        	),
        	m_externalClientSockets.end()
        );
    }
    // if (m_mode == NetworkMode::CLIENT)
    // {
    //     if (m_myClientConnection.m_socket != INVALID_SOCKET)
    //     {
    //         closesocket(m_myClientConnection.m_socket);
    //         m_myClientConnection.m_socket = INVALID_SOCKET;
    //     }
    //     m_connectionState = 0;
    // }
}

void NetworkSystem::Disconnect(uint64_t socket)
{
    if (m_mode == NetworkMode::CLIENT)
    {
		/*if (m_myClientConnection.m_socket != INVALID_SOCKET)
		{
			closesocket(m_myClientConnection.m_socket);
			m_myClientConnection.m_socket = INVALID_SOCKET;
		}*/
		m_connectionState = 0;
    }
    if (m_mode == NetworkMode::SERVER)
    {
        //StopServer();
		/*if (m_serverListenSocket != INVALID_SOCKET)
		{
			closesocket(m_serverListenSocket);
			m_serverListenSocket = INVALID_SOCKET;
		}
		for (auto& client : m_externalClientSockets)
		{
			closesocket(client.m_socket);
		}
		m_externalClientSockets.clear();*/
		if (socket != INVALID_SOCKET && m_serverListenSocket != INVALID_SOCKET)
		{
			for (auto it = m_externalClientSockets.begin(); it != m_externalClientSockets.end(); ++it)
			{
				if (it->m_socket == socket)
				{
					closesocket(it->m_socket);
					g_theDevConsole->AddLine(Rgba8::YELLOW, Stringf("Disconnected client socket: %llu", socket));
					m_externalClientSockets.erase(it);


					return;
				}
			}
		}
    }
}

void NetworkSystem::StopServer()
{
	if (m_serverListenSocket != INVALID_SOCKET)
	{
		closesocket(m_serverListenSocket);
		m_serverListenSocket = INVALID_SOCKET;
	}
	for (auto& client : m_externalClientSockets)
	{
		closesocket(client.m_socket);
		closesocket(client.m_socket);
	}
	m_externalClientSockets.clear();
}

void NetworkSystem::StopClient()
{
	if (m_myClientConnection.m_socket != INVALID_SOCKET)
	{
		closesocket(m_myClientConnection.m_socket);
		m_myClientConnection.m_socket = INVALID_SOCKET;
	}
	m_connectionState = 0;
}
