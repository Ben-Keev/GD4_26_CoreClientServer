#include "SocketWrapperPCH.hpp"
#include "Client.hpp"
#include "application.hpp"

/* Client should offer the user a menu where they can select
1. Echo
2. DateAndTime
3. Stats
4. Quit
Take input from the user, send it to the server and listen and display the result
*/

int main()
{
	std::cout << "YOU IN THE CLIENT BOAH" << std::endl;

	//Player* new_player = new Player();

	//std::cout << "Size of a player is: " << sizeof(new_player) << std::endl;

	//SocketUtil::StaticInit();

	//UDPSocketPtr client_socket = SocketUtil::CreateUDPSocket(INET);

	//client_socket->SetNonBlockingMode(false);
	//Client client = Client(client_socket, "127.0.0.1");

	// RUN THE CA1 GAME ====================================================================

	sf::Joystick::update();

	try
	{
		Application app;
		app.Run();
	}
	catch (std::runtime_error& e)
	{
		std::cout << e.what() << std::endl;
	}

	// RUN THE CA1 GAME ====================================================================

	//client.DoServiceLoop();
	//client.SendPlayerOutputBitStream(client_socket, new_player);
	//SocketUtil::CleanUp();
}

Client::Client(UDPSocketPtr client_socket, string server_address)
	: client_socket_(client_socket)
{
	server_address_ = SocketAddress(SocketUtil::ConvertIPStringToInt(server_address), 50000);
}

void Client::DoServiceLoop()
{
	bool service_running = true;
	string choice;
	SocketAddress sender_address;
	int bytes_received = 0;

	char receive_buffer[kMaxPacketSize];
	char* begin = receive_buffer;
	char* end = begin + sizeof(receive_buffer);
	std::fill(begin, end, 0);

	while (service_running)
	{
		PrintOptions();
		GetChoice(choice);
		switch (static_cast<Choice>(stoi(choice)-1))
		{
		case Choice::kEcho:
			Echo();
			break;
		case Choice::kDateAndTime:
			GetDateAndTime();
			break;
		case Choice::kStats:
			GetStats();
			break;
		case Choice::kQuit:
			Quit();
			return;
		}
		ReceiveDataFromServer(receive_buffer, sender_address, bytes_received, service_running);
	}
}

void Client::PrintOptions()
{
	std::cout << "Please enter: " << std::endl;
	std::cout << "1) To use the ECHO service: " << std::endl;
	std::cout << "2) To use the DateAndTime service: " << std::endl;
	std::cout << "3) To use the Stats service: " << std::endl;
	std::cout << "4) To use the Quit:" << std::endl;
}

void Client::GetChoice(std::string& choice)
{
	std::getline(std::cin, choice);
}

void Client::Echo()
{
	string echo;
	std::cout << "What would you like to echo: " << std::endl;
	std::getline(std::cin, echo);
	SendDataToServer(Choice::kEcho, echo);
}

void Client::SendDataToServer(Choice choice, std::string& to_send)
{
	int choice_int = static_cast<int>(choice);
	string choice_string = std::to_string(choice_int);
	string to_send_combined = choice_string + " " + to_send;
	int bytes_sent = client_socket_->SendTo(to_send_combined.c_str(), sizeof(to_send_combined), server_address_);
}

void Client::SendDataToServer(Choice choice)
{
	int choice_int = static_cast<int>(choice);
	string choice_string = std::to_string(choice_int);
	std::cout << "Sending " << choice_string << std::endl;
	int bytes_sent = client_socket_->SendTo(choice_string.c_str(), sizeof(choice_string), server_address_);
}


void Client::GetDateAndTime()
{
	SendDataToServer(Choice::kDateAndTime);
}

void Client::GetStats()
{
	SendDataToServer(Choice::kStats);
}

void Client::Quit()
{
	SendDataToServer(Choice::kQuit);
}

void Client::ReceiveDataFromServer(char* receive_buffer, SocketAddress sender_address, int bytes_received, bool& service_running)
{
	std::cout << sizeof(receive_buffer) << std::endl;
	bytes_received = client_socket_->ReceiveFrom(receive_buffer, kMaxPacketSize, sender_address);
	if (bytes_received > 0)
	{
		ProcessReceivedData(receive_buffer, bytes_received, sender_address, service_running);
	}
}

void Client::ProcessReceivedData(char* receive_buffer, int bytes_received, SocketAddress sender_address, bool& service_running)
{
	char key[] = "QUIT";

	if (strcmp(key, receive_buffer) == 0)
	{
		std::cout << "Server says we need to shut down...." << std::endl;
		service_running = false;
	}

	std::cout << "Got " << bytes_received << " from " << sender_address.ToString() << std::endl;
	std::cout << "The message is: " << receive_buffer << std::endl;
}

void Client::SendPlayerOutputByteStream(UDPSocketPtr client_socket, Player* new_player)
{
	//SocketAddress server_address = SocketAddress(SocketUtil::ConvertIPStringToInt("127.0.0.1"), 50000);
	//OutputMemoryStream out_stream;
	//new_player->Write(out_stream);
	//int bytes_sent = client_socket->SendTo(out_stream.GetBufferPtr(), out_stream.GetLength(), server_address);
	//std::cout << "Sent: " << bytes_sent << std::endl;
	//std::cin.ignore();
}

void Client::SendPlayerOutputBitStream(UDPSocketPtr client_socket, Player* new_player)
{
	//SocketAddress server_address = SocketAddress(SocketUtil::ConvertIPStringToInt("127.0.0.1"), 50000);
	//OutputMemoryBitStream out_stream;
	//new_player->Write(out_stream);
	//int bytes_sent = client_socket->SendTo(out_stream.GetBufferPtr(), out_stream.GetByteLength(), server_address);
	//std::cout << "Sent: " << bytes_sent << std::endl;
	//std::cin.ignore();
}




