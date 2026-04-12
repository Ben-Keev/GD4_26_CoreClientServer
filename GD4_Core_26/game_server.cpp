// ============================================================
// GEN AI - Commented by Claude
// game_server.cpp
// Implements the authoritative game server for the multiplayer
// tank game. The server runs on its own thread, manages all
// connected peers, tracks the canonical game state (positions,
// hitpoints, etc.), and broadcasts state updates to every client
// at a fixed tick rate.
// ============================================================

#include "game_server.hpp"
#include "network_protocol.hpp"   // Defines PacketType enums shared between client & server
#include <SFML/Network/Packet.hpp>
#include <SFML/System/Sleep.hpp>
#include <iostream>
#include "data_tables.hpp"        // InitializeTankPositions and other data-driven tables
#include "constants.hpp"

// ---------------------------------------------------------------------------
// Anonymous namespace — limits the visibility of SpawnPositions to this TU.
// ---------------------------------------------------------------------------
namespace
{
    // Pre-computed list of world-space positions where newly connected players
    // spawn.  Populated once at startup from the data table.
    const std::vector<sf::Vector2f> SpawnPositions = InitializeTankPositions();
}

// ---------------------------------------------------------------------------
// GameServer constructor
// Sets up all member variables, starts with a single (empty) peer slot, and
// configures the listener socket as non-blocking so it never stalls the thread.
// NOTE: The server thread is NOT started here — the caller must invoke
//       SetListening(true) or use the thread wrapper below.
// ---------------------------------------------------------------------------
GameServer::GameServer(sf::Vector2f battlefield_size)
    : m_thread(&GameServer::ExecutionThread, this)  // Binds ExecutionThread as the thread entry point
    , m_listening_state(false)                        // Not yet accepting connections
    , m_client_timeout(kClientTimeout)              // Kick a peer after 1 second of silence
    , m_max_connected_players(kMaxPlayers)                     // Hard cap on simultaneous players
    , m_connected_players(0)                          // Current number of live connections
    , m_world_height(battlefield_size.y)              // Scrolling world height in pixels
    , m_battlefield_rect(sf::Vector2f(0.f, 0.f), battlefield_size) // AABB of the entire level
    , m_peers(1)                                      // Start with capacity for 1 peer
    , m_aircraft_identifier_counter(1)                // Unique IDs start at 1 (0 = invalid)
    , m_waiting_thread_end(false)                     // Flag that tells ExecutionThread to quit
	, m_lobby_active(true)                            // Lobby is active until the first player spawns
	, m_lobby_countdown(sf::seconds(kLobbyCountdown))           // Countdown timer for lobby (starts at 10 seconds)
	, m_total_skip_countdown(0)                      // Number of "skip countdown" votes received from clients
{
    // Non-blocking so accept() returns immediately when no client is pending
    m_listener_socket.setBlocking(false);

    // Allocate the first (empty) peer slot
    m_peers[0].reset(new RemotePeer);
}

// ---------------------------------------------------------------------------
// GameServer destructor
// Signals the execution thread to stop and waits for it to finish cleanly.
// ---------------------------------------------------------------------------
GameServer::~GameServer()
{
    m_waiting_thread_end = true;  // Tell ExecutionThread to exit its loop
    m_thread.join();              // Block until the thread has actually exited
}

// ---------------------------------------------------------------------------
// (Claude AI) NotifyPlayerSpawn
// Broadcasts a kPlayerConnect packet to ALL peers so every client knows a
// new aircraft has entered the game and where it spawned.
// ---------------------------------------------------------------------------
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

