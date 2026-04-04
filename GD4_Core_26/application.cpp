#include "application.hpp"
#include "constants.hpp"
#include "fontid.hpp"
#include "game_state.hpp"
#include "title_state.hpp"
#include "menu_state.hpp"
#include "pause_state.hpp"
#include "settings_state.hpp"
#include "game_over_state.hpp"
#include "SocketWrapperPCH.hpp"
#include "state.hpp"

bool Application::m_joystick = sf::Joystick::isConnected(0);

void Application::isJoystickConnected()
{
	Application::m_joystick = sf::Joystick::isConnected(0);
	std::cout << "Joystick connected: " << Application::m_joystick << std::endl;
}

/// <summary>
/// Modified: Ben Mc Keever D00254413
/// Now takes two players in constructor
/// Modified: Kaylon Riordan D00255039
/// Adjusted window size to fit new assets
/// </summary>
Application::Application()
	: m_window(sf::VideoMode({ 1024, 576 }), "States", sf::Style::Close)
	, m_key_binding(1)
	, m_stack(State::Context(m_window, m_textures, m_fonts, m_music, m_sound, m_key_binding))
{
	m_window.setKeyRepeatEnabled(false);
	m_fonts.Load(FontID::kMain, "Media/Fonts/Sansation.ttf");
	m_textures.Load(TextureID::kTitleScreen, "Media/Textures/TitleScreen.png");
	m_textures.Load(TextureID::kButtons, "Media/Textures/Buttons.png");

	RegisterStates();
	m_stack.PushState(StateID::kTitle);
}

void Application::Run()
{
	sf::Clock clock;
	sf::Time time_since_last_update = sf::Time::Zero;
	while (m_window.isOpen())
	{
		time_since_last_update += clock.restart();
		while (time_since_last_update.asSeconds() > kTimePerFrame)
		{
			time_since_last_update -= sf::seconds(kTimePerFrame);
			ProcessInput();
			Update(sf::seconds(kTimePerFrame));

			if (m_stack.IsEmpty())
			{
				m_window.close();
			}
		}
		Render();
	}
}

void Application::ProcessInput()
{
	while (const std::optional event = m_window.pollEvent())
	{
		m_stack.HandleEvent(*event);

		if (event->is<sf::Event::Closed>())
		{
			m_window.close();
		}

	}
}

void Application::Update(sf::Time dt)
{
	m_stack.Update(dt);
}

void Application::Render()
{
	m_window.clear();
	m_stack.Draw();
	m_window.display();
}

void Application::RegisterStates()
{
	m_stack.RegisterState<TitleState>(StateID::kTitle);
	m_stack.RegisterState<MenuState>(StateID::kMenu);
	m_stack.RegisterState<GameState>(StateID::kGame);
	m_stack.RegisterState<PauseState>(StateID::kPause);
	m_stack.RegisterState<SettingsState>(StateID::kSettings);
	m_stack.RegisterState<GameOverState>(StateID::kGameOver);
}


