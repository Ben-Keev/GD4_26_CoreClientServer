// ============================================================
// GEN AI - Commented by Claude
// multiplayer_gamestate.cpp
// Implements the client-side multiplayer game state.
// This state manages the connection to the game server,
// drives the local world simulation, and translates incoming
// server packets into scene-graph mutations (spawning aircraft,
// updating positions, removing dead planes, etc.).
// It sits in the State Stack alongside the menu, pause, and
// game-over states, and is pushed when a multiplayer session begins.
// ============================================================

#include "SocketWrapperPCH.hpp"       // Pre-compiled header wrapping SFML network types
#include "multiplayer_gamestate.hpp"
#include "music_player.hpp"
#include "utility.hpp"                // CentreOrigin helper
#include "data_tables.hpp"

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Network/Packet.hpp>
#include <SFML/Network/IpAddress.hpp>

#include <fstream>
#include <iostream>



// ---------------------------------------------------------------------------
// MultiplayerGameState constructor
// Initialises the world, UI text objects, and attempts to connect to the
// server whose IP is stored in ip.txt.  A brief "Attempting to connect..."
// frame is rendered immediately so the user gets visual feedback before the
// 5-second TCP handshake timeout.
// ---------------------------------------------------------------------------
MultiplayerGameState::MultiplayerGameState(StateStack& stack, Context context)
    : State(stack, context)
    , m_world(*context.window, *context.fonts, *context.sound, true) // true = multiplayer mode
    , m_window(*context.window)
    , m_texture_holder(*context.textures)
    , m_connected(false)           // Not yet connected to the server
    , m_game_server(nullptr)       // Only set if this client is also hosting
    , m_active_state(true)         // Whether real-time input should be processed
    , m_has_focus(true)            // Whether the application window has OS focus
    , m_game_started(false)        // Set true once the server sends kSpawnSelf
    , m_client_timeout(sf::seconds(1.f))        // Disconnect after 1 s with no packet
    , m_time_since_last_packet(sf::seconds(0.f))// Accumulator for the above timeout
    , m_broadcast_text(context.fonts->Get(FontID::kMain))
    , m_player_invitation_text(context.fonts->Get(FontID::kMain))
    , m_failed_connection_text(context.fonts->Get(FontID::kMain))
    , m_player_dead(false)
    , m_returning_to_lobby(false)
{
    // Broadcast messages appear centred near the top of the screen
    m_broadcast_text.setPosition(sf::Vector2f(1024.f / 2, 100.f));

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
    m_failed_connection_text.setString("Failed to connect to server");
    Utility::CentreOrigin(m_failed_connection_text);

    if(context.socket == nullptr)
    {
        m_connected = false;
        m_failed_connection_text.setString("No valid socket found in Context");
        Utility::CentreOrigin(m_failed_connection_text);
        m_failed_connection_clock.restart();

    }
    else 
    {
		m_connected = true;  // Assume the socket is already connected (handshake done in LobbyState)
    }

    // Start the in-game music
    context.music->Play(MusicThemes::kMissionTheme);
}

// ---------------------------------------------------------------------------
// Draw
// If connected: renders the game world and any active HUD overlays
//   (broadcast messages, "waiting for second player" invitation).
// If not connected: shows the connection-status/error message.
// ---------------------------------------------------------------------------
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

