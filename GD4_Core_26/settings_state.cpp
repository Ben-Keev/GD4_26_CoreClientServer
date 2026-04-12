#include "settings_state.hpp"
#include "Utility.hpp"
#include "action.hpp"
#include "key_binding.hpp"

/// <summary>
/// Remove 2nd local player bindings
/// Modified: Ben and Kaylon with assistance of Claude
/// Modified: Kaylon Riordan D00255039
/// Fix button and text positions to fit with new texture
/// </summary>
SettingsState::SettingsState(StateStack& stack, Context context)
    : State(stack, context)
    , m_gui_container()
    , m_background_sprite(context.textures->Get(TextureID::kTitleScreen))
{
    AddButtonLabel(static_cast<int>(Action::kMoveLeft), 0, 0, "Move Left", context);
    AddButtonLabel(static_cast<int>(Action::kMoveRight), 0, 1, "Move Right", context);
    AddButtonLabel(static_cast<int>(Action::kMoveUp), 0, 2, "Move Up", context);
    AddButtonLabel(static_cast<int>(Action::kMoveDown), 0, 3, "Move Down", context);
    AddButtonLabel(static_cast<int>(Action::kBulletFire), 0, 4, "Fire", context);

    UpdateLabels();

    auto back_button = std::make_shared<gui::Button>(context);
    back_button->setPosition(sf::Vector2f(100.f, 750.f));
    back_button->SetText("Back");
    back_button->SetCallback(std::bind(&SettingsState::RequestStackPop, this));
    m_gui_container.Pack(back_button);
}

void SettingsState::Draw()
{
    sf::RenderWindow& window = *GetContext().window;
    window.draw(m_background_sprite);
    window.draw(m_gui_container);
}

bool SettingsState::Update(sf::Time dt)
{
    return true;
}

bool SettingsState::HandleEvent(const sf::Event& event)
{
    bool is_key_binding = false;

    // (Ben) Remove two player logic
    for (std::size_t action = 0; action < static_cast<int>(Action::kActionCount); ++action)
    {
        if (m_binding_buttons[action]->IsActive())
        {
            is_key_binding = true;
            const auto* key_event = event.getIf<sf::Event::KeyReleased>();
            if (key_event)
            {
                sf::Keyboard::Scancode pressed_key = key_event->scancode;
                GetContext().keys->AssignKey(static_cast<Action>(action), pressed_key);

                m_binding_buttons[action]->Deactivate();
            }
            break;
        }
    }

    if (is_key_binding)
    {
        UpdateLabels();
    }
    else
    {
        m_gui_container.HandleEvent(event);
    }
    return false;
}

/// <summary>
/// Remove 2 local player logic
/// Modified: Ben
/// </summary>
void SettingsState::UpdateLabels()
{
    for (std::size_t i = 0; i < static_cast<int>(Action::kActionCount); ++i)
    {
        auto action = static_cast<Action>(i);

        // Get keys
        sf::Keyboard::Scancode key = GetContext().keys->GetAssignedKey(action);

        // Assign key strings to labels
        m_binding_labels[i]->SetText(Utility::toString(key));
    }
}

/// <summary>
/// Modified: Kaylon Riordan D00255039
/// Changed size and position to fit new buttons
/// </summary>
void SettingsState::AddButtonLabel(std::size_t index, std::size_t x, std::size_t y, const std::string& text, Context context)
{
    index += static_cast<int>(Action::kActionCount) * x;

    m_binding_buttons[index] = std::make_shared<gui::Button>(context);
    m_binding_buttons[index]->setPosition(sf::Vector2f(400.f * x + 100.f, 150.f * y + 0.f));
    m_binding_buttons[index]->SetText(text);
    m_binding_buttons[index]->SetToggle(true);

    m_binding_labels[index] = std::make_shared<gui::Label>("", *context.fonts);
    m_binding_labels[index]->setPosition(sf::Vector2f(400.f * x + 480.f, 150.f * y + 40.f));

    m_gui_container.Pack(m_binding_buttons[index]);
    m_gui_container.Pack(m_binding_labels[index]);
}
