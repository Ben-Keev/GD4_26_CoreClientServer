#include "game_server.hpp"
#include "network_protocol.hpp"
#include "data_tables.hpp"
#include "constants.hpp"

#include <SFML/Network/Packet.hpp>
#include <SFML/System/Sleep.hpp>
#include <iostream>

namespace
{
    // Spawnpoints taken from data table.cpp
    const std::vector<sf::Vector2<uint16_t>> SpawnPositions = InitializeTankPositions();
}

/// <summary>
/// Server
/// Modified: Ben Mc Keever, Kaylon Riordan, Assisted by Claude
/// (Kaylon)
/// </summary>
GameServer::GameServer(sf::Vector2f battlefield_size)
    : m_thread(&GameServer::ExecutionThread, this)  
    , m_listening_state(false)                        
    , m_client_timeout(kClientTimeout)                // Kick a peer once client timeout exceeded
    , m_max_connected_players(kMaxPlayers)            // Cap how many players can connect
    , m_connected_players(0)                          // How many players are connected
    , m_world_height(battlefield_size.y)              // Scrolling world height in pixels
    , m_battlefield_rect(sf::Vector2f(0.f, 0.f), battlefield_size) // Rect size of battlefield
    , m_peers(1)                                      // Start with capacity for 1 peer
    , m_waiting_thread_end(false)                     // Flag that tells ExecutionThread to quit
	, m_lobby_active(true)                            // Whether the lobby is active or not
	, m_lobby_countdown(sf::seconds(kLobbyCountdown)) // Countdown for game start
	, m_total_skip_countdown(0)                       // Number of "skip countdown" votes received from clients
    , m_post_game_delay_active(false)
    , m_post_game_delay_timer(sf::Time::Zero)
    , m_game_has_had_multiple_players(false)
{
    // Non-blocking so accept() returns immediately when no client is pending
    m_listener_socket.setBlocking(false);

    // Allocate the first (empty) peer slot
    m_peers[0].reset(new RemotePeer);

    // (Kaylon's Claude) Populate available IDs in reverse so ID 1 is popped first
    for (int i = m_max_connected_players; i >= 1; --i)
    {
        m_available_ids.push(static_cast<uint8_t>(i));
    }
}

/// <summary>
/// Unmodified
/// </summary>
GameServer::~GameServer()
{
    m_waiting_thread_end = true;  // Tell ExecutionThread to exit its loop
    m_thread.join();              // Block until the thread has actually exited
}

/// <summary>
/// Modified: Kaylon's Claude
/// </summary>
void GameServer::NotifyPlayerSpawn(uint8_t aircraft_identifier)
{
    // Find the peer that owns this identifier
    for (std::size_t i = 0; i < m_connected_players; ++i)
    {
        for (uint8_t id : m_peers[i]->m_aircraft_identifiers)
        {
            if (id == aircraft_identifier)
            {
                sf::Packet packet;
                packet << static_cast<uint8_t>(Server::PacketType::kPlayerConnect);
                packet << aircraft_identifier
                    << m_aircraft_info[aircraft_identifier].m_position.x
                    << m_aircraft_info[aircraft_identifier].m_position.y
                    << m_peers[i]->m_name;
                SendToAll(packet);
                return;
            }
        }
    }
}

/// <summary>
/// Unmodified
/// </summary>
void GameServer::NotifyPlayerRealtimeChange(uint8_t aircraft_identifier, uint8_t action, bool action_enabled)
{
    sf::Packet packet;
    packet << static_cast<uint8_t>(Server::PacketType::kPlayerRealtimeChange);
    packet << aircraft_identifier;   
    packet << action;                
    packet << action_enabled;        
    SendToAll(packet);
}

/// <summary>
/// Unmodified
/// </summary>
void GameServer::NotifyPlayerEvent(uint8_t aircraft_identifier, uint8_t action)
{
    sf::Packet packet;
    std::cout << "Server: Notify Player Event" << +aircraft_identifier << +action << std::endl;
    packet << static_cast<uint8_t>(Server::PacketType::kPlayerEvent);
    packet << aircraft_identifier;  
    packet << action;               
    SendToAll(packet);
}