// ---------------------------------------------------------------------------
// Update
// Per-frame update logic split into two main branches:
//   Connected    — advance the world, process local/network input, receive
//                  server packets, send position updates, check game-over.
//   Not connected — wait up to 5 seconds then bail back to the main menu.
// Returns true so the state stack continues processing states below this one.
// ---------------------------------------------------------------------------
bool MultiplayerGameState::Update(sf::Time dt)
{
    if (m_connected)
    {
        //// If the game is paused (active_state == false), disable real-time input
        //// for all local players so they cannot move while the pause menu is open
        //if (!m_active_state)
        //    DisableAllRealtimeActions(true);

        m_world.Update(dt);

        // --- Check for destroyed / missing local aircraft ---
        bool found_local_plane = false;
        for (auto itr = m_players.begin(); itr != m_players.end();)
        {
            // Track whether the local player's aircraft exists
            if (itr->first == m_local_player_identifier)
            {
                found_local_plane = true;
            }

            // If the world has removed this aircraft (it was destroyed), erase it
            if (!m_world.GetAircraft(itr->first))
            {
                itr = m_players.erase(itr);

                // If ALL players are gone, trigger the game-over state
                if (m_players.empty())
                {
                    //RequestStackPush(StateID::kGameOver);
                }
            }
            else
            {
                ++itr;
            }
        }

        // If none of our own aircraft are alive (and the game was already running),
        // push the game-over state (handles the case where local planes despawn
        // without the players map becoming empty)
        if (!found_local_plane && m_game_started && !m_player_dead)
        {
            m_player_dead = true;
            RequestStackPush(StateID::kNetworkPause);
        }

        // --- Local real-time input ---
        // Only process keyboard/gamepad state if the window is focused and unpaused
        if (m_active_state && m_has_focus)
        {
            CommandQueue& commands = m_world.GetCommandQueue();
            for (auto& pair : m_players)
            {
                pair.second->HandleRealTimeInput(commands, m_world.GetCamera());
            }
        }

        // --- Network real-time input ---
        // Always applied regardless of window focus (mirrors remote players' actions)
        CommandQueue& commands = m_world.GetCommandQueue();
        for (auto& pair : m_players)
        {
            
            pair.second->HandleRealtimeNetworkInput(commands);
        }

        // (Claude AI) --- Receive server packets ---
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

        // No packet arrived this frame; check for timeout
        if (m_time_since_last_packet > m_client_timeout)
        {
            m_connected = false;
            m_failed_connection_text.setString("Lost connection to the server");
            Utility::CentreOrigin(m_failed_connection_text);
            m_failed_connection_clock.restart(); // Begin the 5-second return-to-menu countdown
        }

        // Advance the broadcast message display timer
        UpdateBroadcastMessage(dt);

        // --- Send game events to the server ---
        // World events (e.g. enemy destroyed) are polled from the world's event queue
        // and forwarded to the server so it can decide on consequences (pickups, etc.)
        GameActions::Action game_action;
        while (m_world.PollGameAction(game_action))
        {
            sf::Packet packet;
            packet << static_cast<uint8_t>(Client::PacketType::kGameEvent);
            packet << static_cast<uint8_t>(game_action.type);
            packet << game_action.position.x;
            packet << game_action.position.y;
            packet << game_action.identifier;  // (Claude)
            GetContext().socket->send(packet);
        }



        // --- Send position update at 20 Hz ---
        // Matches the server's tick rate so updates are not sent unnecessarily often.
        if (m_tick_clock.getElapsedTime() > sf::seconds(1.f / 30.f))
        {
            sf::Packet position_update_packet;
            position_update_packet << static_cast<uint8_t>(Client::PacketType::kStateUpdate);
            position_update_packet << static_cast<uint8_t>(1);

            if (Tank* aircraft = m_world.GetAircraft(m_local_player_identifier))
            {
                // Compress turret rotation from [0, 360] degrees into a uint8_t [0, 255]
                // to save bandwidth. The server decompresses back to degrees when relaying.
                position_update_packet << m_local_player_identifier
                    << aircraft->getPosition().x
                    << aircraft->getPosition().y
                    << static_cast<uint8_t>(aircraft->GetHitPoints())
                    << static_cast<uint8_t>(0)  // Missile count placeholder (not yet implemented)
                    << static_cast<uint8_t>(aircraft->GetTurret()->getRotation().asDegrees() / 360.f * 255.f);
                    //<< aircraft->getRotation().asDegrees();  // Hull rotation — full float precision
            }
            GetContext().socket->send(position_update_packet);
            m_tick_clock.restart();
        }

        // Accumulate time since the last received packet to detect server timeout
        m_time_since_last_packet += dt;
    }
    // Not connected branch — failed to connect (or lost connection)
    else if (m_failed_connection_clock.getElapsedTime() >= sf::seconds(5.f))
    {
        // After 5 seconds of showing the failure message, return to the main menu
        RequestStackClear();
        RequestStackPush(StateID::kMenu);
    }
    return true;
}