// ---------------------------------------------------------------------------
// NotifyPlayerRealtimeChange
// Relays a change in a player's continuous input state (e.g. "move forward
// key is now held / released") to all clients so remote aircraft mirror the
// correct movement.
// ---------------------------------------------------------------------------
void GameServer::NotifyPlayerRealtimeChange(uint8_t aircraft_identifier, uint8_t action, bool action_enabled)
{
    sf::Packet packet;
    packet << static_cast<uint8_t>(Server::PacketType::kPlayerRealtimeChange);
    packet << aircraft_identifier;   // Which player's input changed
    packet << action;                // Which action (move left, fire, etc.)
    packet << action_enabled;        // true = key pressed, false = key released
    SendToAll(packet);
}

// ---------------------------------------------------------------------------
// NotifyPlayerEvent
// Relays a one-shot player action (e.g. firing a missile) to all clients.
// Unlike realtime changes, events fire once rather than representing held state.
// ---------------------------------------------------------------------------
void GameServer::NotifyPlayerEvent(uint8_t aircraft_identifier, uint8_t action)
{
    sf::Packet packet;
    std::cout << "Server: Notify Player Event" << +aircraft_identifier << +action << std::endl;
    packet << static_cast<uint8_t>(Server::PacketType::kPlayerEvent);
    packet << aircraft_identifier;  // Which player triggered the event
    packet << action;               // The action code (e.g. fire missile)
    SendToAll(packet);
}

// ---------------------------------------------------------------------------
// SetListening
// Starts or stops accepting new TCP connections on SERVER_PORT.
// If already in the requested state, does nothing (idempotent).
// ---------------------------------------------------------------------------
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

// ---------------------------------------------------------------------------
// ExecutionThread
// The server's main loop, running on a dedicated thread.
// Handles incoming connections and packets, advances the battlefield scroll
// at 60 fps physics, and calls Tick() at 20 Hz for game-state updates.
// A 50 ms sleep each iteration reduces CPU usage and leaves headroom for the
// client to run on the same machine.
// ---------------------------------------------------------------------------
void GameServer::ExecutionThread()
{
    // Start accepting client connections immediately
    SetListening(true);

    // Target update intervals
    sf::Time frame_rate = sf::seconds(1.f / 60.f);  // Physics step: 60 fps
    sf::Time frame_time = sf::Time::Zero;             // Accumulated physics debt
    sf::Time tick_rate = sf::seconds(kTickRate);  // Network tick: 30 Hz
    sf::Time tick_time = sf::Time::Zero;             // Accumulated tick debt

    sf::Clock frame_clock, tick_clock;  // Independent clocks for each accumulator

    // Loop until the destructor signals us to stop
    while (!m_waiting_thread_end)
    {
        // Poll for new TCP connections from clients
        HandleIncomingConnections();
        // Poll for and dispatch any packets already received from connected peers
        HandleIncomingPackets();

        // Add elapsed time to both accumulators
        frame_time += frame_clock.getElapsedTime();
        frame_clock.restart();
        tick_time += tick_clock.getElapsedTime();
        tick_clock.restart();

        // Fixed-timestep network tick: send state updates to clients at 20 Hz
        while (tick_time >= tick_rate)
        {
            Tick();
            tick_time -= tick_rate;
        }

        // Sleep to yield CPU — allows the client to run on the same machine.
        // Consider removing or reducing if performance becomes an issue.
        sf::sleep(sf::milliseconds(10));
    }
}

