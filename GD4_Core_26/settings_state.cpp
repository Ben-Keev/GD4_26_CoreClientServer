#include "SocketWrapperPCH.hpp"
#include "SocketWrapperPCH.hpp"
#include "settings_state.hpp"
#include "Utility.hpp"

/// <summary>
/// Modified: Ben Mc Keever D00254413
/// Now includes buttons for fire of both players and removes movement/missile binding
/// Constructs the SettingsState and initializes GUI elements,
/// including binding buttons and the background sprite. (GPT)
/// </summary>
/// <param name="stack"></param>
/// <param name="context"></param>
SettingsState::SettingsState(StateStack& stack, Context context)
    : State(stack, context)
    , m_gui_container()
    , m_background_sprite(
        context.textures->Get(TextureID::kTitleScreen))
{
    // Add binding button for Player 1 fire action (GPT)
    AddButtonLabel(Action::kBulletFire, 0, 250.f, "Fire P1", context);

    // Add binding button for Player 2 fire action (GPT)
    AddButtonLabel(Action::kBulletFire, 1, 350.f, "Fire P2", context);

    // Update button labels to reflect current bindings (GPT)
    UpdateLabels();

    // Create Back button to return to previous state (GPT)
    auto back_button = std::make_shared<gui::Button>(context);
    back_button->setPosition(sf::Vector2f(80.f, 475.f));
    back_button->SetText("Back");

    // When pressed, pop this state off the stack (GPT)
    back_button->SetCallback(std::bind(&SettingsState::RequestStackPop, this));

    m_gui_container.Pack(back_button);
}

/// <summary>
/// Draws the settings background and GUI container. (GPT)
/// </summary>
void SettingsState::Draw()
{
    sf::RenderWindow& window = *GetContext().window;

    window.draw(m_background_sprite);
    window.draw(m_gui_container);
}

/// <summary>
/// Updates GUI container for real-time input handling. (GPT)
/// </summary>
/// <param name="dt"></param>
bool SettingsState::Update(sf::Time dt)
{
    m_gui_container.HandleRealtimeInput();
    return true;
}

/// <summary>
/// Modified: Ben Mc Keever D00254413
/// Now rebinds joystick buttons depending on which player the button and action are bound to
/// Handles joystick input for rebinding controls,
/// or forwards events to the GUI container if not rebinding. (GPT)
/// </summary>
/// <param name="event"></param>
/// <returns></returns>
bool SettingsState::HandleEvent(const sf::Event& event)
{
    bool is_key_binding = false;

    // Loop through both players (GPT)
    for (int player = 0; player < 2; ++player)
    {
        // Only allow rebinding for gameplay actions (starting at index 4) (GPT)
        for (std::size_t action = 4; action < static_cast<int>(Action::kActionCount); ++action)
        {
            auto& button = m_binding_buttons[player][action];

            // If button is active, we are currently waiting for input (GPT)
            if (button->IsActive())
            {
                is_key_binding = true;

                const auto* joy_released = event.getIf<sf::Event::JoystickButtonReleased>();

                // Check if joystick button was released for correct player (GPT)
                if (joy_released && joy_released->joystickId == player)
                {
                    Player* target = (player == 0) ? GetContext().red_player : GetContext().blue_player;

                    // Assign released button to selected action (GPT)
                    target->AssignKey(
                        static_cast<Action>(action),
                        static_cast<XboxLayout>(joy_released->button)
                    );

                    // Stop listening for input (GPT)
                    button->Deactivate();
                }

                break;
            }
        }
    }

    // If we were rebinding, refresh labels (GPT)
    if (is_key_binding)
    {
        UpdateLabels();
    }
    else
    {
        // Otherwise forward event to GUI container (GPT)
        m_gui_container.HandleEvent(event);
    }

    return false;
}

/// <summary>
/// Modified: Ben Mc Keever D00254413
/// Now accesses button through player bindings and displays controller layout buttons
/// Updates on-screen labels to reflect current button bindings
/// for both players. (GPT)
/// </summary>
void SettingsState::UpdateLabels()
{
    Player& red_player = *GetContext().red_player;
    Player& blue_player = *GetContext().blue_player;

    // Loop through both players (GPT)
    for (int player = 0; player < 2; ++player)
    {
        Player& target = (player == 0) ? red_player : blue_player;

        // Loop through bindable actions (GPT)
        for (std::size_t i = 4; i < static_cast<int>(Action::kActionCount); ++i)
        {
            int assigned = target.GetAssignedKey(static_cast<Action>(i));
            std::string text = ButtonIntegerToXboxString(assigned);

            // Update label text if label exists (GPT)
            if (m_binding_labels[player][i])
                m_binding_labels[player][i]->SetText(text);
        }
    }
}

// Convert an integer from XBOX button into string for display

/// <summary>
/// Authored: Ben Mc Keever D00254413
/// Converts an Xbox controller button index into a readable string
/// for display in the settings menu. (GPT)
/// </summary>
/// <param name="button"></param>
/// <returns></returns>
std::string SettingsState::ButtonIntegerToXboxString(int button)
{
    switch (button)
    {
    case 0:  return "A";
    case 1:  return "B";
    case 2:  return "X";
    case 3:  return "Y";
    case 4:  return "LB";
    case 5:  return "RB";
    case 6:  return "Back";
    case 7:  return "Start";
    case 8:  return "LStick";
    case 9:  return "RStick";
    default: return "Invalid";
    }
}

// Creates a binding button and label for a specific player/action

/// <summary>
/// Modified: Ben Mc Keever D00254413
/// Now stores player index and action index by button so these can be easily accessed per player
/// Creates a toggleable binding button and corresponding label
/// for a specific player and action. (GPT)
/// </summary>
/// <param name="action"></param>
/// <param name="playerIndex"></param>
/// <param name="y"></param>
/// <param name="text"></param>
/// <param name="context"></param>
void SettingsState::AddButtonLabel(
    Action action,
    int playerIndex,
    float y,
    const std::string& text,
    Context context)
{
    int actionIndex = static_cast<int>(action);

    // Create binding button (GPT)
    auto button = std::make_shared<gui::Button>(context);

    button->setPosition(sf::Vector2f(80.f, y));
    button->SetText(text);
    button->SetToggle(true);

    // Store button reference for later access (GPT)
    m_binding_buttons[playerIndex][actionIndex] = button;

    // Create label to display assigned button (GPT)
    auto label = std::make_shared<gui::Label>("", *context.fonts);

    label->setPosition(sf::Vector2f(300.f, y + 15.f));

    // Store label reference (GPT)
    m_binding_labels[playerIndex][actionIndex] = label;

    // Add button and label to GUI container (GPT)
    m_gui_container.Pack(button);
    m_gui_container.Pack(label);
}