/// <summary>
/// Unmodified
/// </summary>
/// <param name="enable"></param>
void GameServer::SetListening(bool enable)
{
    if (enable)
    {
        // Only begin listening if we are not already doing so
        if (!m_listening_state)
        {
            // Attempt to bind the listener to SERVER_PORT; store success/fail
            m_listening_state = (m_listener_socket.listen(SERVER_PORT) == sf::TcpListener::Status::Done);
        }
    }
    else
    {
        // Close the listener socket and mark us as no longer listening
        m_listener_socket.close();
        m_listening_state = false;
    }
}

/// <summary>
/// Loop that handles tickrate and framerate
/// (Kaylon) Increase tick rate to 30 to improve performance noticably
/// </summary>
void GameServer::ExecutionThread()
{
    // Start accepting client connections immediately
    SetListening(true);

    // Target frame rate
    sf::Time frame_rate = sf::seconds(1.f / 60.f); 
    sf::Time frame_time = sf::Time::Zero;          

    // Tickrate and time elapsed
    sf::Time tick_rate = sf::seconds(kTickRate);
    sf::Time tick_time = sf::Time::Zero;        

    // Clocks for tick_time and frame_time
    sf::Clock frame_clock, tick_clock;

    // Loop until destructor says stop
    while (!m_waiting_thread_end)
    {
        // Check for connections
        HandleIncomingConnections();
        // Check for packets coming from clients
        HandleIncomingPackets();

        // Add time to each clock
        frame_time += frame_clock.getElapsedTime();
        frame_clock.restart();
        tick_time += tick_clock.getElapsedTime();
        tick_clock.restart();

        // Send updates to clients at the tickrate
        while (tick_time >= tick_rate)
        {
            Tick();
            tick_time -= tick_rate;
        }

        // (Kaylon) Run client at the same time as server. Reduced to 10ms.
        sf::sleep(sf::milliseconds(10));
    }
}