// ---------------------------------------------------------------------------
// Tick
// Called at 20 Hz. Performs per-tick game-logic checks:
//   1. Sends current state to all clients.
//   2. Checks win condition (all aircraft past the finish line).
//   3. Removes destroyed aircraft from the authoritative state.
//   4. Spawns new enemy waves when the timer expires.
// ---------------------------------------------------------------------------
void GameServer::Tick()
{
    // Broadcast the latest aircraft positions/HP to every connected client
    if (m_game_started && !m_lobby_active)
        UpdateClientState();

    if (m_lobby_active) 
    {
        // Heartbeat — send every 500ms to prevent server timeout (Claude)
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

    if (m_lobby_active && m_connected_players >= 2)
    {
        m_lobby_countdown -= sf::seconds(kTickRate); // Decrease countdown by tick duration

        if (m_lobby_countdown <= sf::Time::Zero)
        {
            sf::Packet game_start_packet;
            game_start_packet << static_cast<uint8_t>(Server::PacketType::kGameStart);
            SendToAll(game_start_packet);

            // Send each peer their own spawn packet now that the lobby is over
            for (std::size_t i = 0; i < m_connected_players; ++i)
            {
                if (m_peers[i]->m_ready)
                {
                    for (uint8_t identifier : m_peers[i]->m_aircraft_identifiers)
                    {
                        sf::Packet spawn_packet;
                        spawn_packet << static_cast<uint8_t>(Server::PacketType::kSpawnSelf);
                        spawn_packet << identifier;
                        spawn_packet << m_aircraft_info[identifier].m_position.x;
                        spawn_packet << m_aircraft_info[identifier].m_position.y;
                        m_peers[i]->m_socket.send(spawn_packet);
                    }
                }
            }

            // Send each peer their own spawn packet now that the lobby is over
            for (std::size_t i = 0; i < m_connected_players; ++i)
            {
				InformWorldState(m_peers[i]->m_socket);
            }

            m_lobby_active = false;
            m_game_started = true;
        }
    }

    if (!m_lobby_active && m_game_started) 
    {
        // --- Clean up destroyed aircraft ---
        // Iterate through the server's aircraft registry and remove any with 0 HP.
        // Using itr++ (post-increment) ensures the iterator remains valid after erase.
        for (auto itr = m_aircraft_info.begin(); itr != m_aircraft_info.end();)
        {
            if (itr->second.m_hitpoints <= 0)
            {
                //std::cout << "Aircraft " << std::to_string(itr->first) << " destroyed" << std::endl;
                m_aircraft_info.erase(itr++);  // Erase and advance iterator safely
            }
            else
            {
                ++itr;
            }
        }

        // Count alive aircraft (optional explicit version)
        std::size_t alive = m_aircraft_info.size();

		//std::cout << "The amount alive: " << std::to_string(alive) << std::endl;

        if (alive <= 1)
        {
            std::cout << "Game over!!" << std::endl;

            ResetGameState();

            sf::Packet return_packet;
            return_packet << static_cast<uint8_t>(Server::PacketType::kReturnToLobby);
            SendToAll(return_packet);

            SendLobbyPacket(true);
            SendPlayerList();
        }
    }
}

// ---------------------------------------------------------------------------
// Now
// Convenience wrapper — returns elapsed time from the server's master clock.
// Using a single clock ensures consistent timestamps across all Tick() calls.
// ---------------------------------------------------------------------------
sf::Time GameServer::Now() const
{
    return m_clock.getElapsedTime();
}

// ---------------------------------------------------------------------------
// HandleIncomingPackets  (outer loop)
// Iterates over all ready peers, drains their receive buffers, and delegates
// each packet to the inner HandleIncomingPackets overload.
// Also detects peers that have gone silent for longer than m_client_timeout
// and triggers disconnect handling.
// ---------------------------------------------------------------------------
void GameServer::HandleIncomingPackets()
{
    bool detected_timeout = false;

    for (PeerPtr& peer : m_peers)
    {
        if (peer->m_ready)  // Only process fully-connected peers
        {
            sf::Packet packet;
            // Drain all available packets from this peer's TCP stream
            while (peer->m_socket.receive(packet) == sf::Socket::Status::Done)
            {
                // Dispatch packet contents to the appropriate handler
                HandleIncomingPackets(packet, *peer, detected_timeout);

                // Reset the timeout clock for this peer — it is still alive
                peer->m_last_packet_time = Now();
                packet.clear();  // Prepare the packet object for the next receive
            }

            // If the peer has been silent longer than the timeout threshold, flag it
            if (Now() > peer->m_last_packet_time + m_client_timeout)
            {
                peer->m_timed_out = true;
                detected_timeout = true;
            }
        }
    }

    // Process any timed-out peers after the loop to avoid iterator invalidation
    if (detected_timeout)
    {
        HandleDisconnections();
    }
}

// ---------------------------------------------------------------------------
// HandleIncomingPackets  (inner dispatch)
// Reads the packet type byte and routes the remaining payload to the
// correct case. Modifies detected_timeout if the client requests a quit.
// ---------------------------------------------------------------------------
void GameServer::HandleIncomingPackets(sf::Packet& packet, RemotePeer& receiving_peer, bool& detected_timeout)
{
    uint8_t packet_type;
    packet >> packet_type;  // First byte always identifies the packet type

    switch (static_cast<Client::PacketType>(packet_type))
    {
        // Client is closing gracefully — treat identically to a timeout disconnect
    case Client::PacketType::kQuit:
    {
        receiving_peer.m_timed_out = true;
        detected_timeout = true;
    }
    break;

    case Client::PacketType::kVoteSkipCountdown:
    {
        bool wants_to_skip;
        packet >> wants_to_skip;
        if (wants_to_skip)
        {
            m_total_skip_countdown++; // Increment the count of "skip countdown" votes
            BroadcastMessage(std::to_string(m_total_skip_countdown) + " players have voted to skip the countdown! (++)");

            if (m_total_skip_countdown >= m_connected_players)
            {
                BroadcastMessage("The countdown was skipped!");
                m_lobby_countdown = sf::Time::Zero; // End the countdown immediately
            }
        }
        else 
        {
            BroadcastMessage(std::to_string(m_total_skip_countdown) + " players have voted to skip the countdown! (--)");
            m_total_skip_countdown--; // Decrement the count of "skip countdown" votes
        }
	}
    break;

    // Client reports a one-shot input event (e.g. missile fired).
    // The server relays this to all other clients via NotifyPlayerEvent.
    case Client::PacketType::kPlayerEvent:
    {
        uint8_t aircraft_identifier;
        uint8_t action;
        packet >> aircraft_identifier >> action;
        NotifyPlayerEvent(aircraft_identifier, action);
    }
    break;

    // Client reports a change in continuous input state (key held/released).
    // The server relays this to all other clients via NotifyPlayerRealtimeChange.
    case Client::PacketType::kPlayerRealtimeChange:
    {
        uint8_t aircraft_identifier;
        uint8_t action;
        bool action_enabled;
        packet >> aircraft_identifier >> action >> action_enabled;
        NotifyPlayerRealtimeChange(aircraft_identifier, action, action_enabled);
    }
    break;

    // Client is sending its authoritative local state (position, HP, turret angle, etc.).
    // The server updates its canonical m_aircraft_info map so it can relay the
    // information to other clients during the next UpdateClientState() call.
    case Client::PacketType::kStateUpdate:
    {
        uint8_t num_aircraft;
        packet >> num_aircraft;  // How many local aircraft this client is reporting

        // Claude - use packet struct
        for (uint8_t i = 0; i < num_aircraft; ++i)
        {
            PacketStructs::AircraftStatePacket state;
            state.Read(packet);

            auto itr = m_aircraft_info.find(state.identifier);
            if (itr != m_aircraft_info.end())
            {
                itr->second.m_position = sf::Vector2f(state.x, state.y);
                itr->second.m_hitpoints = state.hitpoints;
                itr->second.m_turret_rotation = (static_cast<float>(state.turret_rotation) / 255.f) * 360.f;
                itr->second.m_aircraft_rotation = state.hull_rotation;
            }
        }
    }
    break;

    // (Claude AI)
    case Client::PacketType::kPlayerDetails:
    {
        std::string name;
        int score;
        int high_score;
        packet >> name >> score >> high_score;
        receiving_peer.m_name = name;
        receiving_peer.m_score = score;
        receiving_peer.m_high_score = high_score;
        SendPlayerList();
    }
    break;
    break;

    // Client notifies the server of a world event (e.g. an enemy exploded).
    // The server can optionally decide to spawn a pickup in response.
    // NOTE: Pickup spawning is currently commented out / disabled.
    case Client::PacketType::kGameEvent:
    {
        // Claude - notify of wall destruction
        uint8_t action;
        float x, y;
        uint16_t id;
        packet >> action >> x >> y >> id;

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

// ---------------------------------------------------------------------------
// HandleIncomingConnections
// Called every iteration of ExecutionThread. Checks whether the listener
// socket has a pending connection and, if so, completes the handshake:
//   - Assigns a unique aircraft identifier and spawn position.
//   - Sends the new client a kSpawnSelf packet with its own ID.
//   - Sends the new client a kInitialState snapshot of all existing aircraft.
//   - Notifies all existing clients of the new player via kPlayerConnect.
//   - Adds a new empty peer slot if the server is not yet full.
// ---------------------------------------------------------------------------
void GameServer::HandleIncomingConnections()
{
    if (!m_listening_state || !m_lobby_active)
        return;  // Not accepting connections right now

    if (m_listener_socket.accept(m_peers[m_connected_players]->m_socket) == sf::TcpListener::Status::Done)
    {
        std::cout << "[Server] New connection accepted. Connected players before: " << std::to_string(m_connected_players) << std::endl;

        m_aircraft_info[m_aircraft_info.size() - 1].m_position = SpawnPositions[m_connected_players];
        m_aircraft_info[m_aircraft_info.size() - 1].m_hitpoints = 10;

        m_peers[m_connected_players]->m_aircraft_identifiers.emplace_back(m_aircraft_identifier_counter);
        m_peers[m_connected_players]->m_ready = true;
        m_peers[m_connected_players]->m_last_packet_time = Now();

        m_connected_players++;

        // Don't send kSpawnSelf here — deferred until lobby ends
        SendLobbyPacket(true);
        BroadcastMessage("New player");

        if (m_connected_players >= m_max_connected_players)
            SetListening(false);
        else
            m_peers.emplace_back(PeerPtr(new RemotePeer()));

        m_aircraft_identifier_counter++;

		SendPlayerList(); // Update the player list for all clients in the lobby
    }
    else 
    {
		// some other logic
    }
}

/// <summary> (Claude AI)
/// Send a player list to the lobby for everyone. Copilot generated.
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

void GameServer::SendLobbyPacket(bool connected) 
{
    m_lobby_countdown = sf::seconds(kLobbyCountdown); // Reset lobby countdown if a new player joins
    // Build kSpawnSelf — tells each client the new countdown and how many players are connected.
    sf::Packet packet;
    packet << static_cast<uint8_t>(Server::PacketType::kLobbyCountdownReset);

    packet << kLobbyCountdown
        << static_cast<uint8_t>(m_connected_players)
        << "Placeholder name"
        << connected;

    //std::cout << "A player has joined or left and the lobby packet was sent." << std::endl;

    // Send the updated countdown to all clients
    SendToAll(packet);
}

// ---------------------------------------------------------------------------
// HandleDisconnections
// Iterates over all peers and removes those that have timed out (either by
// silence or explicit kQuit). For each removed peer, broadcasts a
// kPlayerDisconnect packet so clients can despawn the aircraft, and resumes
// listening if the server dropped below the max-player cap.
// ---------------------------------------------------------------------------
void GameServer::HandleDisconnections()
{
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
            }

            m_connected_players--;

            // Erase the peer and get a valid iterator to the next element
            itr = m_peers.erase(itr);

            // If there is now room for more players, start accepting connections again
            if (m_connected_players < m_max_connected_players)
            {
                m_peers.emplace_back(PeerPtr(new RemotePeer()));
                SetListening(true);
            }

            BroadcastMessage("A player has disconnected");

            if(m_lobby_active)
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
}

// ---------------------------------------------------------------------------
// InformWorldState
// Sends a kInitialState packet to a single socket (typically the newly
// connected client). The packet contains the current battlefield scroll
// position and the full list of all existing aircraft with their state,
// so the new client can reconstruct the world before its first frame.
// ---------------------------------------------------------------------------
void GameServer::InformWorldState(sf::TcpSocket& socket)
{
    sf::Packet packet;
    packet << static_cast<uint8_t>(Server::PacketType::kInitialState);

    // World height and how far down the battlefield has scrolled
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
                    << m_aircraft_info[identifier].m_turret_rotation      // Already in degrees (server-side)
                    << m_peers[i]->m_name;
            }
        }
    }

    socket.send(packet);
}

