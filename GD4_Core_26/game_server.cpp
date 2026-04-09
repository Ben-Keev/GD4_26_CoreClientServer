// ============================================================
// GEN AI - Commented by Claude
// game_server.cpp
// Implements the authoritative game server for the multiplayer
// tank game. The server runs on its own thread, manages all
// connected peers, tracks the canonical game state (positions,
// hitpoints, etc.), and broadcasts state updates to every client
// at a fixed tick rate.
// ============================================================

#include "SocketWrapperPCH.hpp"   // Pre-compiled header; wraps SFML network includes
#include "game_server.hpp"
#include "network_protocol.hpp"   // Defines PacketType enums shared between client & server
#include "tank_type.hpp"          // TankType enum used when spawning enemies
#include "utility.hpp"            // RandomInt, CentreOrigin, etc.
#include <SFML/Network/Packet.hpp>
#include <SFML/System/Sleep.hpp>
#include <iostream>
#include "data_tables.hpp"        // InitializeTankPositions and other data-driven tables

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
    , m_client_timeout(sf::seconds(1.f))              // Kick a peer after 1 second of silence
    , m_max_connected_players(20)                     // Hard cap on simultaneous players
    , m_connected_players(0)                          // Current number of live connections
    , m_world_height(battlefield_size.y)              // Scrolling world height in pixels
    , m_battlefield_rect(sf::Vector2f(0.f, 0.f), battlefield_size) // AABB of the entire level
    , m_battlefield_scrollspeed(0.f)                  // Pixels/second scroll speed (currently 0)
    , m_aircraft_count(0)                             // Total spawned aircraft (players + enemies)
    , m_peers(1)                                      // Start with capacity for 1 peer
    , m_aircraft_identifier_counter(1)                // Unique IDs start at 1 (0 = invalid)
    , m_waiting_thread_end(false)                     // Flag that tells ExecutionThread to quit
	, m_lobby_active(true)                            // Lobby is active until the first player spawns
	, m_lobby_countdown(sf::seconds(10.f))           // Countdown timer for lobby (starts at 10 seconds)
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
// NotifyPlayerSpawn
// Broadcasts a kPlayerConnect packet to ALL peers so every client knows a
// new aircraft has entered the game and where it spawned.
// ---------------------------------------------------------------------------
void GameServer::NotifyPlayerSpawn(uint8_t aircraft_identifier)
{
    sf::Packet packet;
    // Every packet begins with its type identifier (cast to uint8_t for compactness)
    packet << static_cast<uint8_t>(Server::PacketType::kPlayerConnect);
    // Append the new aircraft's unique ID and its initial world position
    packet << aircraft_identifier
        << m_aircraft_info[aircraft_identifier].m_position.x
        << m_aircraft_info[aircraft_identifier].m_position.y;
    SendToAll(packet);
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
    sf::Time tick_rate = sf::seconds(1.f / 20.f);  // Network tick: 20 Hz
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

        // Fixed-timestep physics: consume frame_rate-sized slices from the accumulator.
        // This keeps the battlefield scroll deterministic regardless of actual loop speed.
        while (frame_time >= frame_rate)
        {
            // Scroll the battlefield rectangle downward each physics step
            m_battlefield_rect.position.y += m_battlefield_scrollspeed * frame_rate.asSeconds();
            frame_time -= frame_rate;
        }

        // Fixed-timestep network tick: send state updates to clients at 20 Hz
        while (tick_time >= tick_rate)
        {
            Tick();
            tick_time -= tick_rate;
        }

        // Sleep to yield CPU — allows the client to run on the same machine.
        // Consider removing or reducing if performance becomes an issue.
        sf::sleep(sf::milliseconds(50));
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
    UpdateClientState();

    // Heartbeat — send every 500ms to prevent server timeout (Claude)
    m_heartbeat_timer += sf::seconds(1.f / 20.f);
    if (m_heartbeat_timer >= sf::seconds(0.5f))
    {
        sf::Packet heartbeat;
        heartbeat << static_cast<uint8_t>(Server::PacketType::kLobbyPing);
		heartbeat << static_cast<float>(m_lobby_countdown.asSeconds());
        SendToAll(heartbeat);
        m_heartbeat_timer = sf::Time::Zero;
    }

    if (m_lobby_active && m_connected_players >= 2)
    {
        m_lobby_countdown -= sf::seconds(1.f / 20.f); // Decrease countdown by tick duration

        if (m_lobby_countdown <= sf::Time::Zero)
        {
            m_lobby_active = false; // End the lobby phase
            sf::Packet game_start_packet;
            game_start_packet << static_cast<uint8_t>(Server::PacketType::kGameStart);
        }
    }

    if (!m_lobby_active) 
    {
        // --- Win condition check ---
        bool all_aircraft_done = true;
        for (const auto& current : m_aircraft_info)
        {
            // Check condition for aircraft being done here
        }
        if (all_aircraft_done)
        {
            // Tell every client to show the mission-success screen
            sf::Packet mission_success_packet;
            mission_success_packet << static_cast<uint8_t>(Server::PacketType::kMissionSuccess);
            SendToAll(mission_success_packet);
        }

        // --- Clean up destroyed aircraft ---
        // Iterate through the server's aircraft registry and remove any with 0 HP.
        // Using itr++ (post-increment) ensures the iterator remains valid after erase.
        for (auto itr = m_aircraft_info.begin(); itr != m_aircraft_info.end();)
        {
            if (itr->second.m_hitpoints <= 0)
            {
                m_aircraft_info.erase(itr++);  // Erase and advance iterator safely
            }
            else
            {
                ++itr;
            }
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

    // Client requests a second local player (co-op partner).
    // NOTE: This feature is currently unused/disabled on the client side but
    // the server still allocates the aircraft and notifies other peers.
    case Client::PacketType::kRequestCoopPartner:
    {
        //// Register the new aircraft under the requesting peer
        //receiving_peer.m_aircraft_identifiers.emplace_back(m_aircraft_identifier_counter);

        //// Spawn the co-op partner at the centre of the visible battlefield area
        //m_aircraft_info[m_aircraft_identifier_counter].m_position =
        //    sf::Vector2f(m_battlefield_rect.size.x / 2,
        //        m_battlefield_rect.position.y + m_battlefield_rect.size.y / 2);
        //m_aircraft_info[m_aircraft_identifier_counter].m_hitpoints = 100;
        //m_aircraft_info[m_aircraft_identifier_counter].m_missile_ammo = 2;

        //// Notify all OTHER peers about the new aircraft
        //sf::Packet notify_packet;
        //notify_packet << static_cast<uint8_t>(Server::PacketType::kPlayerConnect);
        //notify_packet << m_aircraft_identifier_counter;
        //notify_packet << m_aircraft_info[m_aircraft_identifier_counter].m_position.x;
        //notify_packet << m_aircraft_info[m_aircraft_identifier_counter].m_position.y;

        //for (PeerPtr& peer : m_peers)
        //{
        //    if (peer.get() != &receiving_peer && peer->m_ready)
        //    {
        //        peer->m_socket.send(notify_packet);
        //    }
        //}
        //m_aircraft_identifier_counter++;  // Advance ID so the next aircraft gets a unique one
    }
    break;

    // Client is sending its authoritative local state (position, HP, turret angle, etc.).
    // The server updates its canonical m_aircraft_info map so it can relay the
    // information to other clients during the next UpdateClientState() call.
    case Client::PacketType::kStateUpdate:
    {
        uint8_t num_aircraft;
        packet >> num_aircraft;  // How many local aircraft this client is reporting

        for (uint8_t i = 0; i < num_aircraft; ++i)
        {
            uint8_t aircraft_identifier;
            uint8_t aircraft_hitpoints;
            uint8_t missile_ammo;
            sf::Vector2f aircraft_position;
            uint8_t turret_byte;      // Compressed turret rotation: 0-255 maps to 0-360 degrees
            float aircraft_rotation;  // Hull rotation in degrees (uncompressed)

            packet >> aircraft_identifier
                >> aircraft_position.x
                >> aircraft_position.y
                >> aircraft_hitpoints
                >> missile_ammo
                >> turret_byte
                >> aircraft_rotation;

            // Update the server's authoritative record for this aircraft
            m_aircraft_info[aircraft_identifier].m_position = aircraft_position;
            m_aircraft_info[aircraft_identifier].m_hitpoints = aircraft_hitpoints;
            m_aircraft_info[aircraft_identifier].m_missile_ammo = missile_ammo;
            // Convert compressed byte back to degrees for storage
            m_aircraft_info[aircraft_identifier].m_turret_byte = (static_cast<float>(turret_byte) / 255.f) * 360.f;
            m_aircraft_info[aircraft_identifier].m_aircraft_rotation = aircraft_rotation;
        }
    }
    break;

    // Client notifies the server of a world event (e.g. an enemy exploded).
    // The server can optionally decide to spawn a pickup in response.
    // NOTE: Pickup spawning is currently commented out / disabled.
    case Client::PacketType::kGameEvent:
    {
        uint8_t action;
        float x;
        float y;

        packet >> action >> x >> y;

        // Only process kEnemyExplode events, and only from the first/host peer,
        // to avoid duplicate pickup spawns when multiple clients report the same explosion.
        if (action == GameActions::kEnemyExplode
            && Utility::RandomInt(3) == 0          // 33% chance of a pickup
            && &receiving_peer == m_peers[0].get())
        {
            // Pickup spawning disabled — packets are commented out for now.
            // sf::Packet packet;
            // packet << static_cast<uint8_t>(Server::PacketType::kSpawnPickup);
            // packet << static_cast<uint8_t>(Utility::RandomInt(static_cast<int>(PickupType::kPickupCount)));
            // packet << x;
            // packet << y;
            // SendToAll(packet);
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
    if (!m_listening_state)
        return;  // Not accepting connections right now

    // accept() is non-blocking — returns immediately if no client is waiting
    if (m_listener_socket.accept(m_peers[m_connected_players]->m_socket) == sf::TcpListener::Status::Done)
    {
        // Initialise the new aircraft's authoritative server state
        m_aircraft_info[m_aircraft_identifier_counter].m_position = SpawnPositions[m_connected_players];
        m_aircraft_info[m_aircraft_identifier_counter].m_hitpoints = 100;
        m_aircraft_info[m_aircraft_identifier_counter].m_missile_ammo = 2;

        std::cout << "The counter for spawn position is here: " << m_aircraft_identifier_counter << std::endl;

        // Build kSpawnSelf — tells the new client which ID it owns and where to place itself
        sf::Packet packet;
        packet << static_cast<uint8_t>(Server::PacketType::kSpawnSelf);
        packet << m_aircraft_identifier_counter;
        packet << m_aircraft_info[m_aircraft_identifier_counter].m_position.x;
        packet << m_aircraft_info[m_aircraft_identifier_counter].m_position.y;

        // Record the new aircraft under this peer so we can clean it up on disconnect
        m_peers[m_connected_players]->m_aircraft_identifiers.emplace_back(m_aircraft_identifier_counter);

        // Announce the new player to everyone, send the world snapshot to the new client,
        // and then send the kSpawnSelf packet (order matters — world state first so the
        // client can populate the scene before handling its own spawn).
        BroadcastMessage("New player");
        InformWorldState(m_peers[m_connected_players]->m_socket);  // Send existing-world snapshot
        NotifyPlayerSpawn(m_aircraft_identifier_counter++);        // Broadcast new player; advance ID

        // Finally send the personalised kSpawnSelf packet to the new client THIS SHOULD WAIT UNTIL THE LOBBY IS FINISHED
        m_peers[m_connected_players]->m_socket.send(packet);

        // Mark the peer as ready so HandleIncomingPackets will process its messages
        m_peers[m_connected_players]->m_ready = true;
        m_peers[m_connected_players]->m_last_packet_time = Now();

        m_aircraft_count++;
        m_connected_players++;

        if (m_connected_players >= m_max_connected_players)
        {
            // Server is full — stop accepting new connections
            SetListening(false);
        }
        else
        {
            // Prepare an empty slot for the next potential connection
            m_peers.emplace_back(PeerPtr(new RemotePeer()));
        }

        if (m_lobby_active)
        {
			SendLobbyPacket(); // Send lobby countdown update to all clients when a new player joins
        }
    }
}

void GameServer::SendLobbyPacket() 
{
    m_lobby_countdown = sf::seconds(10.f); // Reset lobby countdown if a new player joins
    // Build kSpawnSelf — tells each client the new countdown and how many players are connected.
    sf::Packet packet;
    packet << static_cast<uint8_t>(Server::PacketType::kLobbyCountdownReset);

    packet << 10.f
        << static_cast<uint8_t>(m_connected_players);

    std::cout << "A player has joined or left and the lobby packet was sent." << std::endl;

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
            m_aircraft_count -= (*itr)->m_aircraft_identifiers.size();

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
                SendLobbyPacket(); // Update lobby countdown and player count for remaining clients
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
    packet << static_cast<uint8_t>(m_aircraft_count);

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
                    << m_aircraft_info[identifier].m_missile_ammo
                    << m_aircraft_info[identifier].m_turret_byte      // Already in degrees (server-side)
                    << m_aircraft_info[identifier].m_aircraft_rotation;
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
    sf::Packet update_client_state_packet;
    update_client_state_packet << static_cast<uint8_t>(Server::PacketType::kUpdateClientState);

    // Current bottom edge of the battlefield scroll region (used by clients to
    // determine the view position relative to the world)
    update_client_state_packet << static_cast<float>(m_battlefield_rect.position.y + m_battlefield_rect.size.y);

    // Total number of aircraft entries that follow in the packet
    update_client_state_packet << static_cast<uint8_t>(m_aircraft_count);

    for (const auto& aircraft : m_aircraft_info)
    {
        update_client_state_packet
            << aircraft.first                        // Unique aircraft identifier
            << aircraft.second.m_position.x          // World X
            << aircraft.second.m_position.y          // World Y
            << aircraft.second.m_hitpoints           // Current HP (0-100)
            << aircraft.second.m_missile_ammo        // Remaining missiles
            << aircraft.second.m_turret_byte;        // Turret angle (stored as degrees on server)
    }

    SendToAll(update_client_state_packet);
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