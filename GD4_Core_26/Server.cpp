#include "SocketWrapperPCH.hpp"
#include "Server.hpp"

int main()
{
	std::cout << "YOU IN THE SERVER BOAH" << std::endl;

	SocketUtil::StaticInit();
	UDPSocketPtr server_socket = SocketUtil::CreateUDPSocket(INET);
	server_socket->SetNonBlockingMode(false);
	Server server = Server(server_socket, "127.0.0.1");
	server.DoServiceLoop();
	server.ReceivePlayerInputBitStream(server_socket);
}

Server::Server(UDPSocketPtr server_socket, string server_address)
	:server_socket_(server_socket)
{
	SocketAddress server_address_sock = SocketAddress(SocketUtil::ConvertIPStringToInt(server_address), 50000);
	server_socket->Bind(server_address_sock);
}

void Server::DoServiceLoop()
{
	bool service_running = true;

	char receive_buffer[kMaxPacketSize];
	int bytes_received = 0;
	int requests = 0;
	SocketAddress sender_address;

	while (service_running)
	{
		bytes_received = server_socket_->ReceiveFrom(receive_buffer, sizeof(receive_buffer), sender_address);
		if (bytes_received > 0)
		{
			requests++;
			ProcessReceivedData(receive_buffer, bytes_received, sender_address, requests, service_running);
		}
	}
}

void Server::ProcessReceivedData(char* receive_buffer, int bytes_received, const SocketAddress& sender_address, int requests, bool service_running)
{
	std::cout << "Got " << bytes_received << " from " << sender_address.ToString() << std::endl;
	std::cout << "The message is: " << receive_buffer << std::endl;

	char response_data[kMaxPacketSize] = "";
	vector<string> array_tokens;
	string current_date_time = "";
	std::string requests_string = "";
	std::string quit_string = "Thank you for using the service";
	std::string echo_response = "";

	std::stringstream ss(receive_buffer);
	string temp;
	while (ss >> temp)
	{
		std::cout << temp << std::endl;
		array_tokens.emplace_back(temp);
	}
	Choice choice = static_cast<Choice>(stoi(array_tokens[0]));

	switch (choice)
	{
	case Choice::kEcho:
		std::cout << "Echo request" << std::endl;
		for (int i = 1; i < array_tokens.size(); ++i)
		{
			echo_response += array_tokens[i] + " ";
		}

		strcpy_s(response_data, echo_response.c_str());
		break;

	case Choice::kDateAndTime:
		std::cout << "DateAndTime request" << std::endl;
		current_date_time = ReturnCurrentDateAndTime();
		current_date_time.copy(response_data, current_date_time.length(), 0);
		break;

	case Choice::kStats:
		std::cout << "STATS request" << std::endl;
		requests_string = std::to_string(requests);
		requests_string.copy(response_data, requests_string.length(), 0);
		break;

	case Choice::kQuit:
		std::cout << "QUIT request" << std::endl;
		quit_string.copy(response_data, quit_string.length(), 0);
		break;
	}
	int bytes_sent = server_socket_->SendTo(response_data, sizeof(response_data), sender_address);
}

std::string Server::ReturnCurrentDateAndTime()
{
	auto now = std::chrono::system_clock::now();
	auto in_time_t = std::chrono::system_clock::to_time_t(now);

	std::stringstream ss;
	ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %X");
	return ss.str();
}

void Server::ReceivePlayerInputByteStream(const UDPSocketPtr server_socket)
{
	Player* receiver = new Player();
	SocketAddress sender_address;

	char* temporary_buffer = static_cast<char*>(std::malloc(kMaxPacketSize));
	std::cout << "Server waiting for input on the socket" << std::endl;
	int bytes_received = server_socket->ReceiveFrom(temporary_buffer, kMaxPacketSize, sender_address);
	InputMemoryStream in_stream(temporary_buffer, static_cast<uint32_t>(bytes_received));
	receiver->Read(in_stream);
	std::cout << "Received: " << bytes_received << std::endl;
	receiver->ToString();
	std::cin.ignore();

}

void Server::ReceivePlayerInputBitStream(const UDPSocketPtr server_socket)
{
	Player* receiver = new Player();
	SocketAddress sender_address;

	char* temporary_buffer = static_cast<char*>(std::malloc(kMaxPacketSize));
	std::cout << "Server waiting for input on the socket" << std::endl;
	int bytes_received = server_socket->ReceiveFrom(temporary_buffer, kMaxPacketSize, sender_address);
	std::cout << "Received: " << bytes_received << std::endl;
	InputMemoryBitStream in_stream(temporary_buffer, static_cast<uint32_t>(bytes_received*8));
	receiver->ReadBits(in_stream);
	std::cout << "Received: " << bytes_received << std::endl;
	receiver->ToString();
	std::cin.ignore();

}
