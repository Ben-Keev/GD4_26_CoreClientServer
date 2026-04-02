#include "SocketWrapperPCH.hpp"
#include "game_over_state.hpp"
#include "utility.hpp"
#include "constants.hpp"
#include <iostream>

/// <summary>
/// Modified: Ben Mc Keever D00254413
/// </summary>
/// <param name="stack"></param>
/// <param name="context"></param>
GameOverState::GameOverState(StateStack& stack, Context context)
    : State(stack, context)
    , m_game_over_text(context.fonts->Get(FontID::kMain))
    , m_elapsed_time(sf::Time::Zero)
{
    sf::Vector2f window_size(context.window->getSize());

    context.music->Stop();
	context.sound->Play(SoundEffect::kGameEnd);

    if (context.red_player->GetMissionStatus() == MissionStatus::kMissionFailure && context.blue_player->GetMissionStatus() == MissionStatus::kMissionFailure) 
    {
        m_game_over_text.setString("It's a tie!");
    }
    else if (context.red_player->GetMissionStatus() == MissionStatus::kMissionSuccess)
    {
        m_game_over_text.setString("Red Tank Won!");
    }
    else if (context.blue_player->GetMissionStatus() == MissionStatus::kMissionSuccess)
    {
        m_game_over_text.setString("Blue Tank Won!");
    }
    
    m_game_over_text.setCharacterSize(70);
    Utility::CentreOrigin(m_game_over_text);
    m_game_over_text.setPosition(sf::Vector2f(0.5 * window_size.x, 0.4 * window_size.y));
}

void GameOverState::Draw()
{
    sf::RenderWindow& window = *GetContext().window;
    window.setView(window.getDefaultView());

    //Create a dark semi-transparent background
    sf::RectangleShape background_shape;
    background_shape.setFillColor(sf::Color(0, 0, 0, 150));
    background_shape.setSize(window.getView().getSize());

    window.draw(background_shape);
    window.draw(m_game_over_text);
}

bool GameOverState::Update(sf::Time dt)
{
    //Show gameover for 3 seconds and then return to the main menu
    m_elapsed_time += dt;
    if (m_elapsed_time > sf::seconds(kGameOverToMenuPause))
    {
        RequestStackClear();
        RequestStackPush(StateID::kMenu);
    }
    return false;
}

bool GameOverState::HandleEvent(const sf::Event& event)
{
    return false;
}
