#include "multiplayer_gamestate.hpp"
#include "music_player.hpp"
#include "utility.hpp"
#include "constants.hpp"

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Network/Packet.hpp>
#include <iostream>

// (Kaylon's Claude) Initialize file reading and writing methods from Menu State
std::string LoadPlayerName();
int LoadHighScore();
void SaveDetails(const std::string& name, int high_score);

/// <summary>
/// Initialise Multiplayer Game State
/// Class Authors: John Loane, Ben Mc Keever, Kaylon Riordan, with assistance from Claude AI
/// </summary>
MultiplayerGameState::MultiplayerGameState(StateStack& stack, Context context)
    : State(stack, context)
    , m_world(*context.window, *context.fonts, *context.sound, true) // true = multiplayer mode
    , m_window(*context.window)
    , m_texture_holder(*context.textures)
    , m_connected(false)           // Connection isn't confirmed. Assume false.
    , m_active_state(true)         // Enabled/Disabled depending on if in pause menu
    , m_has_focus(true)            // Whether the window is focused
    , m_game_started(false)        // Exited lobby state and in game
	, m_client_timeout(sf::seconds(5.f))        // If no packet received for 5 second, assume connection lost and return to menu
    , m_time_since_last_packet(sf::seconds(0.f))
    , m_broadcast_text(context.fonts->Get(FontID::kMain))
    , m_failed_connection_text(context.fonts->Get(FontID::kMain))
    , m_player_dead(false)          // Whether the player has died
    , m_returning_to_lobby(false)   // Whether we're in the process of returning to the lobby
{
    // Messages received over network show up at the top of the screen
    m_broadcast_text.setPosition(sf::Vector2f(1024.f / 2, 100.f));

    // Connection status while transitioning from lobby
    m_failed_connection_text.setCharacterSize(35);
    m_failed_connection_text.setFillColor(sf::Color::White);
    m_failed_connection_text.setString("Attempting to connect...");
    Utility::CentreOrigin(m_failed_connection_text);
    m_failed_connection_text.setPosition(
        sf::Vector2f(m_window.getSize().x / 2.f, m_window.getSize().y / 2.f));

    // Black screen to facillitate transition and make text readable.
    m_window.clear(sf::Color::Black);
    m_window.draw(m_failed_connection_text);
    m_window.display();

    // Shown if connection fails from hereon.
    m_failed_connection_text.setString("Failed to connect to server");
    Utility::CentreOrigin(m_failed_connection_text);

    // The socket wasn't passed from lobby to multiplayer gamestate correctly.
    if(context.socket == nullptr)
    {
        m_connected = false;
        m_failed_connection_text.setString("No valid socket found in Context");
        Utility::CentreOrigin(m_failed_connection_text);
        m_failed_connection_clock.restart();
    }
    else
    {
		m_connected = true;  // Assume the socket was correctly transitioned
    }

    // Start the in-game music
    context.music->Play(MusicThemes::kMissionTheme);
}

