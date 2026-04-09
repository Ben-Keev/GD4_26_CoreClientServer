#include "SocketWrapperPCH.hpp"       // Pre-compiled header wrapping SFML network types
#include "lobby_state.hpp"
#include "utility.hpp"

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Network/Packet.hpp>
#include <SFML/Network/IpAddress.hpp>
#include <fstream>

// ---------------------------------------------------------------------------
// GetAddressFromFile
// Reads the target server IP from "ip.txt" in the working directory.
// If the file does not exist or is unreadable, creates the file and writes
// localhost (127.0.0.1) as the default, then returns that address.
// Returns an optional sf::IpAddress in case resolution fails.
// ---------------------------------------------------------------------------
sf::IpAddress GetAddressFromFile()
{
	{
		// Attempt to open an existing ip.txt file
		std::ifstream input_file("ip.txt");
		std::string ip_address;
		if (input_file >> ip_address)
		{
			// resolve() handles both dotted-decimal IPs and hostnames
			if (auto address = sf::IpAddress::resolve(ip_address))
			{
				return *address;
			}
		}
	}
	// File was missing or contained an unresolvable address — create a fresh one
	std::ofstream output_file("ip.txt");
	sf::IpAddress local_address = sf::IpAddress::LocalHost;
	output_file << local_address.toString();
	return local_address;
}

LobbyState::LobbyState(StateStack& stack, Context context)
	: State(stack, context)
	, m_broadcast_text(context.fonts->Get(FontID::kMain))
	, m_lobby_countdown_text(context.fonts->Get(FontID::kMain))
	, m_failed_connection_text(context.fonts->Get(FontID::kMain))
	, m_players_connected_text(context.fonts->Get(FontID::kMain))
	, m_window(*context.window)
	, m_failed_connection_clock()
	, m_connected(false)
	, m_connected_players(0)                          // Number of connections as last reported by the server...
	, m_heartbeat_timer(sf::Time::Zero)
	, m_client_timeout(sf::seconds(1.f))        // Disconnect after 1s with no packet
	, m_time_since_last_packet(sf::seconds(0.f))// Accumulator for the above timeout
{
	// Broadcast messages appear centred near the top of the screen
	m_broadcast_text.setPosition(sf::Vector2f(1024.f / 2, 100.f));

	m_players_connected_text.setCharacterSize(35);
	m_players_connected_text.setFillColor(sf::Color::White);
	m_players_connected_text.setString(std::to_string(m_connected_players) + " players are connected");

	Utility::CentreOrigin(m_players_connected_text);
	m_players_connected_text.setPosition(
		sf::Vector2f(m_window.getSize().x / 2.f, m_window.getSize().y / 2.f));

	m_lobby_countdown_text.setCharacterSize(35);
	m_lobby_countdown_text.setFillColor(sf::Color::White);
	m_lobby_countdown_text.setString("Waiting for players...");
	Utility::CentreOrigin(m_lobby_countdown_text);
	m_lobby_countdown_text.setPosition(
		sf::Vector2f(m_window.getSize().x / 2.f, m_window.getSize().y / 2.f - 50.f));

	// Configure the connection-status text shown while connecting / on failure
	m_failed_connection_text.setCharacterSize(35);
	m_failed_connection_text.setFillColor(sf::Color::White);
	m_failed_connection_text.setString("Attempting to connect...");
	Utility::CentreOrigin(m_failed_connection_text);
	m_failed_connection_text.setPosition(
		sf::Vector2f(m_window.getSize().x / 2.f, m_window.getSize().y / 2.f));

	// Render one frame immediately so the user sees "Attempting to connect..."
	// rather than a black screen during the potentially blocking connect() call
	m_window.clear(sf::Color::Black);
	m_window.draw(m_failed_connection_text);
	m_window.display();

	// Pre-set the failure string now; it will be shown if connect() fails
	m_failed_connection_text.setString("Failed to connect");
	Utility::CentreOrigin(m_failed_connection_text);

	// Read the server address from disk
	std::optional<sf::IpAddress> ip;
	ip = GetAddressFromFile();

	if (ip)
	{
		// Try to establish a TCP connection — 5 second timeout for the handshake
		auto status = m_socket.connect(*ip, SERVER_PORT, sf::seconds(5.f));

		if (status == sf::Socket::Status::Done)
		{
			m_connected = true;  // Handshake succeeded
		}
		else
		{
			// Connection failed; start the 5-second "return to menu" countdown
			m_failed_connection_clock.restart();
		}
	}
	else
	{
		// IP resolution failed; start the countdown to return to the menu
		m_failed_connection_clock.restart();
	}

	// Switch to non-blocking mode now that the (blocking) connect() is done.
	// All subsequent receive() calls must return immediately so Update() doesn't stall.
	m_socket.setBlocking(false);
}