/// <summary>
/// Called at tickrate
/// Modified: Ben, Kaylon
/// </summary>
void GameServer::Tick()
{
    // (Ben) Only broadcast game updates if we're in game
    if (m_game_started && !m_lobby_active)
        UpdateClientState();

    // (Ben) Only send lobby ping if in lobby
    if (m_lobby_active) 
    {
        // (Ben's Claude) Lobby ping updates countdown and serves as a heartbeat to prevent timeout
        m_heartbeat_timer += sf::seconds(kTickRate);
        if (m_heartbeat_timer >= sf::seconds(0.5f))
        {
            sf::Packet heartbeat;
            heartbeat << static_cast<uint8_t>(Server::PacketType::kLobbyPing);
            heartbeat << static_cast<float>(m_lobby_countdown.asSeconds());
            SendToAll(heartbeat);
            m_heartbeat_timer = sf::Time::Zero;
        }
    }

    // (Kaylon's Claude) Tick the post game delay before allowing countdown to start
    if (m_post_game_delay_active)
    {
        m_post_game_delay_timer += sf::seconds(kTickRate);
        if (m_post_game_delay_timer >= sf::seconds(3.f))
            m_post_game_delay_active = false;
    }

    // (Ben) The countdown may start on the lobby screen
    if (m_lobby_active && m_connected_players >= 2 && !m_post_game_delay_active)
    {
        // (Ben's Claude) Decrease countdown by tickrate
        m_lobby_countdown -= sf::seconds(kTickRate);

        // (Ben) The countdown is 0 and the game may start
        if (m_lobby_countdown <= sf::Time::Zero)
        {
            // (Ben) Signal to clients to move to multiplayer_game_state
            sf::Packet game_start_packet;
            game_start_packet << static_cast<uint8_t>(Server::PacketType::kGameStart);
            SendToAll(game_start_packet);

            // (Ben) Now that clients are in game, trigger spawn self on each client
            for (std::size_t i = 0; i < m_connected_players; ++i)
            {
                if (m_peers[i]->m_ready)
                {
                    for (uint8_t identifier : m_peers[i]->m_aircraft_identifiers)
                    {
                        sf::Packet spawn_packet;
                        spawn_packet << static_cast<uint8_t>(Server::PacketType::kSpawnSelf);
                        spawn_packet << identifier;
                        spawn_packet << static_cast<uint16_t>(m_aircraft_info[identifier].m_position.x);
                        spawn_packet << static_cast<uint16_t>(m_aircraft_info[identifier].m_position.y);
                        m_peers[i]->m_socket.send(spawn_packet);
                    }
                }
            }

            // (Ben) Now that each client has spawned themself spawn remote clients
            for (std::size_t i = 0; i < m_connected_players; ++i)
            {
				InformWorldState(m_peers[i]->m_socket);
            }

            // The lobby has ended and the game has started
            m_lobby_active = false;
            m_game_started = true;
        }
    }

    // The game has started. Game Logic.
    if (!m_lobby_active && m_game_started) 
    {
        // Remove tanks that are dead
        for (auto itr = m_aircraft_info.begin(); itr != m_aircraft_info.end();)
        {
            if (itr->second.m_hitpoints <= 0)
            {
                m_aircraft_info.erase(itr++);
            }
            else
            {
                ++itr;
            }
        }

        // Keep track of how many are alive. Dead tanks are removed from this.
        std::size_t alive = m_aircraft_info.size();

        // (Kaylon's Claude) Track when multiple players have been alive to prevent
        // the win condition firing before the match has actually started
        if (alive >= 2)
            m_game_has_had_multiple_players = true;

        // (Ben) "Win Condition". Game ends when there's only one tank left.
        if (alive <= 1)
        {
            // (Kaylon's Claude) Award bonus points to the last surviving player
            if (!m_aircraft_info.empty())
            {
                uint8_t winner_id = m_aircraft_info.begin()->first;
                uint8_t bonus = static_cast<uint8_t>((m_connected_players + 4) / 5);
                sf::Packet bonus_packet;
                bonus_packet << static_cast<uint8_t>(Server::PacketType::kAwardBonusPoints);
                bonus_packet << winner_id;
                bonus_packet << bonus;
                SendToAll(bonus_packet);
            }

            // (Kaylon's Claude) Tell clients to return BEFORE resetting state so they
            // aren't kicked by stale ID reassignment or a kQuit from OnDestroy
            sf::Packet return_packet;
            return_packet << static_cast<uint8_t>(Server::PacketType::kReturnToLobby);
            SendToAll(return_packet);

            // Reset after clients have been told to leave
            ResetGameState();

            // Send up to date information
            SendLobbyPacket(true);
            SendPlayerList();
        }
    }
}

/// <summary>
/// Unmodified
/// </summary>
sf::Time GameServer::Now() const
{
    return m_clock.getElapsedTime();
}

/// <summary>
/// Unmodified
/// </summary>
void GameServer::HandleIncomingPackets()
{
    bool detected_timeout = false;

    for (PeerPtr& peer : m_peers)
    {
        if (peer->m_ready)
        {
            sf::Packet packet;
            while (peer->m_socket.receive(packet) == sf::Socket::Status::Done)
            {
                HandleIncomingPackets(packet, *peer, detected_timeout);

                peer->m_last_packet_time = Now();
                packet.clear();
            }

            if (Now() > peer->m_last_packet_time + m_client_timeout)
            {
                peer->m_timed_out = true;
                detected_timeout = true;
            }
        }
    }

    if (detected_timeout)
    {
        HandleDisconnections();
    }
}

