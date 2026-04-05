#include "SocketWrapperPCH.hpp"
#include "player.hpp"
#include "command_queue.hpp"
#include "tank.hpp"

#include "network_protocol.hpp"
#include <SFML/Network/Packet.hpp>

#include <map>

// Functor that moves an aircraft when a command is executed
struct AircraftMover
{
    AircraftMover(float vx, float vy, int identifier)
        : velocity(vx, vy)
        , aircraft_id(identifier)
    {
    }

    // This runs when the command is executed
    void operator()(Tank& aircraft, sf::Time) const
    {
        // Only move the aircraft that belongs to this player
        if (aircraft.GetIdentifier() == aircraft_id)
        {
            aircraft.Accelerate(velocity * 200.f);
        }
    }

    sf::Vector2f velocity; // Direction of movement
    int aircraft_id;       // Which aircraft to control
};

// Functor that triggers bullet firing
struct AircraftFireTrigger
{
    AircraftFireTrigger(int identifier)
        : aircraft_id(identifier)
    {
    }

    void operator() (Tank& aircraft, sf::Time) const
    {
        // Only fire for this player's aircraft
        if (aircraft.GetIdentifier() == aircraft_id)
            aircraft.Fire();
    }

    int aircraft_id;
};

// Player constructor
Player::Player(sf::TcpSocket* socket, uint8_t identifier, const KeyBinding* binding)
    : m_key_binding(binding)                 // Key bindings for this player
    , m_current_mission_status(MissionStatus::kMissionRunning)
    , m_identifier(identifier)               // Player ID
    , m_socket(socket)                       // Network socket (nullptr if local game)
{
    InitialiseActions(); // Setup all action -> command mappings

    // Set all commands to affect player aircraft category
    for (auto& pair : m_action_binding)
    {
        pair.second.category = static_cast<unsigned int>(ReceiverCategories::kRedTank);
    }
}

// Handles key press/release events
void Player::HandleEvent(const sf::Event& event, CommandQueue& command_queue)
{
    const auto* key_pressed = event.getIf<sf::Event::KeyPressed>();
    if (key_pressed)
    {
        Action action;

        // Check if key corresponds to an action and is NOT realtime (e.g. fire missile)
        if (m_key_binding && m_key_binding->CheckAction(key_pressed->scancode, action) && !IsRealtimeAction(action))
        {
            // If connected to network -> send event to server
            if (m_socket)
            {
                sf::Packet packet;
                packet << static_cast<uint8_t>(Client::PacketType::kPlayerEvent);
                packet << m_identifier;
                packet << static_cast<uint8_t>(action);
                m_socket->send(packet);
            }
            // Otherwise execute locally (single player)
            else
            {
                command_queue.Push(m_action_binding[action]);
            }
        }
    }

    // Structure to track key press/release state
    struct KeyStatus {
        sf::Keyboard::Scancode code;
        bool isPressed;
    };

    std::optional<KeyStatus> keyData;

    // Detect press
    if (const auto* press = event.getIf<sf::Event::KeyPressed>())
        keyData = { press->scancode, true };
    // Detect release
    else if (const auto* release = event.getIf<sf::Event::KeyReleased>())
        keyData = { release->scancode, false };

    // Handle realtime input over network (movement keys)
    if (keyData && m_socket)
    {
        Action action;
        if (m_key_binding && m_key_binding->CheckAction(keyData->code, action) && IsRealtimeAction(action))
        {
            // Send realtime input change to server
            sf::Packet packet;
            packet << static_cast<uint8_t>(Client::PacketType::kPlayerRealtimeChange);
            packet << m_identifier;
            packet << static_cast<uint8_t>(action);
            packet << keyData->isPressed;
            m_socket->send(packet);
        }
    }
}

// Returns true if this player is controlled locally
bool Player::IsLocal() const
{
    // No key binding means this player is remote
    return m_key_binding != nullptr;
}

// Enables/disables all realtime actions over network
void Player::DisableAllRealtimeActions(bool enable)
{
    for (auto& action : m_action_proxies)
    {
        sf::Packet packet;
        packet << static_cast<uint8_t>(Client::PacketType::kPlayerRealtimeChange);
        packet << m_identifier;
        packet << static_cast<uint8_t>(action.first);
        packet << enable;
        m_socket->send(packet);
    }
}

// Handles realtime input (movement keys held down)
void Player::HandleRealTimeInput(CommandQueue& command_queue)
{
    // If local player in multiplayer OR single player
    if ((m_socket && IsLocal()) || !m_socket)
    {
        // Get all currently held realtime keys
        std::vector<Action> activeActions = m_key_binding->GetRealtimeActions();

        // Debug: print held realtime keys
        for (Action action : activeActions)
        {
            //std::cout << "Realtime key held: " << static_cast<int>(action) << std::endl;
            command_queue.Push(m_action_binding[action]);
        }
    }
}

// Handles realtime input received from network
void Player::HandleRealtimeNetworkInput(CommandQueue& commands)
{
    if (m_socket && !IsLocal())
    {
        // For remote players, use proxy states
        for (auto pair : m_action_proxies)
        {
            if (pair.second && IsRealtimeAction(pair.first))
                commands.Push(m_action_binding[pair.first]);
        }
    }
}

// Handles a network event (like fire)
void Player::HandleNetworkEvent(Action action, CommandQueue& commands)
{
    commands.Push(m_action_binding[action]);
}

// Handles realtime network input change (key pressed/released)
void Player::HandleNetworkRealtimeChange(Action action, bool actionEnabled)
{
    m_action_proxies[action] = actionEnabled;
}

// Set mission/game status
void Player::SetMissionStatus(MissionStatus status)
{
    m_current_mission_status = status;
}

// Get mission/game status
MissionStatus Player::GetMissionStatus() const
{
    return m_current_mission_status;
}

// Setup action -> command mappings
void Player::InitialiseActions()
{
    // Movement
    m_action_binding[Action::kMoveLeft].action = DerivedAction<Tank>(AircraftMover(-1, 0.f, m_identifier));
    m_action_binding[Action::kMoveRight].action = DerivedAction<Tank>(AircraftMover(+1, 0.f, m_identifier));
    m_action_binding[Action::kMoveUp].action = DerivedAction<Tank>(AircraftMover(0.f, -1, m_identifier));
    m_action_binding[Action::kMoveDown].action = DerivedAction<Tank>(AircraftMover(0.f, 1, m_identifier));

    // Weapons
    m_action_binding[Action::kBulletFire].action = DerivedAction<Tank>(AircraftFireTrigger(m_identifier));
}