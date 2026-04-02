#include "pause_state.hpp"
#include "utility.hpp"

/// <summary>
/// Modified: Ben Mc Keever D00254413
/// Now message reflects use of controller
/// </summary>
/// <param name="stack"></param>
/// <param name="context"></param>
PauseState::PauseState(StateStack& stack, Context context) : State(stack, context), m_paused_text(context.fonts->Get(FontID::kMain)), m_instruction_text(context.fonts->Get(FontID::kMain))
{
    sf::Vector2f view_size = context.window->getView().getSize();

    m_paused_text.setString("Game Paused");
    m_paused_text.setCharacterSize(70);
    Utility::CentreOrigin(m_paused_text);
    m_paused_text.setPosition(sf::Vector2f(0.5f * view_size.x, 0.4f * view_size.y));

    m_instruction_text.setString("Press back to return to the main menu, start to return to the game");
    Utility::CentreOrigin(m_instruction_text);
    m_instruction_text.setPosition(sf::Vector2f(0.5f * view_size.x, 0.6f * view_size.y));
    GetContext().music->SetPaused(true);
}

void PauseState::Draw()
{
    sf::RenderWindow& window = *GetContext().window;
    window.setView(window.getDefaultView());

    sf::RectangleShape backgroundShape;
    backgroundShape.setFillColor(sf::Color(0, 0, 0, 150));
    backgroundShape.setSize(window.getView().getSize());

    window.draw(backgroundShape);
    window.draw(m_paused_text);
    window.draw(m_instruction_text);
}

bool PauseState::Update(sf::Time dt)
{
    return false;
}

PauseState::~PauseState()
{
    GetContext().music->SetPaused(false);
}

/// <summary>
/// Modified: Ben Mc Keever D00254413
/// Now takes joystick input to unpause or return to menu, rather than keyboard input. (Copilot)
/// </summary>
/// <param name="event"></param>
/// <returns></returns>
bool PauseState::HandleEvent(const sf::Event& event)
{
    const auto* joy_pressed = event.getIf<sf::Event::JoystickButtonPressed>();

    if (!joy_pressed)
    {
        return false;
    }
    if (joy_pressed->button == static_cast<int>(XboxLayout::Start))
    {
        RequestStackPop();
    }
    if (joy_pressed->button == static_cast<int>(XboxLayout::Back))
    {
        RequestStackClear();
        RequestStackPush(StateID::kMenu);
    }
    return false;
}