// ---------------------------------------------------------------------------
// BroadcastMessage
// Sends a plain text message (kBroadcastMessage) to every ready peer.
// Clients display these messages on screen for a brief period (see
// MultiplayerGameState::UpdateBroadcastMessage).
// ---------------------------------------------------------------------------
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

// ---------------------------------------------------------------------------
// SendToAll
// Helper that delivers an already-constructed packet to every ready peer.
// Used by notification methods to avoid duplicating the iteration logic.
// ---------------------------------------------------------------------------
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

// ---------------------------------------------------------------------------
// UpdateClientState
// Assembles a kUpdateClientState packet containing the current position, HP,
// ammo and turret angle of every tracked aircraft, then sends it to all
// clients. Called every Tick() (20 Hz) to keep remote aircraft in sync.
// Clients interpolate remote positions rather than snapping to avoid jitter.
// ---------------------------------------------------------------------------
void GameServer::UpdateClientState()
{
    sf::Packet update_packet;
    update_packet << static_cast<uint8_t>(Server::PacketType::kUpdateClientState);
    update_packet << static_cast<float>(m_battlefield_rect.position.y + m_battlefield_rect.size.y);
    update_packet << static_cast<uint8_t>(m_aircraft_info.size());

	// Claude - Use packet struct to write aircraft state
    for (const auto& aircraft : m_aircraft_info)
    {
        PacketStructs::AircraftStatePacket state;
        state.identifier = aircraft.first;
        state.x = aircraft.second.m_position.x;
        state.y = aircraft.second.m_position.y;
        state.hitpoints = aircraft.second.m_hitpoints;
        state.turret_rotation = aircraft.second.m_turret_rotation;
        state.hull_rotation = aircraft.second.m_aircraft_rotation;
        state.Write(update_packet);
    }

    SendToAll(update_packet);
}