/// <summary>
/// Handle Packets from clients
/// Modified: Ben and Kaylon with assistance from Claude
/// </summary>
void GameServer::HandleIncomingPackets(sf::Packet& packet, RemotePeer& receiving_peer, bool& detected_timeout)
{
    uint8_t packet_type;
    packet >> packet_type;  // First byte always identifies the packet type

    switch (static_cast<Client::PacketType>(packet_type))
    {
    // Unmodified
    case Client::PacketType::kQuit:
    {
        receiving_peer.m_timed_out = true;
        detected_timeout = true;
    }
    break;
    // (Ben) Tally how many clients want to skip the countdown
    case Client::PacketType::kVoteSkipCountdown:
    {
        bool wants_to_skip;
        packet >> wants_to_skip;
        if (wants_to_skip)
        {
            m_total_skip_countdown++;
            BroadcastMessage(std::to_string(m_total_skip_countdown) + " vote(s) to start! (++)");

            if (m_total_skip_countdown >= m_connected_players)
            {
                BroadcastMessage("The countdown was skipped!");
                m_lobby_countdown = sf::Time::Zero;
            }
        }
        else 
        {
            BroadcastMessage(std::to_string(m_total_skip_countdown) + " vote(s) to start! (--)");
            m_total_skip_countdown--;
        }
	}
    break;

    // Unmodified
    case Client::PacketType::kPlayerEvent:
    {
        uint8_t aircraft_identifier;
        uint8_t action;
        packet >> aircraft_identifier >> action;
        NotifyPlayerEvent(aircraft_identifier, action);
    }
    break;

    // Unmodified
    case Client::PacketType::kPlayerRealtimeChange:
    {
        uint8_t aircraft_identifier;
        uint8_t action;
        bool action_enabled;
        packet >> aircraft_identifier >> action >> action_enabled;
        NotifyPlayerRealtimeChange(aircraft_identifier, action, action_enabled);
    }
    break;

    // (Ben's Claude)
    case Client::PacketType::kStateUpdate:
    {
        uint8_t num_aircraft;
        packet >> num_aircraft;  // How many local aircraft this client is reporting

        // (Ben's Claude) Use packet struct for consistency
        for (uint8_t i = 0; i < num_aircraft; ++i)
        {
            PacketStructs::AircraftStatePacket state;
            state.Read(packet);

            auto itr = m_aircraft_info.find(state.identifier);
            if (itr != m_aircraft_info.end())
            {
                itr->second.m_position = sf::Vector2<uint16_t>(state.x, state.y);
                itr->second.m_hitpoints = state.hitpoints;
                // (Kaylon's Claude) multiplyrotation down to 255 points so it only uses 1 byte over the network
                itr->second.m_turret_rotation = (static_cast<float>(state.turret_rotation) / 255.f) * 360.f;
                itr->second.m_aircraft_rotation = (static_cast<float>(state.hull_rotation) / 255.f) * 360.f;
            }
        }
    }
    break;

    // (Kaylon's Claude) Receive player details for the lobby
    case Client::PacketType::kPlayerDetails:
    {
        std::string name;
        int score;
        int high_score;
        packet >> name >> score >> high_score;
        receiving_peer.m_name = name;
        receiving_peer.m_score = score;
        receiving_peer.m_high_score = high_score;

        // Update lobby with this information
        SendPlayerList();
    }
    break;

    // (Ben's Claude)
    case Client::PacketType::kGameEvent:
    {
        // (Ben's Claude) Relay Wall destruction to everyone
        uint8_t action;
        float x, y; // Wall Position
        uint16_t id; // Projectile ID
        packet >> action >> x >> y >> id;

        // Confirm it's a wall destroyed action
        if (action == GameActions::kWallDestroyed)
        {
            // Relay to everyone including sender
            sf::Packet relay;
            relay << static_cast<uint8_t>(Server::PacketType::kWallDestroyed);
            relay << x << y << id;
            SendToAll(relay);
        }
    }
    } // end switch
}

/// <summary>
/// Handle incoming connections
/// Modified: Ben, Kaylon using Claude
/// </summary>
void GameServer::HandleIncomingConnections()
{
    // (Ben) Refuse connections while in game
    if (!m_listening_state || !m_lobby_active)
        return;

    // (Claude AI) Reject if no IDs available — server is full
    if (m_available_ids.empty())
    {
        std::cout << "[Server] No available IDs, rejecting connection\n";
        return;
    }

    if (m_listener_socket.accept(m_peers[m_connected_players]->m_socket) == sf::TcpListener::Status::Done)
    {
        // (Claude AI) Pop next available ID from stack
        uint8_t new_id = m_available_ids.top();
        m_available_ids.pop();

        m_aircraft_info[new_id].m_position = SpawnPositions[m_connected_players];
        m_aircraft_info[new_id].m_hitpoints = 10;

        m_peers[m_connected_players]->m_aircraft_identifiers.emplace_back(new_id);
        m_peers[m_connected_players]->m_ready = true;
        m_peers[m_connected_players]->m_last_packet_time = Now();

        m_connected_players++;

        // (Ben's Claude) Don't send kSpawnSelf here — deferred until lobby ends
        SendLobbyPacket(true);
        BroadcastMessage("New player");

        if (m_connected_players >= m_max_connected_players)
            SetListening(false);
        else
            m_peers.emplace_back(PeerPtr(new RemotePeer()));

        // (Ben) Update player list for everyone
        SendPlayerList();
    }
}

