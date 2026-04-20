#include "player.hpp"
#include "command_queue.hpp"
#include "tank.hpp"
#include "network_protocol.hpp"
#include "turret.hpp"

#include <SFML/Network/Packet.hpp>
#include <map>

/// <summary>
/// CA1 Unmodified
/// </summary>
sf::Angle CalculateRotation(float x, float y)
{
    // Returns radians
    float radians = std::atan2(y, x);

    return sf::radians(radians);
}

/// <summary>
/// Updated to take mouse position. Updated to only message turrets whose IDs match.
/// CA1 Modified Ben
/// </summary>
struct TurretRotator
{
    TurretRotator(const sf::Vector2f& mousePos, int identifier)
        : mouse_position(mousePos)
		, turret_id(identifier)
    {
    }

    void operator()(Turret& turret, sf::Time) const
    {
        // (Ben) Only move the turret that belongs to this player
        if (turret.GetIdentifier() == turret_id)
        {
            sf::Angle angle = Turret::CalculateMouseRotation(turret.GetWorldPosition(), mouse_position);
            turret.setRotation(angle - turret.GetParent()->GetWorldRotation());
        }
    }

    sf::Vector2f mouse_position;
    int turret_id;
};

/// <summary>
/// Updated to take id
/// </summary>
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
        // (Ben) Rotate tank
        sf::Angle angle = CalculateRotation(velocity.x, velocity.y);

        // Only move the tank that belongs to this player
        if (aircraft.GetIdentifier() == aircraft_id)
        {
            aircraft.setRotation(angle);
            aircraft.Accelerate(velocity * 200.f);
        }
    }

    sf::Vector2f velocity;
    int aircraft_id;
};

/// <summary>
/// Unmodified
/// </summary>
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

/// <summary>
/// Modified to take window as a parameter. This allows for mouse aiming to be implemented.
/// Class modified: Ben with assistance of Claude, Kaylon with assistance of claude
/// </summary>
Player::Player(sf::TcpSocket* socket, uint8_t identifier, const KeyBinding* binding, sf::RenderWindow* window)
    : m_key_binding(binding)                 // Keybindings
    , m_identifier(identifier)               // ID
    , m_socket(socket)                       // Network socket
    , m_window(window)
{
    InitialiseActions(); // Setup all action -> command mappings

    // Set all commands to affect player aircraft category
    for (auto& pair : m_action_binding)
    {
        pair.second.category = static_cast<unsigned int>(ReceiverCategories::kTank);
    }
}

/// <summary>
/// Updated to aim using mouse position instead of joystick
/// CA1 Modified: Ben
/// </summary>
Command Player::AnalogueAiming(const sf::Vector2f& mousePos)
{
    Command rotate;
    rotate.category = static_cast<unsigned int>(ReceiverCategories::kTurret);
    rotate.action = DerivedAction<Turret>(TurretRotator(mousePos, m_identifier));
    return rotate;
}

/// <summary>
/// Modified: Kaylon with Claude
/// Update projetile firing paket to include position and roation data
/// </summary>
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

                // If firing, append the authoritative spawn position and turret rotation
                if (action == Action::kBulletFire)
                {
                    // Pass the tank's world position and turret rotation 
                    packet << m_fire_position.x << m_fire_position.y << m_fire_rotation;
                }
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

    if (keyData && m_socket)
    {
        Action action;
        if (m_key_binding && m_key_binding->CheckAction(keyData->code, action) && IsRealtimeAction(action))
        {
            sf::Packet packet;
            packet << static_cast<uint8_t>(Client::PacketType::kPlayerRealtimeChange);
            packet << m_identifier;
            packet << static_cast<uint8_t>(action);
            packet << keyData->isPressed;
            m_socket->send(packet);
        }
    }
}

/// <summary>
/// Unmodified
/// </summary>
bool Player::IsLocal() const
{
    // No key binding means this player is remote
    return m_key_binding != nullptr;
}

/// <summary>
/// Unmodified
/// </summary>
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

/// <summary>
/// Updated to add diagonal movement using a combined vector
/// Modified: Ben's Claude
/// </summary>
/// <param name="commands"></param>
void Player::HandleRealtimeNetworkInput(CommandQueue& commands)
{
    if (m_socket && !IsLocal())
    {
        sf::Vector2f combined_velocity(0.f, 0.f);
        for (auto& pair : m_action_proxies)
        {
            if (!pair.second || !IsRealtimeAction(pair.first)) continue;
            switch (pair.first)
            {
            case Action::kMoveLeft:  combined_velocity.x -= 1.f; break;
            case Action::kMoveRight: combined_velocity.x += 1.f; break;
            case Action::kMoveUp:    combined_velocity.y -= 1.f; break;
            case Action::kMoveDown:  combined_velocity.y += 1.f; break;
            default:
                commands.Push(m_action_binding[pair.first]);
                break;
            }
        }

        PushCombinedMoveCommand(commands, combined_velocity);
    }
}

/// <summary>
/// Updated to add diagonal movement using a combined vector. Modified to pass mouse position for aiming
/// Modified: Ben's Claude
/// </summary>
/// <param name="commands"></param>
void Player::HandleRealTimeInput(CommandQueue& command_queue, const sf::View& world_view)
{
    if ((m_socket && IsLocal()) || !m_socket)
    {
        std::vector<Action> activeActions = m_key_binding->GetRealtimeActions();

        // (Claude) Accumulate all active movement directions into one vector
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
                // (Claude) Non-movement realtime actions still push their own commands
                command_queue.Push(m_action_binding[action]);
                break;
            }
        }

        // (Claude) Only push a move command if there's actual input
        if (combined_velocity.x != 0.f || combined_velocity.y != 0.f)
        {
            // (Claude) Normalise so diagonal speed matches cardinal speed
            float length = std::sqrt(combined_velocity.x * combined_velocity.x
                + combined_velocity.y * combined_velocity.y);
            combined_velocity /= length;

            Command move;
            move.category = static_cast<unsigned int>(ReceiverCategories::kTank);
            move.action = DerivedAction<Tank>(AircraftMover(
                combined_velocity.x, combined_velocity.y, m_identifier));
            command_queue.Push(move);
        }

        // (Ben's Claude) Trigger analogue aiming on mouse
        sf::Vector2i mouseScreen = sf::Mouse::getPosition(*m_window);
        sf::Vector2f mouseWorld = m_window->mapPixelToCoords(mouseScreen, world_view);
        command_queue.Push(AnalogueAiming(mouseWorld));
    }
}

/// <summary>
/// Calculate velocity and pass it into a command
/// Author: Ben's Claude
/// </summary>
/// <param name="commands"></param>
/// <param name="velocity"></param>
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

/// <summary>
/// Modified: Kaylon's Claude
/// </summary>
void Player::HandleNetworkEvent(Action action, CommandQueue& commands)
{
    if (action == Action::kBulletFire && m_on_remote_fire)
        m_on_remote_fire(m_fire_position, m_fire_rotation);

    commands.Push(m_action_binding[action]);
}

/// <summary>
/// Unmodified
/// </summary>
void Player::HandleNetworkRealtimeChange(Action action, bool actionEnabled)
{
    m_action_proxies[action] = actionEnabled;
}

/// <summary>
/// Modified to remove missile action
/// Modified: Ben
/// </summary>
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