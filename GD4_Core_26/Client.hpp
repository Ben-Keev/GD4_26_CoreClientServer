#pragma once
class Client
{
public:
	Client(UDPSocketPtr client_socket, string server_address);
	void DoServiceLoop();

private:	
	void PrintOptions();
	void GetChoice(std::string& choice);
	void Echo();
	void SendDataToServer(Choice choice, std::string& to_send);
	void SendDataToServer(Choice choice);
	void GetDateAndTime();
	void GetStats();
	void Quit();

	void ReceiveDataFromServer(char* receive_buffer, SocketAddress sender_address, int bytes_received, bool& service_running);

	void ProcessReceivedData(char* receive_buffer, int bytes_received, SocketAddress sender_address, bool& service_running);

private:
	UDPSocketPtr client_socket_;
	SocketAddress server_address_;
};