/// <summary>
/// Send player list to everyone
/// Authored: Ben with assistance of Github Copilot | Modified: Kaylon with assistance of Claude
/// </summary>
void GameServer::SendPlayerList()
{
    sf::Packet packet;
    packet << static_cast<uint8_t>(Server::PacketType::kPlayerList);
    packet << static_cast<uint8_t>(m_connected_players);
    for (std::size_t i = 0; i < m_connected_players; ++i)
    {
        if (m_peers[i]->m_ready)
        {
            for (uint8_t id : m_peers[i]->m_aircraft_identifiers)
            {
                packet << id
                    << m_peers[i]->m_name    // from peer
                    << m_peers[i]->m_score   // from peer
                    << m_peers[i]->m_high_score;
            }
        }
    }
    SendToAll(packet);
}

/// <summary>
/// Send packet containing new lobby countdown. Called on disconnect and connect
/// Authored: Ben
/// </summary>
void GameServer::SendLobbyPacket(bool connected) 
{
    // (Ben) Reset lobby countdown
    m_lobby_countdown = sf::seconds(kLobbyCountdown);

    // (Ben) Inform clients and tally total connected players to clients
    sf::Packet packet;
    packet << static_cast<uint8_t>(Server::PacketType::kLobbyCountdownReset);

    packet << static_cast<uint8_t>(m_connected_players);

    //std::cout << "A player has joined or left and the lobby packet was sent." << std::endl;

    // Send the updated countdown to all clients
    SendToAll(packet);
}

/// <summary>
/// Modified: Ben, Kaylon, Claude
/// </summary>
void GameServer::HandleDisconnections()
{
    bool needs_new_slot = false;

    for (auto itr = m_peers.begin(); itr != m_peers.end();)
    {
        if ((*itr)->m_timed_out)
        {
            // Notify all remaining clients about each aircraft owned by this peer
            for (uint8_t identifier : (*itr)->m_aircraft_identifiers)
            {
                // Build and send the disconnect packet inline using a temporary sf::Packet
                SendToAll((sf::Packet()
                    << static_cast<uint8_t>(Server::PacketType::kPlayerDisconnect)
                    << identifier));

                // Remove the aircraft from the server's authoritative state
                m_aircraft_info.erase(identifier);
                m_available_ids.push(identifier); // return ID to pool
            }

            m_connected_players--;
            itr = m_peers.erase(itr);  // safe, no realloc here

            if (m_connected_players < m_max_connected_players)
                needs_new_slot = true;  // defer the emplace_back

            BroadcastMessage("A player has disconnected");

            // Update lobby to stop displaying disconnected player and reset countdown
            if (m_lobby_active)
            {
                SendLobbyPacket(false); // Update lobby countdown and player count for remaining clients
                SendPlayerList(); // Update the player list for all clients in the lobby
            }
        }
        else
        {
            ++itr;
        }
    }

    // Safe to reallocate now that we're out of the loop
    if (needs_new_slot)
    {
        m_peers.emplace_back(PeerPtr(new RemotePeer()));
        SetListening(true);
    }
}

/// <summary>
/// Send initial information about the wolrd once
/// Modified: Ben & Ben
/// </summary>
/// <param name="socket"></param>
void GameServer::InformWorldState(sf::TcpSocket& socket)
{
    sf::Packet packet;
    packet << static_cast<uint8_t>(Server::PacketType::kInitialState);

    // World dimensions
    packet << m_world_height
        << m_battlefield_rect.position.y + m_battlefield_rect.size.y;

    // Number of existing aircraft so the client knows how many records to read
    packet << static_cast<uint8_t>(m_aircraft_info.size());

    // Append each ready peer's aircraft data
    for (std::size_t i = 0; i < m_connected_players; ++i)
    {
        if (m_peers[i]->m_ready)
        {
            for (uint8_t identifier : m_peers[i]->m_aircraft_identifiers)
            {
                packet << identifier
                    << m_aircraft_info[identifier].m_position.x
                    << m_aircraft_info[identifier].m_position.y
                    << m_aircraft_info[identifier].m_hitpoints
                    << m_aircraft_info[identifier].m_turret_rotation      // (Ben)
                    << m_peers[i]->m_name; // (Kaylon)
            }
        }
    }

    socket.send(packet);
}