// ---------------------------------------------------------------------------
// HandleEvent
// Processes discrete SFML events (key presses, window focus changes).
// Forwards all events to local players first, then handles state-level keys.
// ---------------------------------------------------------------------------
bool MultiplayerGameState::HandleEvent(const sf::Event& event)
{
    CommandQueue& commands = m_world.GetCommandQueue();

    // Let all local players react to the event (e.g. fire key pressed)
    for (auto& pair : m_players)
    {
        pair.second->HandleEvent(event, commands);
    }

    const auto* key_pressed = event.getIf<sf::Event::KeyPressed>();
    if (key_pressed)
    {
        // Escape opens the network pause screen and disables real-time input
        if (key_pressed->scancode == sf::Keyboard::Scancode::Escape)
        {

            if (!m_player_dead)
            DisableAllRealtimeActions(false);  // false = disable (stop) real-time actions
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

// ---------------------------------------------------------------------------
// OnActivate
// Called by the state stack when this state becomes the top-most active state
// (e.g. after the pause screen is popped). Re-enables input processing.
// ---------------------------------------------------------------------------
void MultiplayerGameState::OnActivate()
{
    m_active_state = true;

    if (m_players.count(m_local_player_identifier))
        m_players[m_local_player_identifier]->DisableAllRealtimeActions(false);
}

// ---------------------------------------------------------------------------
// OnDestroy
// Called just before the state is removed from the stack.
// If this client is NOT the host and is still connected, sends a kQuit packet
// so the server can cleanly remove this peer immediately rather than waiting
// for the timeout.
// ---------------------------------------------------------------------------
void MultiplayerGameState::OnDestroy()
{
    std::cout << "OnDestroy called, returning_to_lobby: " << m_returning_to_lobby << "\n";
    if (m_connected && !m_returning_to_lobby)
    {
        std::cout << "Sending kQuit\n";
        sf::Packet packet;
        packet << static_cast<uint8_t>(Client::PacketType::kQuit);
        GetContext().socket->send(packet);
    }
}

// ---------------------------------------------------------------------------
// DisableAllRealtimeActions
// Propagates the active/paused state to every local Player object so their
// held-key inputs (move, fire) are suppressed while the game is paused.
// The parameter name "enable" is slightly misleading — passing false disables
// real-time actions (pause), passing true re-enables them (resume).
// ---------------------------------------------------------------------------
void MultiplayerGameState::DisableAllRealtimeActions(bool enable)
{
    m_active_state = enable;
    m_players[m_local_player_identifier]->DisableAllRealtimeActions(enable);
}

// ---------------------------------------------------------------------------
// UpdateBroadcastMessage
// Ticks the display timer for the current broadcast message.
// Each message is shown for 2 seconds; when it expires it is dequeued and
// the next message in the queue (if any) begins displaying immediately.
// ---------------------------------------------------------------------------
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

// ---------------------------------------------------------------------------
// HandlePacket
// Central dispatcher for packets received from the server.
// Reads the packet type (already extracted by the caller) and routes the
// remaining payload to the appropriate handler block.
// ---------------------------------------------------------------------------
void MultiplayerGameState::HandlePacket(uint8_t packet_type, sf::Packet& packet, sf::Time dt)
{
    switch (static_cast<Server::PacketType>(packet_type))
    {
        // --- kBroadcastMessage ---
        // Server is sending a text message to display on screen (e.g. "New player").
        // Messages are queued; each is shown for 2 seconds before the next appears.
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
    // --- kSpawnSelf ---
    // Sent once, right after connection, to tell this client which aircraft
    // identifier it owns and where to place it.  Creates the local Player
    // object with the correct key bindings (keys1) and marks the game as started.
    case Server::PacketType::kSpawnSelf:
    {
        uint8_t aircraft_identifier;
        sf::Vector2f aircraft_position;

        packet >> aircraft_identifier >> aircraft_position.x >> aircraft_position.y;

        std::cout << "Client kSpawnSelf " << +aircraft_identifier << std::endl;

        // Create a Player with actual key bindings (keys1) — this is a local player
        m_players[aircraft_identifier].reset(
            new Player(GetContext().socket, aircraft_identifier, GetContext().keys1, GetContext().window));

        // Spawn the aircraft in the world scene graph and position it
        Tank* aircraft = m_world.AddAircraft(
            aircraft_identifier, 
            GetContext().player_details,
            { 512, 288 }
        );

        aircraft->setPosition(aircraft_position);

		m_local_player_identifier = aircraft_identifier;
		m_world.SetLocalPlayerIdentifier(aircraft_identifier);

        m_game_started = true;  // Allow game-over checks to run
    }
    break;

    // --- kPlayerConnect ---
    // A new remote player has joined.  Create a Player WITHOUT key bindings
    // (nullptr) since this client does not control it; the server will send
    // position updates to animate it.
    case Server::PacketType::kPlayerConnect:
    {
        uint8_t aircraft_identifier;
        sf::Vector2f aircraft_position;
        packet >> aircraft_identifier >> aircraft_position.x >> aircraft_position.y;

        // nullptr key bindings = remote player; input handled via network packets only
        m_players[aircraft_identifier].reset(
            new Player(GetContext().socket, aircraft_identifier, nullptr, GetContext().window));

        // Spawn the aircraft in the world scene graph and position it
        Tank* aircraft = m_world.AddAircraft(
            aircraft_identifier, 
            GetContext().player_details,
            { 512, 288 }
        );

        aircraft->setPosition(aircraft_position);
    }
    break;

    // --- kPlayerDisconnect ---
    // A remote player has left.  Remove their aircraft from the world and
    // erase their Player entry.
    case Server::PacketType::kPlayerDisconnect:
    {
        uint8_t aircraft_identifier;
        packet >> aircraft_identifier;
        m_world.RemoveAircraft(aircraft_identifier);
        m_players.erase(aircraft_identifier);
    }
    break;

    // --- kInitialState ---
    // Sent to a newly connected client to describe all aircraft that already
    // exist in the game world (players who were connected before us).
    // Also communicates the current battlefield scroll position.
    case Server::PacketType::kInitialState:
    {
        uint8_t aircraft_count;
        float world_height, current_scroll;
        packet >> world_height >> current_scroll;

        // Synchronise the world's internal dimensions with the server's values
        m_world.SetWorldHeight(world_height);
        m_world.SetCurrentBattleFieldPosition(current_scroll);

        packet >> aircraft_count;
        for (uint8_t i = 0; i < aircraft_count; ++i)
        {
            uint8_t aircraft_identifier;
            uint8_t hitpoints;
            uint8_t missile_ammo;
            sf::Vector2f aircraft_position;
            float turret_rotation;
            //float aircraft_rotation;  // Hull rotation — only needed for initial state sync

            packet >> aircraft_identifier
                >> aircraft_position.x
                >> aircraft_position.y
                >> hitpoints
                >> missile_ammo
                >> turret_rotation;
                //>> aircraft_rotation;

            // Skip if this is our own aircraft — already spawned via kSpawnSelf (Claude)
			if (aircraft_identifier == m_local_player_identifier)
            {
                continue;
            }

            // All existing players are remote from this client's perspective (nullptr keys)
            m_players[aircraft_identifier].reset(
                new Player(GetContext().socket, aircraft_identifier, nullptr, GetContext().window));

            Tank* aircraft = m_world.AddAircraft(
                aircraft_identifier,
                GetContext().player_details,
                { 512, 288 });
            
            aircraft->setPosition(aircraft_position);
            //aircraft->setRotation(sf::degrees(aircraft_rotation));
            aircraft->GetTurret()->setRotation(sf::degrees(turret_rotation));
        }
    }
    break;

    // --- kPlayerEvent ---
    // A remote player performed a one-shot action (e.g. fired a missile).
    // The event is routed to the correct Player object which applies it to
    // the world's command queue.
    case Server::PacketType::kPlayerEvent:
    {
        uint8_t aircraft_identifier;
        uint8_t action;
        packet >> aircraft_identifier >> action;
        std::cout << "Player Event" << aircraft_identifier << std::endl;

        auto itr = m_players.find(aircraft_identifier);
        if (itr != m_players.end())
        {
            std::cout << "Handling Network Event" << std::endl;
            itr->second->HandleNetworkEvent(
                static_cast<Action>(action), m_world.GetCommandQueue());
        }
    }
    break;

    // --- kPlayerRealtimeChange ---
    // A remote player's continuous input state changed (key held / released).
    // Forwarded to the Player object so it can start/stop the corresponding
    // movement or action on the remote aircraft.
    case Server::PacketType::kPlayerRealtimeChange:
    {
        uint8_t aircraft_identifier;
        uint8_t action;
        bool action_enabled;
        packet >> aircraft_identifier >> action >> action_enabled;

        auto itr = m_players.find(aircraft_identifier);
        if (itr != m_players.end())
        {
            itr->second->HandleNetworkRealtimeChange(
                static_cast<Action>(action), action_enabled);
        }
    }
    break;

    // --- kSpawnEnemy ---
    // The server has decided to spawn an enemy at a given location and type.
    // Currently the packet is received but no action is taken (enemy spawning
    // logic not yet implemented on the client side).
    case Server::PacketType::kSpawnEnemy:
    {
        float height;
        uint8_t type;
        float relative_x;
        packet >> type >> height >> relative_x;
        // TODO: Spawn the enemy aircraft in the world at (relative_x, height)
    }
    break;

    // --- kMissionSuccess ---
    // All objectives have been met.  Push the mission-success state to display
    // the victory screen to the player.
    case Server::PacketType::kMissionSuccess:
    {
        RequestStackPush(StateID::kMissionSuccess);
    }
    break;

    // --- kSpawnPickup ---
    // Server is telling the client to place a pickup item in the world.
    // Currently received but not acted upon (pickup logic not yet implemented).
    case Server::PacketType::kSpawnPickup:
    {
        uint8_t type;
        sf::Vector2f position;
        packet >> type >> position.x >> position.y;
        // TODO: Spawn the pickup entity in the world scene graph
    }
    break;

    // --- kUpdateClientState ---
    // Periodic (20 Hz) authoritative state snapshot from the server.
    // For each aircraft described, the client:
    //   - Ignores local aircraft (we already have authoritative local state).
    //   - Interpolates remote aircraft towards their reported positions
    //     (10% blend per frame) to smooth out network jitter.
    //   - Updates the remote turret rotation directly (no interpolation needed).
    case Server::PacketType::kUpdateClientState:
    {
        float current_world_position;
        uint8_t aircraft_count;
        packet >> current_world_position >> aircraft_count;

        // current_view_position could be used to reconcile the scroll, but is
        // currently calculated and unused (kept for potential future use)
        float current_view_position =
            m_world.GetViewBounds().position.y + m_world.GetViewBounds().size.y;

        for (uint8_t i = 0; i < aircraft_count; ++i)
        {
            sf::Vector2f aircraft_position;
            uint8_t aircraft_identifier;
            uint8_t hitpoints;
            uint8_t ammo;
            float turret_rotation;  // Server sends this in degrees (decompressed server-side)

            packet >> aircraft_identifier
                >> aircraft_position.x
                >> aircraft_position.y
                >> hitpoints
                >> ammo
                >> turret_rotation;

            Tank* aircraft = m_world.GetAircraft(aircraft_identifier);

            // Check if this is the aircraft we control locally
			bool is_local_plane = aircraft_identifier == m_local_player_identifier;

            if (aircraft && !is_local_plane)
            {
                // (Claude AI)
                // Framerate-independent exponential interpolation towards server position.
                // 0.1f controls smoothing (lower = snappier), 30.f should match server tick rate.
                float blend = 1.f - std::pow(0.5f, dt.asSeconds() * 30.f);
                sf::Vector2f interpolated_position =
                    aircraft->getPosition()
                    + (aircraft_position - aircraft->getPosition()) * blend;
                aircraft->setPosition(interpolated_position);

                // Turret angle is snapped directly — the rotation is small and
                // frequent enough that interpolation is not necessary
                aircraft->GetTurret()->setRotation(sf::degrees(turret_rotation));
            }
        }
    }
    break;
    case Server::PacketType::kReturnToLobby :
    {
        m_returning_to_lobby = true;

		RequestStackClear(); // Pop the multiplayer game state


		RequestStackPush(StateID::kRejoinLobby);
    }
    break;

    //(Claude AI)
    case Server::PacketType::kLobbyCountdownReset:
    {
        std::cout << "kLobbyCountdownReset received in game state - ignoring\n";
        float countdown;
        uint8_t player_count;
        std::string name;
        bool connected;
        packet >> countdown >> player_count >> name >> connected;
    }
    break;

    case Server::PacketType::kPlayerList:
    {
        std::cout << "kPlayerList received in game state - ignoring\n";
        uint8_t count;
        packet >> count;
        for (uint8_t i = 0; i < count; ++i)
        {
            uint8_t id;
            std::string name;
            packet >> id >> name;
        }
    }
    break;

    case Server::PacketType::kLobbyPing:
    {
        float countdown;
        packet >> countdown;
    }
    break;
    case Server::PacketType::kPlayerVelocityUpdate:
    {
        uint8_t aircraft_identifier;
        float vx, vy;
        packet >> aircraft_identifier >> vx >> vy;

        auto itr = m_players.find(aircraft_identifier);
        if (itr != m_players.end())
        {
            itr->second->SetNetworkVelocity(sf::Vector2f(vx, vy));
        }
    }
    break;
    case Server::PacketType::kWallDestroyed:
    {
        // Claude - destroy a wall at the given position according to server
        float x, y;
        uint16_t id;
        packet >> x >> y >> id;
        m_world.DestroyWallAt(sf::Vector2f(x, y));
		m_world.DestroyProjectile(id);  // Also destroy the projectile that hit the wall (if it still exists on the client
    }
    break;
    default:
        std::cout << "Unknown packet type: " << +packet_type << "\n";
        break;
    } // end switch
}