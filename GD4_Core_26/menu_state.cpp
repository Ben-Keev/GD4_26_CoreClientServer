#include "menu_state.hpp"
#include "fontID.hpp"
#include "button.hpp"

#include <SFML/Graphics/Text.hpp>
#include <fstream>

/// <summary>
/// Read name from file
/// Authored: Kaylon's Claude
/// </summary>
std::string LoadPlayerName()
{
    std::ifstream input_file("details.txt");
    std::string name;
    if (input_file >> name)
    {
        if (name.size() > 5)
            name = name.substr(0, 5);
        return name;
    }
    // File missing — create with defaults
    std::ofstream output_file("details.txt");
    output_file << "Guest\n" << 0;
    return "Guest";
}

/// <summary>
/// Read Score from file
/// Authored: Kaylon's Claude
/// </summary>
int LoadHighScore()
{
    std::ifstream input_file("details.txt");
    std::string name;
    int high_score = 0;
    if (input_file >> name >> high_score) // reads line 1 then line 2
    {
        return high_score;
    }
    return 0;
}

/// <summary>
/// Write data to file
/// Authored: Kaylon's Claude
/// </summary>
void SaveDetails(const std::string& name, int high_score)
{
    std::ofstream output_file("details.txt");
    output_file << name << "\n" << high_score;
}

/// <summary>
/// Modified: Ben, Kaylon with assistance of Claude
/// </summary>
MenuState::MenuState(StateStack& stack, Context context) 
    : State(stack, context)
    , m_background_sprite(context.textures->Get(TextureID::kTitleScreen))
    , m_name_input_text(context.fonts->Get(FontID::kMain))
{
    // (Kaylon's Claude) Load saved name and apply to context
    m_name_input = LoadPlayerName();
    context.player_details->m_name = m_name_input;

    // (Kaylon's Claude) Setup name input display
    m_name_input_text.setCharacterSize(50);
    m_name_input_text.setFillColor(sf::Color::White);
    m_name_input_text.setOutlineColor(sf::Color::Black);
    m_name_input_text.setOutlineThickness(1.f);
    m_name_input_text.setPosition(sf::Vector2f(180.f, 60.f));
    m_name_input_text.setString("Name: " + m_name_input);

    m_name_input_box.setSize(sf::Vector2f(400.f, 80.f));
    m_name_input_box.setPosition(sf::Vector2f(130.f, 50.f));
    m_name_input_box.setFillColor(sf::Color(0, 0, 0, 150));
    m_name_input_box.setOutlineThickness(2.f);
    m_name_input_box.setOutlineColor(sf::Color::White);

    // (Ben) Remove host and play locally
    auto join_play_button = std::make_shared<gui::Button>(context);
    join_play_button->setPosition(sf::Vector2f(100, 150));
    join_play_button->SetText("Join");
    join_play_button->SetCallback([this]()
        {
            RequestStackPop();
            RequestStackPush(StateID::kJoinLobby);
        });

    auto settings_button = std::make_shared<gui::Button>(context);
    settings_button->setPosition(sf::Vector2f(100, 300));
    settings_button->SetText("Settings");
    settings_button->SetCallback([this]()
        {
            RequestStackPush(StateID::kSettings);
        });

    auto exit_button = std::make_shared<gui::Button>(context);
    exit_button->setPosition(sf::Vector2f(100, 450));
    exit_button->SetText("Exit");
    exit_button->SetCallback([this]()
        {
            RequestStackPop();
        });

    m_gui_container.Pack(join_play_button);
    m_gui_container.Pack(settings_button);
    m_gui_container.Pack(exit_button);

    context.music->Play(MusicThemes::kMenuTheme);
}

void MenuState::Draw()
{
    sf::RenderWindow& window = *GetContext().window;
    window.setView(window.getDefaultView());
    window.draw(m_background_sprite);
    window.draw(m_gui_container);
    window.draw(m_name_input_text);
    window.draw(m_name_input_box);
    window.draw(m_name_input_text);
}

bool MenuState::Update(sf::Time dt)
{
    return true;
}

/// <summary>
/// Add Name Field Handling
/// Modified: Kaylon's Claude
/// </summary>
bool MenuState::HandleEvent(const sf::Event& event)
{
    // (Kaylon's Claude) Check for click on name field
    const auto* mouse_pressed = event.getIf<sf::Event::MouseButtonPressed>();
    if (mouse_pressed)
    {
        sf::Vector2f mouse(static_cast<float>(mouse_pressed->position.x),
            static_cast<float>(mouse_pressed->position.y));
        if (m_name_input_box.getGlobalBounds().contains(mouse))
        {
            m_name_field_active = true;
            m_name_input_box.setOutlineColor(sf::Color::Yellow); // highlight when active
        }
        else
        {
            m_name_field_active = false;
            m_name_input_box.setOutlineColor(sf::Color::White);
        }
    }

    // (Kaylon's Claude) Only handle text input when field is active
    const auto* text_entered = event.getIf<sf::Event::TextEntered>();
    if (text_entered && m_name_field_active)
    {
        if (text_entered->unicode == '\b')
        {
            if (!m_name_input.empty())
                m_name_input.pop_back();
        }
        else if (text_entered->unicode == '\r' || text_entered->unicode == 27) // Enter or Escape
        {
            m_name_field_active = false;
            m_name_input_box.setOutlineColor(sf::Color::White);
        }
        else if (m_name_input.size() < 5 && text_entered->unicode >= 32 && text_entered->unicode < 128)
        {
            m_name_input += static_cast<char>(text_entered->unicode);
        }

        m_name_input_text.setString("Name: " + m_name_input);
        GetContext().player_details->m_name = m_name_input;
        SaveDetails(m_name_input, LoadHighScore());
    }

    // (Kaylon's Claude) Only forward events to GUI when name field is not active
    if (!m_name_field_active)
        m_gui_container.HandleEvent(event);

    return true;
}