/// <summary>
/// Unmodified
/// </summary>
void GameServer::BroadcastMessage(const std::string& message)
{
    sf::Packet packet;
    packet << static_cast<uint8_t>(Server::PacketType::kBroadcastMessage);
    packet << message;
    for (std::size_t i = 0; i < m_connected_players; ++i)
    {
        if (m_peers[i]->m_ready)
        {
            m_peers[i]->m_socket.send(packet);
        }
    }
}

/// <summary>
/// Unmodified
/// </summary>
void GameServer::SendToAll(sf::Packet& packet)
{
    for (std::size_t i = 0; i < m_connected_players; ++i)
    {
        if (m_peers[i]->m_ready)
        {
            m_peers[i]->m_socket.send(packet);
        }
    }
}

/// <summary>
/// Modified: Ben and Kaylon with assistance of claude
/// </summary>
void GameServer::UpdateClientState()
{
    sf::Packet update_packet;
    update_packet << static_cast<uint8_t>(Server::PacketType::kUpdateClientState);
    update_packet << static_cast<float>(m_battlefield_rect.position.y + m_battlefield_rect.size.y);
    update_packet << static_cast<uint8_t>(m_aircraft_info.size());

    // (Ben's Claude) use struct for consistency
    for (const auto& aircraft : m_aircraft_info)
    {
        PacketStructs::AircraftStatePacket state;
        state.identifier = aircraft.first;
        state.x = aircraft.second.m_position.x;
        state.y = aircraft.second.m_position.y;
        state.hitpoints = aircraft.second.m_hitpoints;
        // (Kaylon's Claude) multiplyrotation down to 255 points so it only uses 1 byte over the network
        state.turret_rotation = static_cast<uint8_t>(aircraft.second.m_turret_rotation / 360.f * 255.f);
        state.hull_rotation = static_cast<uint8_t>(aircraft.second.m_aircraft_rotation / 360.f * 255.f);
        state.Write(update_packet);
    }

    SendToAll(update_packet);
}

/// <summary>
/// Unmodified
/// </summary>
GameServer::RemotePeer::RemotePeer()
    : m_ready(false)
    , m_timed_out(false)
{
    m_socket.setBlocking(false);
}

/// <summary>
/// Reset all variables to default. Called on transition to lobby
/// Authored: Ben with assistance of Claude
/// Modified: Kaylon with assistance of Claude
/// </summary>
void GameServer::ResetGameState()
{
    m_lobby_active = true;
    m_game_started = false;
    m_game_has_had_multiple_players = false;
    m_aircraft_info.clear();
    m_lobby_countdown = sf::seconds(kLobbyCountdown);
    m_total_skip_countdown = 0;

    // (Kaylon's Claude) Prevent countdown starting before clients have rejoined lobby
    m_post_game_delay_active = true;
    m_post_game_delay_timer = sf::Time::Zero;

    while (!m_available_ids.empty())
        m_available_ids.pop();
    for (int i = m_max_connected_players; i >= 1; --i)
        m_available_ids.push(static_cast<uint8_t>(i));

    for (std::size_t i = 0; i < m_connected_players; ++i)
    {
        if (m_peers[i]->m_ready)
        {
            uint8_t new_id = m_available_ids.top();  // Pop next available ID
            m_available_ids.pop();

            m_peers[i]->m_aircraft_identifiers.clear();
            m_peers[i]->m_aircraft_identifiers.emplace_back(new_id);

            m_aircraft_info[new_id].m_position = SpawnPositions[i];
            m_aircraft_info[new_id].m_hitpoints = 10;
        }
    }
}
