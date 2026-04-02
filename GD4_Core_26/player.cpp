#include "player.hpp"
#include "tank.hpp"
#include "SFML/Window/Joystick.hpp"
#include <iostream>

/// <summary>
/// Modified: Ben Mc Keever D00254413
/// Modified: Kaylon Riordan D00255039
/// </summary>

/// <summary>
/// Calculates sprite rotation angle from joystick axis values. (GPT)
/// </summary>
sf::Angle CalculateRotation(float x, float y)
{
    // Compute aim angle https://godotforums.org/d/25705-2d-joystick-rotation/2#:~:text=If%20you%20have%20a%20Vector2%20with%20the%20joystick%20value%2C%20then%20you%20can%20convert%20it%20to%20a%20rotation%20using%20atan2
    // Returns radians
    float radians = std::atan2(y, x);

    // Rotate an added 90 degrees so that the sprite renders upright
    return sf::radians(radians) + sf::degrees(-90.f);
}

/// <summary>
/// Now calculates rotation of tank
/// Functor used to move and rotate the tank based on analogue stick input. (GPT)
/// Modified: Kaylon Riordan D00255039
/// Updated method to rotate tank based off its movement angle so it faces the direction its moving
/// </summary>
struct TankMover
{
    TankMover(float x, float y, float vx, float vy) : axis(x, y), velocity(vx, vy) {}

    /// <summary>
    /// Applies rotation and acceleration to the tank using current axis input. (GPT)
    /// </summary>
    void operator()(Tank& tank, sf::Time) const
    {
        sf::Angle angle = CalculateRotation(axis.x, axis.y);

        tank.setRotation(angle);

        tank.Accelerate(velocity);
    }

    sf::Vector2f axis;      // Raw joystick axis values (GPT)
    sf::Vector2f velocity;  // Calculated velocity from axis input (GPT)
};

/// <summary>
/// Authored: Ben Mc Keever D00254413
/// Functor used to rotate the turret independently from the tank body. (GPT)
/// Modified: Kaylon Riordan D00255039
/// Updated method to rotate turret instead of tank to achieve appearance of turret aiming on top of tank
/// </summary>
struct TurretRotator
{
    TurretRotator(float x, float y) : axis(x, y) {}

    /// <summary>
    /// Rotates turret relative to parent tank rotation. (GPT)
    /// </summary>
    void operator()(Turret& turret, sf::Time) const
    {
        sf::Angle angle = CalculateRotation(axis.x, axis.y);

        turret.setRotation(angle - turret.GetParent()->GetWorldRotation());
    }

    sf::Vector2f axis; // Raw joystick axis values for aiming (GPT)
};

/// <summary>
/// Modified: Ben Mc Keever D00254413
/// Now takes joystick number to support multiple players with separate controls. (Copilot)
/// Constructs a Player and binds default controls for a specific joystick. (GPT)
/// </summary>
/// <param name="joystick_number"></param>
Player::Player(int joystick_number)
{
    this->joystick_number = joystick_number;

    // Default binding: A button fires (GPT)
    m_joystick_binding[XboxLayout::RB] = Action::kBulletFire;

    InitialiseActions();

    // Assign categories based on which joystick/player this is (GPT)
    if (joystick_number == 0) {
        tankCategory = ReceiverCategories::kRedTank;
        turretCategory = ReceiverCategories::kRedTurret;
    }

    if (joystick_number == 1) {
        tankCategory = ReceiverCategories::kBlueTank;
        turretCategory = ReceiverCategories::kBlueTurret;
    }

    // Apply tank category to all action bindings (GPT)
    for (auto& pair : m_action_binding)
    {
        pair.second.category = static_cast<unsigned int>(tankCategory);
    }
}

/// <summary>
/// Modified: Ben Mc Keever D00254413
/// Now checks if the event is a joystick button press and matches the player's joystick number. (Copilot)
/// Handles discrete joystick button press events. (GPT)
/// </summary>
/// <param name="event"></param>
/// <param name="command_queue"></param>
void Player::HandleEvent(const sf::Event& event, CommandQueue& command_queue)
{
    const auto* joy_pressed = event.getIf<sf::Event::JoystickButtonPressed>();
    if (joy_pressed)
    {
        auto found = m_joystick_binding.find(static_cast<XboxLayout>(joy_pressed->button));

        // Trigger command if bound and not real-time (GPT)
        if (found != m_joystick_binding.end() && !IsRealTimeAction(found->second))
        {
            command_queue.Push(m_action_binding[found->second]);
        }
    }

}

