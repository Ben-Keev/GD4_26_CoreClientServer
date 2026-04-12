#include "button.hpp"
#include "fontID.hpp"
#include "utility.hpp"
#include "SFML/Graphics/Rect.hpp"
#include "texture_id.hpp"

gui::Button::Button(State::Context context)
    : m_sprite(context.textures->Get(TextureID::kButtons))
    , m_text(context.fonts->Get(FontID::kMain), "", 50)
    , m_is_toggle(false)
    , m_sounds(*context.sound)
{
    ChangeTexture(ButtonType::kNormal);
    sf::FloatRect bounds = m_sprite.getLocalBounds();
    m_text.setPosition(sf::Vector2f(bounds.size.x / 2, bounds.size.y / 2));
}

void gui::Button::SetCallback(Callback callback)
{
    m_callback = std::move(callback);
}

void gui::Button::SetToggle(bool flag)
{
    m_is_toggle = flag;
}

void gui::Button::SetText(const std::string& text)
{
    m_text.setString(text);
    Utility::CentreOrigin(m_text);
}

bool gui::Button::IsSelectable() const
{
    return true;
}

void gui::Button::Select()
{
    Component::Select();
    ChangeTexture(ButtonType::kSelected);
}

void gui::Button::Deselect()
{
    Component::Deselect();
    ChangeTexture(ButtonType::kNormal);
}

void gui::Button::Activate()
{
    Component::Activate();
    if (m_is_toggle)
    {
        ChangeTexture(ButtonType::kPressed);
    }
    if (m_callback)
    {
        m_callback();
    }
    if (!m_is_toggle)
    {
        Deactivate();
    }
    m_sounds.Play(SoundEffect::kButton);
}

void gui::Button::Deactivate()
{
    Component::Deactivate();
    if (m_is_toggle)
    {
        if (IsSelected())
        {
            ChangeTexture(ButtonType::kSelected);
        }
        else
        {
            ChangeTexture(ButtonType::kNormal);
        }
    }
}

// Claude - Add mouse support
void gui::Button::HandleEvent(const sf::Event& event)
{
    const auto* mouse_moved = event.getIf<sf::Event::MouseMoved>();
    if (mouse_moved)
    {
        sf::Vector2f mouse(static_cast<float>(mouse_moved->position.x),
            static_cast<float>(mouse_moved->position.y));

        if (GetBoundingRect().contains(mouse))
        {
            Select();
        }
        else
        {
            Deselect();
        }
    }

    const auto* mouse_pressed = event.getIf<sf::Event::MouseButtonPressed>();
    if (mouse_pressed && mouse_pressed->button == sf::Mouse::Button::Left)
    {
        sf::Vector2f mouse(static_cast<float>(mouse_pressed->position.x),
            static_cast<float>(mouse_pressed->position.y));

        if (GetBoundingRect().contains(mouse))
        {
            Activate();
        }
    }
}

// Claude - Add a function to get the bounding rect of the button, taking into account its transform
sf::FloatRect gui::Button::GetBoundingRect() const
{
    return getTransform().transformRect(m_sprite.getGlobalBounds());
}

void gui::Button::draw(sf::RenderTarget& target, sf::RenderStates states) const
{
    states.transform *= getTransform();
    target.draw(m_sprite, states);
    target.draw(m_text, states);
}

void gui::Button::ChangeTexture(ButtonType buttonType)
{
    sf::IntRect textureRect({ 0, 147 * static_cast<int>(buttonType) }, { 465, 147 });
    m_sprite.setTextureRect(textureRect);
}

