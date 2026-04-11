#include "settings_state.hpp"
#include "Utility.hpp"
#include "action.hpp"
#include "key_binding.hpp"

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
    back_button->setPosition(sf::Vector2f(80.f, 620.f));
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

    //Iterate through all of the key binding buttons to see if they are being pressed, waiting for input from the user
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

void SettingsState::AddButtonLabel(std::size_t index, std::size_t x, std::size_t y, const std::string& text, Context context)
{
    // For x==0, start at index 0, otherwise start at half of array
    index += static_cast<int>(Action::kActionCount) * x;

    m_binding_buttons[index] = std::make_shared<gui::Button>(context);
    m_binding_buttons[index]->setPosition(sf::Vector2f(400.f * x + 80.f, 50.f * y + 300.f));
    m_binding_buttons[index]->SetText(text);
    m_binding_buttons[index]->SetToggle(true);

    m_binding_labels[index] = std::make_shared<gui::Label>("", *context.fonts);
    m_binding_labels[index]->setPosition(sf::Vector2f(400.f * x + 300.f, 50.f * y + 315.f));

    m_gui_container.Pack(m_binding_buttons[index]);
    m_gui_container.Pack(m_binding_labels[index]);
}