// ---------------------------------------------------------------------------
// RemotePeer constructor
// Initialises the peer as not-ready and not-timed-out, and crucially sets the
// TCP socket to non-blocking mode.  Without this the server thread would stall
// whenever it tries to receive from an idle connection.
// ---------------------------------------------------------------------------
GameServer::RemotePeer::RemotePeer()
    : m_ready(false)       // Will be set true once the handshake is complete
    , m_timed_out(false)   // Will be set true if no packet arrives within m_client_timeout
{
    // Non-blocking is essential — a blocking receive would freeze the entire
    // server thread waiting for data from a single slow or idle client.
    m_socket.setBlocking(false);
}

void GameServer::ResetGameState()
{
    m_lobby_active = true;
    m_game_started = false;
    m_aircraft_info.clear();
    m_lobby_countdown = sf::seconds(kLobbyCountdown);
    m_total_skip_countdown = 0;

    for (std::size_t i = 0; i < m_connected_players; ++i)
    {
        if (m_peers[i]->m_ready)
        {
            m_peers[i]->m_aircraft_identifiers.clear();
            m_peers[i]->m_aircraft_identifiers.emplace_back(i);

            m_aircraft_info[i].m_position = SpawnPositions[i];
            m_aircraft_info[i].m_hitpoints = 10;
        }
    }
}