bool LobbyState::Update(sf::Time delta_time)
{
	if (m_connected) {
		// --- Receive server packets ---
		sf::Packet packet;
		if (m_socket.receive(packet) == sf::Socket::Status::Done)
		{

			//std::cout << "Packet received!" << std::endl;

			// Reset the timeout accumulator — server is still alive
			m_time_since_last_packet = sf::seconds(0.f);
			uint8_t packet_type;
			packet >> packet_type;

			//std::cout << "Packet type: " << (int)packet_type << std::endl;

			HandlePacket(packet_type, packet);
		}
		else
		{
			// No packet arrived this frame; check for timeout
			if (m_time_since_last_packet > m_client_timeout)
			{
				m_connected = false;
				m_failed_connection_text.setString("Lost connection to the server");
				Utility::CentreOrigin(m_failed_connection_text);
				m_failed_connection_clock.restart();  // Begin the 5-second return-to-menu countdown
			}
		}

		// Heartbeat — send every 500ms to prevent server timeout (Claude)
		m_heartbeat_timer += delta_time;
		if (m_heartbeat_timer >= sf::seconds(0.5f))
		{
			sf::Packet heartbeat;
			heartbeat << static_cast<uint8_t>(Client::PacketType::kHeartbeat);
			m_socket.send(heartbeat);
			m_heartbeat_timer = sf::Time::Zero;
		}

		// Show the current broadcast message if the queue is non-empty
		if (!m_broadcasts.empty())
		{
			m_window.draw(m_broadcast_text);
		}

		// Accumulate time since the last received packet to detect server timeout
		m_time_since_last_packet += delta_time;

		// Advance the broadcast message display timer
		UpdateBroadcastMessage(delta_time);
	}
	else if (m_failed_connection_clock.getElapsedTime() >= sf::seconds(5.f))
	{
		// After 5 seconds of showing the failure message, return to the main menu
		RequestStackClear();
		RequestStackPush(StateID::kMenu);
	}

	return true;
}

void LobbyState::Draw()
{
	if (m_connected) 
	{
		m_window.draw(m_players_connected_text);
		m_window.draw(m_lobby_countdown_text);

		// Show the current broadcast message if the queue is non-empty
		if (!m_broadcasts.empty())
		{
			m_window.draw(m_broadcast_text);
		}
	}
	else
	{
		// Not connected — show error or "attempting to connect" text
		m_window.draw(m_failed_connection_text);
	}
}

bool LobbyState::HandleEvent(const sf::Event& event)
{

	const auto* key_pressed = event.getIf<sf::Event::KeyPressed>();
	if (key_pressed)
	{
		// Backspace exits the lobby and returns to the main menu.
		if (key_pressed->scancode == sf::Keyboard::Scancode::Backspace)
		{
			RequestStackClear();
			RequestStackPush(StateID::kMenu);
		}
	}

	return true;
}

void LobbyState::HandlePacket(uint8_t packet_type, sf::Packet& packet)
{
	switch (static_cast<Server::PacketType>(packet_type))
	{
	case Server::PacketType::kLobbyCountdownReset:
	{

		std::cout << "Lobby countdown was reset" << std::endl;
		float countdown;
		uint8_t connected_players;

		// Server has reset the lobby countdown (e.g. a new player joined)
		packet >> countdown >> connected_players;
		m_players_connected_text.setString(std::to_string(connected_players) + " players are connected");
		Utility::CentreOrigin(m_players_connected_text);

		m_connected_players = connected_players;

		UpdateCountdownText(countdown);
	}
	break;
	case Server::PacketType::kLobbyPing:
	{
		float countdown;

		// Server has reset the lobby countdown (e.g. a new player joined)
		packet >> countdown;

		UpdateCountdownText(countdown);
	}
	break;
	case Server::PacketType::kBroadcastMessage:
	{
		std::string message;
		packet >> message;
		m_broadcasts.push_back(message);

		// If this is the first message in the queue, display it immediately
		if (m_broadcasts.size() == 1)
		{
			m_broadcast_text.setString(m_broadcasts.front());
			Utility::CentreOrigin(m_broadcast_text);
			m_broadcast_elapsed_time = sf::Time::Zero;
		}
	}
	break;
	case Server::PacketType::kGameStart:
	{
		std::cout << "Game start packet was received" << std::endl;

		RequestStackPop();
		// Server has started the game — transition to the multiplayer game state
		RequestStackPush(StateID::kJoinGame);
	}
	break;
	}
}

void LobbyState::UpdateCountdownText(float countdown) 
{
	if (m_connected_players > 1)
	{
		m_lobby_countdown_text.setString(std::to_string(countdown) + " seconds remaining");
		Utility::CentreOrigin(m_lobby_countdown_text);
	}
	else
	{
		m_lobby_countdown_text.setString("Waiting for players...");
		Utility::CentreOrigin(m_lobby_countdown_text);
	}
}

// ---------------------------------------------------------------------------
// UpdateBroadcastMessage
// Ticks the display timer for the current broadcast message.
// Each message is shown for 2 seconds; when it expires it is dequeued and
// the next message in the queue (if any) begins displaying immediately.
// ---------------------------------------------------------------------------
void LobbyState::UpdateBroadcastMessage(sf::Time elapsed_time)
{
	if (m_broadcasts.empty())
		return;

	m_broadcast_elapsed_time += elapsed_time;
	if (m_broadcast_elapsed_time > sf::seconds(2.f))
	{
		// Current message has expired — remove it from the front of the queue
		m_broadcasts.erase(m_broadcasts.begin());

		// If more messages are waiting, display the next one immediately
		if (!m_broadcasts.empty())
		{
			m_broadcast_text.setString(m_broadcasts.front());
			Utility::CentreOrigin(m_broadcast_text);
			m_broadcast_elapsed_time = sf::Time::Zero;
		}
	}
}