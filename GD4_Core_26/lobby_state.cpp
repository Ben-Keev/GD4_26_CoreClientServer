#include "lobby_state.hpp"
#include "utility.hpp"

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Network/Packet.hpp>
#include <SFML/Network/IpAddress.hpp>
#include <fstream>

#include "button.hpp"

// (Kaylon)
std::string LoadPlayerName();
int LoadHighScore();
void SaveDetails(const std::string& name, int high_score);

/// <summary>
/// Read IP Address from file
/// Modified: Ben (Moved from multiplayer_gamestate.cpp)
/// </summary>
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

/// <summary>
/// Lobby state between menu and multiplayer game state
/// Authored: Ben Mc Keever
/// </summary>
/// <param name="firstTime">Joining the lobby for the first time from the meny, or rejoining lobby from a game</param>
LobbyState::LobbyState(StateStack& stack, Context context, bool firstTime)
	: State(stack, context)
	, m_background_sprite(context.textures->Get(TextureID::kLobbyScreen))
	, m_broadcast_text(context.fonts->Get(FontID::kMain))
	, m_lobby_countdown_text(context.fonts->Get(FontID::kMain))
	, m_failed_connection_text(context.fonts->Get(FontID::kMain))
	, m_players_connected_text(context.fonts->Get(FontID::kMain))
	, m_players_list_text(context.fonts->Get(FontID::kMain))
	, m_window(*context.window)
	, m_failed_connection_clock()
	, m_connected(false)
	, m_connected_players(0)					// Tally how many are connected
	, m_heartbeat_timer(sf::Time::Zero)			// Track time since the last heartbeat was sent
	, m_client_timeout(sf::seconds(1.f))        // Disconnect after 1s with no packet from server
	, m_time_since_last_packet(sf::seconds(0.f))// Accumulator for the above timeout
{
	// (Ben) Borrowed from Multiplayer Gamestate. Broadcast messages appear centred near the top of the screen
	m_broadcast_text.setPosition(sf::Vector2f(1024.f / 2, 100.f));

	// (Ben) Shows how many players have connected so far
	m_players_connected_text.setCharacterSize(35);
	m_players_connected_text.setFillColor(sf::Color::White);
	m_players_connected_text.setString(std::to_string(m_connected_players) + " players are connected");

	Utility::CentreOrigin(m_players_connected_text);
	m_players_connected_text.setPosition(
		sf::Vector2f(m_window.getSize().x / 2.f, m_window.getSize().y / 2.f));

	// (Ben) Displays countdown until game start. If only one player joined shows waiting for player's text.
	m_lobby_countdown_text.setCharacterSize(35);
	m_lobby_countdown_text.setFillColor(sf::Color::White);
	m_lobby_countdown_text.setString("Waiting for players...");
	Utility::CentreOrigin(m_lobby_countdown_text);
	m_lobby_countdown_text.setPosition(
		sf::Vector2f(m_window.getSize().x / 2.f, m_window.getSize().y / 2.f - 50.f));

	// (Ben) Borrowed from Multiplayer Gamestate
	m_failed_connection_text.setCharacterSize(35);
	m_failed_connection_text.setFillColor(sf::Color::White);
	m_failed_connection_text.setString("Attempting to connect...");
	Utility::CentreOrigin(m_failed_connection_text);
	m_failed_connection_text.setPosition(
		sf::Vector2f(m_window.getSize().x / 2.f, m_window.getSize().y / 2.f));

	// (Ben) List of player's which have joined. Starts empty until packets received from server
	m_players_list_text.setCharacterSize(25);
	m_players_list_text.setFillColor(sf::Color::White);
	m_players_list_text.setString("Players:\n");
	Utility::CentreOrigin(m_players_list_text);
	m_players_list_text.setPosition(
		sf::Vector2f(300, m_window.getSize().y / 2.f));

	// (Ben) Borrowed from multiplayer gamestate.
	m_window.clear(sf::Color::Black);
	m_window.draw(m_failed_connection_text);
	m_window.display();

	// (Ben) Borrowed from multiplayer gamestate.
	m_failed_connection_text.setString("Failed to connect");
	Utility::CentreOrigin(m_failed_connection_text);

	// (Ben) Check if joining from lobby or game
	if (firstTime) 
	{
		// (Ben's Claude) Clean the socket before connecting. Allows exiting and rejoining lobby.
		context.socket->disconnect();
		context.socket->setBlocking(true);

		// (Ben) Borrowed from Multiplayer Game State. Read the server address from disk
		std::optional<sf::IpAddress> ip;
		ip = GetAddressFromFile();

		if (ip)
		{
			// Try to establish a TCP connection — 5 second timeout for the handshake
			auto status = context.socket->connect(*ip, SERVER_PORT, sf::seconds(5.f));

			// (Ben's Claude) Ensure the connection was successful and mark it as such
			if (status == sf::Socket::Status::Done)
			{
				m_connected = true;
			}
			else
			{
				// Connection failed; start the 5-second "return to menu" countdowns
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
		context.socket->setBlocking(false);
	}

	// (Ben) Rejoining from game.
	if (!firstTime) 
	{
		m_connected = true;
		m_time_since_last_packet = sf::Time::Zero;
	}

	// (Kaylon's Claude) Send player details to the server
	if (m_connected)
	{
		sf::Packet details_packet;
		details_packet << static_cast<uint8_t>(Client::PacketType::kPlayerDetails);
		details_packet << context.player_details->m_name;
		details_packet << context.player_details->m_score;
		details_packet << LoadHighScore();
		context.socket->send(details_packet);
	}

	// (Ben) Skip countdown UI
	auto vote_skip_button = std::make_shared<gui::Button>(context);
	vote_skip_button->setPosition(sf::Vector2f(m_window.getSize().x / 2.f - 465.f /2, m_window.getSize().y / 2.f + 100));
	vote_skip_button->SetText("Vote Skip");
	vote_skip_button->SetCallback([this]()
		{
			ToggleVoteSkipCountdown();
		});

	// (Ben) Exit Lobby UI
	auto exit_lobby_button = std::make_shared<gui::Button>(context);
	exit_lobby_button->setPosition(sf::Vector2f(m_window.getSize().x / 2.f - 465.f /2, m_window.getSize().y / 2.f + 250));
	exit_lobby_button->SetText("Exit Lobby");
	exit_lobby_button->SetCallback([this]()
		{
			RequestStackClear();
			RequestStackPush(StateID::kMenu);
		});

	m_gui_container.Pack(vote_skip_button);
	m_gui_container.Pack(exit_lobby_button);
}

/// <summary>
/// Update handles packets
/// Authored: Ben
/// </summary>
bool LobbyState::Update(sf::Time delta_time)
{
	if (m_connected) {
		// Receive packets from server
		sf::Packet packet;
		if (GetContext().socket->receive(packet) == sf::Socket::Status::Done)
		{
			//std::cout << "Packet received!" << std::endl;
			// Reset the timeout accumulator — server is still alive
			m_time_since_last_packet = sf::seconds(0.f);
			uint8_t packet_type;
			packet >> packet_type;

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

		// (Ben's Claude) Send a heartbeat packet every 500ms to prevent timeout
		m_heartbeat_timer += delta_time;
		if (m_heartbeat_timer >= sf::seconds(0.5f))
		{
			sf::Packet heartbeat;
			heartbeat << static_cast<uint8_t>(Client::PacketType::kHeartbeat);
			GetContext().socket->send(heartbeat);
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

/// <summary>
/// Draw Lobbie GUI
/// Authored: Ben & Kaylon
/// </summary>
void LobbyState::Draw()
{
	if (m_connected) 
	{
		m_window.draw(m_background_sprite);
		m_window.draw(m_players_connected_text);
		m_window.draw(m_lobby_countdown_text);
		m_window.draw(m_players_list_text);
		m_window.draw(m_gui_container);

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

/// <summary>
/// Toggle whether local player wants to skip the countdown or not
/// Authored: Ben
/// </summary>
void LobbyState::ToggleVoteSkipCountdown() 
{
	m_vote_skip_countdown = !m_vote_skip_countdown;
	sf::Packet packet;
	packet << static_cast<uint8_t>(Client::PacketType::kVoteSkipCountdown) << m_vote_skip_countdown;
	GetContext().socket->send(packet);
}

/// <summary>
/// Handle keyboard inputs on the lobby
/// Authored: Ben
/// </summary>
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

		// Toggle vote skip countdown with enter
		if (key_pressed->scancode == sf::Keyboard::Scancode::Enter)
		{
			ToggleVoteSkipCountdown();
		}
	}

	m_gui_container.HandleEvent(event);

	return true;
}

/// <summary>
/// Handle different types of packets intended for the lobby
/// Author: Ben
/// </summary>
void LobbyState::HandlePacket(uint8_t packet_type, sf::Packet& packet)
{
	switch (static_cast<Server::PacketType>(packet_type))
	{
	// (Ben) This is received whenever someone connects/disconnects
	case Server::PacketType::kLobbyCountdownReset:
	{
		uint8_t connected_players; // Total players

		// Receive new total
		packet >> connected_players;

		m_connected_players = connected_players;

		// Display how many players are now connected
		m_players_connected_text.setString(std::to_string(m_connected_players) + " players are connected");
		Utility::CentreOrigin(m_players_connected_text);
	}
	break;
	// (Ben) Affirms the client is connected and updates countdown value
	case Server::PacketType::kLobbyPing:
	{
		float countdown;
		packet >> countdown;

		//std::cout << "[Client] kLobbyPing received" << std::endl;

		// Receive the new countdown value from the server
		UpdateCountdownText(countdown);
	}
	break;
	// (Ben) Borrowed from Multiplayer_Game_State
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
	// (Ben) Signify it's time to start the game
	case Server::PacketType::kGameStart:
	{
		//std::cout << "Game start packet was received" << std::endl;

		RequestStackPop(); // Exit the lobby state
		// Server has started the game — transition to the multiplayer game state
		RequestStackPush(StateID::kJoinGame);
	}
	break;

	// (Ben, Kaylon's Claude) Receive a list of connected players
	case Server::PacketType::kPlayerList:
	{
		m_ids_players.clear();
		uint8_t num_players;
		packet >> num_players;

		for (uint8_t i = 0; i < num_players; ++i)
		{
			uint8_t id;
			std::string name;
			int score;
			int high_score;
			packet >> id >> name >> score >> high_score;
			m_ids_players.push_back({ id, name, score, high_score }); // update struct to include score
		}

		std::string list = "Players:\n";
		for (auto& p : m_ids_players)
		{
			list += "- " + p.name + " (MatchScore: " + std::to_string(p.score)
				+ ", HighScore: " + std::to_string(p.high_score) + ")\n";
		}
		m_players_list_text.setString(list);
		Utility::CentreOrigin(m_players_list_text);
	}
	break;
	}
}

/// <summary>
/// Display countdown depending on how many players are connected
/// Authored: Ben
/// </summary>
void LobbyState::UpdateCountdownText(uint8_t countdown) 
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

/// <summary>
/// Borrowed from Multiplayer_Game_State
/// </summary>
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