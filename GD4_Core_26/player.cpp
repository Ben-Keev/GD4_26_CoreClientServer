#include "SocketWrapperPCH.hpp"
#include "player.hpp"
#include "command_queue.hpp"
#include "tank.hpp"
#include "turret_type.hpp"

#include "network_protocol.hpp"
#include <SFML/Network/Packet.hpp>

#include <map>
#include "turret.hpp"
#include "application.hpp"
#include "multiplayer_gamestate.hpp"

sf::Angle CalculateRotation(float x, float y)
{
    // Returns radians
    float radians = std::atan2(y, x);

    return sf::radians(radians);
}

// Modified heavily to work with mouses
struct TurretRotator
{
    TurretRotator(const sf::Vector2f& mousePos, int identifier)
        : mouse_position(mousePos)
		, turret_id(identifier)
    {
    }

    void operator()(Turret& turret, sf::Time) const
    {
        // Only move the aircraft that belongs to this player
        if (turret.GetIdentifier() == turret_id)
        {
            sf::Angle angle = Turret::CalculateMouseRotation(turret.GetWorldPosition(), mouse_position);
            turret.setRotation(angle - turret.GetParent()->GetWorldRotation());
        }
    }

    sf::Vector2f mouse_position;
    int turret_id;
};

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
        sf::Angle angle = CalculateRotation(velocity.x, velocity.y);

        // Only move the aircraft that belongs to this player
        if (aircraft.GetIdentifier() == aircraft_id)
        {
            aircraft.setRotation(angle);
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
Player::Player(sf::TcpSocket* socket, uint8_t identifier, const KeyBinding* binding, sf::RenderWindow* window)
    : m_key_binding(binding)                 // Key bindings for this player
    , m_current_mission_status(MissionStatus::kMissionRunning)
    , m_identifier(identifier)               // Player ID
    , m_socket(socket)                       // Network socket (nullptr if local game)
    , m_window(window)
{
    InitialiseActions(); // Setup all action -> command mappings

    // Set all commands to affect player aircraft category
    for (auto& pair : m_action_binding)
    {
        pair.second.category = static_cast<unsigned int>(ReceiverCategories::kTank);
    }
}

Command Player::AnalogueAiming(const sf::Vector2f& mousePos)
{
    Command rotate;
    rotate.category = static_cast<unsigned int>(ReceiverCategories::kTurret);
    rotate.action = DerivedAction<Turret>(TurretRotator(mousePos, m_identifier));
    return rotate;
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

// Claude - set current network velocity (called by GameState on receipt of kPlayerVelocityUpdate)
void Player::SetNetworkVelocity(sf::Vector2f velocity)
{
    m_current_network_velocity = velocity;
}

// Claude - current network velocity
void Player::HandleRealtimeNetworkInput(CommandQueue& commands)
{
    if (m_socket && !IsLocal())
    {
        // Handle non-movement realtime actions (e.g. firing)
        for (auto& pair : m_action_proxies)
        {
            if (!pair.second || !IsRealtimeAction(pair.first)) continue;

            if (pair.first == Action::kBulletFire)
                commands.Push(m_action_binding[pair.first]);
        }

        // Push movement once using the received network velocity
        PushCombinedMoveCommand(commands, m_current_network_velocity);
    }
}

// Claude - added diagonal movement
void Player::PushCombinedMoveCommand(CommandQueue& commands, sf::Vector2f velocity)
{
    if (velocity.x != 0.f || velocity.y != 0.f)
    {
        float length = std::sqrt(velocity.x * velocity.x + velocity.y * velocity.y);
        velocity /= length;

        Command move;
        move.category = static_cast<unsigned int>(ReceiverCategories::kTank);
        move.action = DerivedAction<Tank>(AircraftMover(velocity.x, velocity.y, m_identifier));
        commands.Push(move);
    }
}

// Claude - added diagonal movement
void Player::HandleRealTimeInput(CommandQueue& command_queue, const sf::View& world_view)
{
    if ((m_socket && IsLocal()) || !m_socket)
    {
        std::vector<Action> activeActions = m_key_binding->GetRealtimeActions();

        sf::Vector2f combined_velocity(0.f, 0.f);
        for (Action action : activeActions)
        {
            switch (action)
            {
            case Action::kMoveLeft:  combined_velocity.x -= 1.f; break;
            case Action::kMoveRight: combined_velocity.x += 1.f; break;
            case Action::kMoveUp:    combined_velocity.y -= 1.f; break;
            case Action::kMoveDown:  combined_velocity.y += 1.f; break;
            default:
                command_queue.Push(m_action_binding[action]);
                break;
            }
        }

        if (combined_velocity.x != 0.f || combined_velocity.y != 0.f)
        {
            float length = std::sqrt(combined_velocity.x * combined_velocity.x
                + combined_velocity.y * combined_velocity.y);
            combined_velocity /= length;

            Command move;
            move.category = static_cast<unsigned int>(ReceiverCategories::kTank);
            move.action = DerivedAction<Tank>(AircraftMover(
                combined_velocity.x, combined_velocity.y, m_identifier));
            command_queue.Push(move);

			// Send the final normalised velocity as a single atomic packet (Claude)
            if (m_socket)
            {
                sf::Packet packet;
                packet << static_cast<uint8_t>(Client::PacketType::kPlayerVelocityUpdate);
                packet << m_identifier;
                packet << combined_velocity.x;
                packet << combined_velocity.y;
                m_socket->send(packet);
            }
        }
        else
        {
            // Send zero velocity so remote tanks stop moving (Claude)
            if (m_socket)
            {
                sf::Packet packet;
                packet << static_cast<uint8_t>(Client::PacketType::kPlayerVelocityUpdate);
                packet << m_identifier;
                packet << 0.f;
                packet << 0.f;
                m_socket->send(packet);
            }
        }

        sf::Vector2i mouseScreen = sf::Mouse::getPosition(*m_window);
        sf::Vector2f mouseWorld = m_window->mapPixelToCoords(mouseScreen, world_view);
        command_queue.Push(AnalogueAiming(mouseWorld));
    }
}



sf::Vector2f Player::GetCombinedNetworkVelocity() const
{
    return sf::Vector2f();
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