/// <summary>
/// Modified: Ben Mc Keever D00254413
/// Now processes analogue stick input for movement and aiming, as well as held buttons. (Copilot)
/// Handles continuous real-time joystick input (movement, aiming, held buttons). (GPT)
/// </summary>
/// <param name="command_queue"></param>
void Player::HandleRealTimeInput(CommandQueue& command_queue)
{

    // Axes of left thumbstick
    float x = sf::Joystick::getAxisPosition(joystick_number, sf::Joystick::Axis::X);
    float y = sf::Joystick::getAxisPosition(joystick_number, sf::Joystick::Axis::Y);

    // Axes of right stick
    float u = sf::Joystick::getAxisPosition(joystick_number, sf::Joystick::Axis::U);
    float v = sf::Joystick::getAxisPosition(joystick_number, sf::Joystick::Axis::V);

    // Deadzone for analogue sticks
    const float deadZone = 15.f;

    // Movement input if outside deadzone (GPT)
    if (std::abs(x) > deadZone || std::abs(y) > deadZone)
    {
        command_queue.Push(AnalogueMovement(x, y));
    }

    // Aiming input if outside deadzone (GPT)
    if (std::abs(u) > deadZone || std::abs(v) > deadZone)
    {
        command_queue.Push(AnalogueAiming(u, v));
    }

    // Buttons (real-time)
    for (auto pair : m_joystick_binding)
    {
        if (sf::Joystick::isButtonPressed(joystick_number,
            static_cast<unsigned int>(pair.first)) &&
            IsRealTimeAction(pair.second))
        {
            command_queue.Push(m_action_binding[pair.second]);
        }
    }
}

/// <summary>
/// Authored: Ben Mc Keever D00254413
/// Creates a movement command based on analogue stick position. (GPT)
/// </summary>
/// <param name="x"></param>
/// <param name="y"></param>
/// <returns></returns>
Command Player::AnalogueMovement(float x, float y)
{
    // Speed the player moves
    const float kPlayerSpeed = 200.f;

    // The command must be called in update so it can reflect the analogue values of the thumbstick
    Command move;

    // As with other actions set category
    move.category = static_cast<unsigned int>(tankCategory);

    // Define the action using live axes values as opposed to preset speed
    move.action = DerivedAction<Tank>(
        TankMover({
            x,
            y,
            kPlayerSpeed * (x / 100.f),
            kPlayerSpeed * (y / 100.f),
        })
    );

    return move;
}

/// <summary>
/// Authored: Ben Mc Keever D00254413
/// Creates a turret rotation command based on right analogue stick. (GPT)
/// </summary>
/// <param name="u"></param>
/// <param name="v"></param>
/// <returns></returns>
Command Player::AnalogueAiming(float u, float v)
{
    // The command must be called in update so it can reflect the analogue values of the thumbstick
    Command rotate;

    // As with other actions set category
    rotate.category = static_cast<unsigned int>(turretCategory);

    rotate.action = DerivedAction<Turret>(
        TurretRotator({ u, v })
    );

    return rotate;
}

/// <summary>
/// Modified: Ben Mc Keever D00254413
/// Now assigns keys based on Xbox controller layout. (Copilot)
/// Rebinds an action to a new Xbox button, removing previous bindings. (GPT)
/// </summary>
/// <param name="action"></param>
/// <param name="button"></param>
void Player::AssignKey(Action action, XboxLayout button)
{
    //Remove keys that are currently bound to the action
    for (auto itr = m_joystick_binding.begin(); itr != m_joystick_binding.end();)
    {
        if (itr->second == action)
        {
            m_joystick_binding.erase(itr++);
        }
        else
        {
            ++itr;
        }
    }

    m_joystick_binding[button] = action;
}

/// <summary>
/// Get the button assigned to an action
/// Modified: Ben Mc Keever D00254413
/// Now returns the Xbox button assigned to an action, or -1 if no binding exists. (Copilot)
/// Returns the integer representation of the bound Xbox button,
/// or -1 if no binding exists. (GPT)
/// </summary>
/// <param name="action">action</param>
/// <returns>the integer the button is tied to, -1 if none found</returns>
int Player::GetAssignedKey(Action action) const
{
    for (auto pair : m_joystick_binding)
    {
        if (pair.second == action)
        {
            return static_cast<int>(pair.first);
        }
    }
    return -1;
}

/// <summary>
/// Sets the player's current mission status. (GPT)
/// </summary>
void Player::SetMissionStatus(MissionStatus status)
{
    m_current_mission_status = status;
}

/// <summary>
/// Gets the player's current mission status. (GPT)
/// </summary>
MissionStatus Player::GetMissionStatus() const
{
    return m_current_mission_status;
}

/// <summary>
/// Initialises available player actions and their corresponding commands. (GPT)
/// </summary>
void Player::InitialiseActions()
{
    m_action_binding[Action::kBulletFire].action =
    DerivedAction<Tank>([](Tank& a, sf::Time)
    {
        a.Fire();
    });
}

/// <summary>
/// Determines whether an action should be processed as real-time input. (GPT)
/// </summary>
bool Player::IsRealTimeAction(Action action)
{
    switch (action)
    {
    case Action::kMoveLeft:
    case Action::kMoveRight:
    case Action::kMoveUp:
    case Action::kMoveDown:
    case Action::kBulletFire:
        return true;
    default:
        return false;
    }
}

/// <summary>
/// Authored: Ben Mc Keever D00254413
/// Destructor outputs debug information when player is destroyed. (GPT)
/// </summary>
Player::~Player()
{
    std::cout << "Destroyed player " << joystick_number << std::endl;
}
