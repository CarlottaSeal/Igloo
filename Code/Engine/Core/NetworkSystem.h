#pragma once
#include <cstdint>
#include <deque>
#include <string>
#include <vector>

enum class NetworkMode
{
    CLIENT,
    SERVER,
    COUNT
};
struct NetworkConnection
{
    uint64_t m_socket;
    
    std::deque<unsigned char> m_incomingDataBuffer; 
    std::deque<unsigned char> m_outgoingDataBuffer; 
};

struct NetworkSystemConfig
{
    NetworkMode m_mode = NetworkMode::CLIENT;
    std::string m_serverAddress = "127.0.0.1";
    uint16_t m_serverPort = 12345;
};

class NetworkSystem
{
public:
    NetworkSystem(NetworkSystemConfig config);
    ~NetworkSystem();

    void Startup();
    void Shutdown();
    void BeginFrame();
    void EndFrame();

protected:
    NetworkSystemConfig m_config;
    NetworkMode m_mode;
    std::string m_serverAddress;
    uint16_t m_serverPort;
    
    //if I am a client, i have...
    NetworkConnection m_myClientConnection;
    // uint64_t m_myClientSocket;
    // std::deque<unsigned char> m_incomingDataForMe;  //game code does .pop_front to receive
    // std::deque<unsigned char> m_outgoingDataForMe;  //game code does .pop_back to receive
//uint8_t is the same as unsigned char
    
    //If iam a server, i have...
    uint64_t m_serverListenSocket;
    //std::vector<uint64_t> m_externalClientSockets;
    std::vector<NetworkConnection> m_externalClientSockets;

    int m_connectionState =0;

public:
    void SetMode(NetworkMode mode);
    void SetClientConfig(std::string serverAddress, uint16_t serverPort);
    void SetServerConfig(uint16_t serverPort);
    void SetServerAddress(std::string serverAddress);
    void SetServerPort(uint16_t serverPort);
    std::string GetMyLocalIPAddress() const;
    void StartServerOrClient();
    bool HasConnected() const;
    void SendStringToRemote(const std::string& msg);
    NetworkMode GetNetworkMode() const { return m_mode; }
    std::string GetServerAddress() const { return m_serverAddress; }
    uint16_t GetServerPort() const { return m_serverPort; }
    NetworkConnection GetMyClientConnection() const { return m_myClientConnection; } //for client
    void DisconnectClient(uint64_t client);
    void Disconnect(uint64_t socket = 0xFFFFFFFFFFFFFFFF);
    void StopServer();
    void StopClient();

    bool m_pendingDisconnect = false;

private:
    void StartupClient();
    void StartupServer();
    void BeginClient();
    void BeginServer();

    void SendAndReceive(NetworkConnection& conn);
    void HandleConnectionError(uint64_t socket);
	void SendStringToServer(const std::string& message);
	void SendStringToClient(const std::string& message, int clientIndex = 0);
	bool IsConnectedToServer() const;
	bool HasServerAcceptedClient() const;
};

