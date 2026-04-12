#include "application.hpp"
#include "constants.hpp"
#include "fontid.hpp"
#include "title_state.hpp"
#include "menu_state.hpp"
#include "pause_state.hpp"
#include "settings_state.hpp"
#include "multiplayer_gamestate.hpp"
#include "lobby_state.hpp"

Application::Application()
	: m_window(sf::VideoMode({ 1792, 896 }), "States", sf::Style::Close)
	, m_key_binding(1)
	, m_key_binding_2(2)
	, m_player_details
		({ "Ben Benim"
		, sf::Color::Cyan
		, 0 })
	, m_stack(State::Context(m_window, m_textures, m_fonts, m_music, m_sound, m_key_binding, m_socket, m_player_details))
{
	m_window.setKeyRepeatEnabled(false);
	m_fonts.Load(FontID::kMain, "Media/Fonts/Supersonic Rocketship.ttf");
	m_textures.Load(TextureID::kTitleScreen, "Media/Textures/TitleScreen.png");
	m_textures.Load(TextureID::kLobbyScreen, "Media/Textures/LobbyScreen.png");
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

/// <summary>
/// Remove Local States
/// Register Lobby states
/// Modified: Ben
/// </summary>
void Application::RegisterStates()
{
	m_stack.RegisterState<TitleState>(StateID::kTitle);
	m_stack.RegisterState<MenuState>(StateID::kMenu);
	m_stack.RegisterState<LobbyState>(StateID::kJoinLobby, true);
	m_stack.RegisterState<LobbyState>(StateID::kRejoinLobby, false);
	m_stack.RegisterState<MultiplayerGameState>(StateID::kJoinGame);
	m_stack.RegisterState<PauseState>(StateID::kNetworkPause, true);
	m_stack.RegisterState<SettingsState>(StateID::kSettings);
}