/// <summary>
/// Draw the World to the client's window
/// Unmodified
/// </summary>
void MultiplayerGameState::Draw()
{
    if (m_connected)
    {
        m_world.Draw();

        // Switch to the default (unscrolled) view for HUD elements
        m_window.setView(m_window.getDefaultView());

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
/// Perframe logic while the client is ingame
/// Modified by: Ben Mc Keever And Kaylon Riordan with help from Claude
/// </summary>
bool MultiplayerGameState::Update(sf::Time dt)
{
    if (m_connected) // (Copilot Comment) Connected branch — run the game as normal, processing server packets and
                     // driving the world simulation. If connection is lost, this branch
                     // will be exited and the failure message will be shown until the
					 // 5-second timeout expires and we return to the menu.
    {
        m_world.Update(dt); // Update the world

        // Check if the local player is alive
        bool found_local_plane = false;
        for (auto itr = m_players.begin(); itr != m_players.end();)
        {
            // Check if the player's identifier matches the local player's
            if (itr->first == m_local_player_identifier)
            {
                found_local_plane = true;
            }

            // If the tank doesn't exist in the world remove it from the players list
            if (!m_world.GetAircraft(itr->first))
            {
                itr = m_players.erase(itr);
            }
            else
            {
                ++itr;
            }
        }

        // The local player died
        // Pause the game so the player can exit or continue to spectate
        if (!found_local_plane && m_game_started && !m_player_dead)
        {
            m_player_dead = true;
            RequestStackPush(StateID::kNetworkPause);
        }

        // Process realtime inputs if the window is focused and the game is unpaused
        if (m_active_state && m_has_focus)
        {
            CommandQueue& commands = m_world.GetCommandQueue();
            for (auto& pair : m_players)
            {
                pair.second->HandleRealTimeInput(commands, m_world.GetCamera());
            }
        }

        // Always apply the remaining actions in the command queue regardless of the world
        CommandQueue& commands = m_world.GetCommandQueue();
        for (auto& pair : m_players)
        {
            pair.second->HandleRealtimeNetworkInput(commands);
        }

		// (Claude Code) Process all packets received from the server this frame.
        sf::Packet packet;
        while (GetContext().socket->receive(packet) == sf::Socket::Status::Done)
        {
            m_time_since_last_packet = sf::seconds(0.f);
            uint8_t packet_type;
            packet >> packet_type;
            HandlePacket(packet_type, packet, dt);
            packet.clear();
            if (m_returning_to_lobby) break;
        }

        // If time since last packet exceeds timeout disconnect
        if (m_time_since_last_packet > m_client_timeout)
        {
            m_connected = false;
            m_failed_connection_text.setString("Lost connection to the server");
            Utility::CentreOrigin(m_failed_connection_text);
            m_failed_connection_clock.restart(); // Begin the 5-second return-to-menu countdown
        }

        // Continue ticking the broadcast message
        UpdateBroadcastMessage(dt);

        // Send game vents to the server
        GameActions::Action game_action;
        while (m_world.PollGameAction(game_action))
        {
            sf::Packet packet;
            packet << static_cast<uint8_t>(Client::PacketType::kGameEvent);
            packet << static_cast<uint8_t>(game_action.type);
            packet << game_action.position.x;
            packet << game_action.position.y;
            packet << game_action.identifier;  // (Ben's Claude Code) Pass an identifier with the action
            GetContext().socket->send(packet);
        }

        // Send updates at the server's tickrate
        if (m_tick_clock.getElapsedTime() > sf::seconds(kTickRate))
        {
            sf::Packet position_update_packet;
            position_update_packet << static_cast<uint8_t>(Client::PacketType::kStateUpdate);
            position_update_packet << static_cast<uint8_t>(1);

            // (Ben's Claude Code) - use packet struct to ensure consistent communication
            if (Tank* aircraft = m_world.GetAircraft(m_local_player_identifier))
            {
                PacketStructs::AircraftStatePacket state;
                state.identifier = m_local_player_identifier;
                state.x = static_cast<uint16_t>(aircraft->getPosition().x);
                state.y = static_cast<uint16_t>(aircraft->getPosition().y);
                state.hitpoints = static_cast<uint8_t>(aircraft->GetHitPoints());
                // (Kaylon's Claude) multiplyrotation down to 255 points so it only uses 1 byte over the network
                state.turret_rotation = static_cast<uint8_t>(aircraft->GetTurret()->getRotation().asDegrees() / 360.f * 255.f);
                state.hull_rotation = static_cast<uint8_t>(aircraft->getRotation().asDegrees() / 360.f * 255.f);

                sf::Packet position_update_packet;
                position_update_packet << static_cast<uint8_t>(Client::PacketType::kStateUpdate);
                position_update_packet << static_cast<uint8_t>(1);
                state.Write(position_update_packet);
                GetContext().socket->send(position_update_packet);
            }
            m_tick_clock.restart();
        }

        // Add time since last received packet
        m_time_since_last_packet += dt;
    }
    // Lost connection or failed to connect
    else if (m_failed_connection_clock.getElapsedTime() >= sf::seconds(5.f))
    {
        // Show failure message for five seconds
        RequestStackClear();
        RequestStackPush(StateID::kMenu);
    }
    return true;
}

/// <summary>
/// Handle input events
/// Modified: Ben Mc Keever
/// </summary>
bool MultiplayerGameState::HandleEvent(const sf::Event& event)
{
    CommandQueue& commands = m_world.GetCommandQueue();

    // Let all players react to the event (e.g. fire key pressed)
    for (auto& pair : m_players)
    {
        pair.second->HandleEvent(event, commands);
    }

    const auto* key_pressed = event.getIf<sf::Event::KeyPressed>();
    if (key_pressed)
    {
        // Esc pauses the game
        if (key_pressed->scancode == sf::Keyboard::Scancode::Escape)
        {
            // (Ben) if the local player is not dead disable its input
            // Removing this line will cause a nullptr crash
            if (!m_player_dead)
                DisableAllRealtimeActions(false);

            RequestStackPush(StateID::kNetworkPause);
        }
    }
    else if (event.is<sf::Event::FocusGained>())
    {
        // Resume processing local input when the window regains OS focus
        m_has_focus = true;
    }
    else if (event.is<sf::Event::FocusLost>())
    {
        // Suppress local input while the window is in the background
        m_has_focus = false;
    }
    return true;
}

/// <summary>
/// On Activiate
/// Modified: Ben Mc Keever
/// </summary>
void MultiplayerGameState::OnActivate()
{
    m_active_state = true;

    // (Ben) Ensure the local player exists in the m_players vector to prevent a nullptr error
    if (m_players.count(m_local_player_identifier))
        m_players[m_local_player_identifier]->DisableAllRealtimeActions(false); // Return from pause by enabling input
}

/// <summary>
/// Runs when this state is destroyed
/// Modified: Ben (Claude Assisted)
/// Note: This isn't currently triggerd from the state stack
/// </summary>
void MultiplayerGameState::OnDestroy()
{
    //std::cout << "OnDestroy called, returning_to_lobby: " << m_returning_to_lobby << "\n";

    // (Ben's Claude Code) Ensure you aren't returning to the lobby to verify you intend to quit
    if (m_connected && !m_returning_to_lobby)
    {
        //std::cout << "Sending kQuit\n";
        sf::Packet packet;
        packet << static_cast<uint8_t>(Client::PacketType::kQuit);
        GetContext().socket->send(packet);
    }
}

/// <summary>
/// Disables input when on pause menu.
/// Modified: Ben Mc Keever
/// </summary>
void MultiplayerGameState::DisableAllRealtimeActions(bool enable)
{
    m_active_state = enable;

    // (Ben) Remove the loop and use the now singular local player identifier
    m_players[m_local_player_identifier]->DisableAllRealtimeActions(enable);
}

/// <summary>
/// Display broadcast message for time period
/// Unmodified
/// </summary>
/// <param name="elapsed_time"></param>
void MultiplayerGameState::UpdateBroadcastMessage(sf::Time elapsed_time)
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

/// <summary>
/// Handle different packet types.
/// Modified: Ben Mc Keever assisted by Claude AI
/// </summary>
void MultiplayerGameState::HandlePacket(uint8_t packet_type, sf::Packet& packet, sf::Time dt)
{
    switch (static_cast<Server::PacketType>(packet_type))
    {
    // (Unmodified) Display Broadcast Messages
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
    // (Ben & Kaylon with assistance from Claude) Spawn the local player
    case Server::PacketType::kSpawnSelf:
    {
        GetContext().player_details->m_score = 0;

        // Receive the identifier and spawn position
        uint8_t aircraft_identifier;

        uint16_t px, py;
        packet >> aircraft_identifier >> px >> py;
        sf::Vector2f aircraft_position(static_cast<float>(px), static_cast<float>(py));

        // Log for clarity
        //std::cout << "Client kSpawnSelf " << +aircraft_identifier << std::endl;

        // Create a player with keys bound to the local machine
        m_players[aircraft_identifier].reset(
            new Player(GetContext().socket, aircraft_identifier, GetContext().keys, GetContext().window));

        // Spawn the aircraft with the local player details. Spawn position passed here is temporary.
        Tank* aircraft = m_world.AddAircraft(
            aircraft_identifier, 
            GetContext().player_details,
            { 512, 288 }
        );

        // Set the aircraft to the correct spawn position for this player
        aircraft->setPosition(aircraft_position);

        // (Ben) Remember this identifier to find the local player later
		m_local_player_identifier = aircraft_identifier;

        // (Ben) Store this identifier in world as well
		m_world.SetLocalPlayerIdentifier(aircraft_identifier);

        // (Ben, suggested by Claude) Signify that the game is started
        m_game_started = true;
    }
    break;

    // (Kaylon's Claude) Add Player details
    case Server::PacketType::kPlayerConnect:
    {
        uint8_t aircraft_identifier;
        std::string name;

        uint16_t px, py;
        packet >> aircraft_identifier >> px >> py >> name;
        sf::Vector2f aircraft_position(static_cast<float>(px), static_cast<float>(py));

        m_remote_player_details[aircraft_identifier] = *GetContext().player_details;
        m_remote_player_details[aircraft_identifier].m_name = name;

        m_players[aircraft_identifier].reset(
            new Player(GetContext().socket, aircraft_identifier, nullptr, GetContext().window));

        Tank* aircraft = m_world.AddAircraft(
            aircraft_identifier,
            &m_remote_player_details[aircraft_identifier],
            { 512, 288 });
        aircraft->setPosition(aircraft_position);
    }
    break;

    // Unmodified
    case Server::PacketType::kPlayerDisconnect:
    {
        uint8_t aircraft_identifier;
        packet >> aircraft_identifier;
        m_world.RemoveAircraft(aircraft_identifier);
        m_players.erase(aircraft_identifier);
    }
    break;

    // (Ben, Kaylon) Pass in the initial state
    case Server::PacketType::kInitialState:
    {
        uint8_t aircraft_count;
        float world_height, current_scroll;
        packet >> world_height >> current_scroll;

        // Sync world dimentions with server values
        m_world.SetWorldHeight(world_height);
        m_world.SetCurrentBattleFieldPosition(current_scroll);

        // Spawn each tank
        packet >> aircraft_count;
        for (uint8_t i = 0; i < aircraft_count; ++i)
        {
            uint8_t aircraft_identifier;
            uint8_t hitpoints;
            uint16_t px, py;
            float turret_rotation;
            std::string name;

            packet >> aircraft_identifier
                >> px
                >> py
                >> hitpoints
                >> turret_rotation  // (Ben) where the turret is facing
                >> name;            // (Kaylon) Name taken from player_Details
            sf::Vector2f aircraft_position(static_cast<float>(px), static_cast<float>(py)); // (Kaylon) 

            // (Ben's Claude Code) Skip spawning local players here to avoid duplicates.
			if (aircraft_identifier == m_local_player_identifier)
            {
                continue;
            }

            // (Kaylon) Set remote player's name
            m_remote_player_details[aircraft_identifier].m_name = name;

            // Remote players get nullptr keybindings. They can't be controlled from this machine
            m_players[aircraft_identifier].reset(
                new Player(GetContext().socket, aircraft_identifier, nullptr, GetContext().window));

            // (Kaylon) Initialise with remote player details
            Tank* aircraft = m_world.AddAircraft(
                aircraft_identifier,
                &m_remote_player_details[aircraft_identifier],
                { 512, 288 });
            
            aircraft->setPosition(aircraft_position);

            // (Ben) Set turret rotation to received value. Convert to degrees.
            aircraft->GetTurret()->setRotation(sf::degrees(turret_rotation));
        }
    }
    break;

    // Unmodified
    case Server::PacketType::kPlayerEvent:
    {
        uint8_t aircraft_identifier;
        uint8_t action;
        packet >> aircraft_identifier >> action;
        //std::cout << "Player Event" << aircraft_identifier << std::endl;

        auto itr = m_players.find(aircraft_identifier);
        if (itr != m_players.end())
        {
            std::cout << "Handling Network Event" << std::endl;
            itr->second->HandleNetworkEvent(
                static_cast<Action>(action), m_world.GetCommandQueue());
        }
    }
    break;

    // Unmodified
    case Server::PacketType::kPlayerRealtimeChange:
    {
        uint8_t aircraft_identifier;
        uint8_t action;
        bool action_enabled;
        packet >> aircraft_identifier >> action >> action_enabled;

        auto itr = m_players.find(aircraft_identifier);
        if (itr != m_players.end())
        {
            itr->second->HandleNetworkRealtimeChange(static_cast<Action>(action), action_enabled);
        }
    }
    break;

    // Update Remote Aircraft Values at TickRate for local client
    case Server::PacketType::kUpdateClientState:
    {
        float current_world_position;
        uint8_t aircraft_count;
        packet >> current_world_position >> aircraft_count;

        // (Ben's Claude) Use packet struct for consistency
        for (uint8_t i = 0; i < aircraft_count; ++i)
        {
            PacketStructs::AircraftStatePacket state;
            state.Read(packet);

            Tank* aircraft = m_world.GetAircraft(state.identifier);
            bool is_local = state.identifier == m_local_player_identifier;

            // (Kaylon) Modify interpolation code, update rotation to account for int 8 compression (Ben) Set turret and hull rotation
            if (aircraft && !is_local)
            {
                float blend = 1.f - std::pow(0.5f, dt.asSeconds() * 30.f);
                aircraft->setPosition(aircraft->getPosition() + (sf::Vector2f(state.x, state.y) - aircraft->getPosition()) * blend);
                // (Kaylon's Claude) multiplyrotation down to 255 points so it only uses 1 byte over the network
                aircraft->setRotation(sf::degrees((static_cast<float>(state.hull_rotation) / 255.f) * 360.f));
                aircraft->GetTurret()->setRotation(sf::degrees((static_cast<float>(state.turret_rotation) / 255.f) * 360.f));
            }
        }
    }
    break;
    // (Ben & Kaylon's Claude) Begin returning to lobby after win condition met
    case Server::PacketType::kReturnToLobby :
    {
        // (Kaylon's Claude) Check and update high score
        int current_score = GetContext().player_details->m_score;
        int high_score = LoadHighScore();

        if (current_score > high_score)
        {
            SaveDetails(GetContext().player_details->m_name, current_score);
            std::cout << "New high score: " << current_score << "\n";
        }

        // (Kaylon's Claude) Signify returning to lobby to prevent sending kQuit packet onDestroy
        m_returning_to_lobby = true;
		RequestStackClear(); // (Kaylon's Claude) Pop the multiplayer game state & pause state, not just the pause state
		RequestStackPush(StateID::kRejoinLobby);
    }
    break;
    // (Kaylon's Claude) Flush lobby packets received while ingame to prevent player's being booted from the lobby
    case Server::PacketType::kLobbyCountdownReset:
    {
        std::cout << "kLobbyCountdownReset received in game state - ignoring\n";
        uint8_t player_count;
        packet >> player_count;
    }
    break;
    // (Kaylon's Claude) Flush lobby packets received while ingame to prevent player's being booted from the lobby
    case Server::PacketType::kPlayerList:
    {
        uint8_t count;
        packet >> count;
        for (uint8_t i = 0; i < count; ++i)
        {
            uint8_t id;
            std::string name;
            int score;
            int high_score;
            packet >> id >> name >> score >> high_score;
        }
    }
    break;
    // (Kaylon's Claude) Flush lobby packets received while ingame to prevent player's being booted from the lobby
    case Server::PacketType::kLobbyPing:
    {
        float countdown;
        packet >> countdown;
    }
    break;
    // (Ben's Claude) Ensure wooden wall destruction stays synced. Receive instructions from server for where to destroy)
    case Server::PacketType::kWallDestroyed:
    {
        // 
        float x, y; // Position of wall
        uint16_t id; // Projectile's id
        packet >> x >> y >> id;
        m_world.DestroyWallAt(sf::Vector2f(x, y));
		m_world.DestroyProjectile(id);  // Destroy projectiles from remote players that collided with the wall
    }
    break;
    // (Kaylon's Claude) Award bonus points to the last surviving player
    case Server::PacketType::kAwardBonusPoints:
    {
        uint8_t winner_id;
        uint8_t bonus;
        packet >> winner_id >> bonus;

        if (winner_id == m_local_player_identifier)
        {
            GetContext().player_details->m_score += bonus;
        }

        // (Kaylon's Claude) Signal that we're returning to lobby so OnDestroy
        // doesn't send kQuit when the stack clears on kReturnToLobby
        m_returning_to_lobby = true;
    }
    break;
    default:
        std::cout << "Unknown packet type: " << +packet_type << "\n";
        break;
    } // end switch
}