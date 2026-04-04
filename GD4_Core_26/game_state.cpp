#include "SocketWrapperPCH.hpp"
#include "game_state.hpp"
#include "mission_status.hpp"
#include <iostream>

/// <summary>
/// Modified: Ben Mc Keever D00254413
/// </summary>
/// <param name="stack"></param>
/// <param name="context"></param>
GameState::GameState(StateStack& stack, Context context) : State(stack, context), m_world(*context.window, *context.fonts, *context.sound), m_red_player(*context.local_player)
{
	context.music->Play(MusicThemes::kMissionTheme);
}

void GameState::Draw()
{
	m_world.Draw();
}

/// <summary>
/// Modified: Ben Mc Keever D00254413
/// </summary>
/// <param name="dt"></param>
/// <returns></returns>
bool GameState::Update(sf::Time dt)
{
	m_world.Update(dt);

	if (!m_world.AllPlayersAlive())
	{
		if (m_world.HasPlayerDied(0) && m_world.HasPlayerDied(1)) 
		{
			m_red_player.SetMissionStatus(MissionStatus::kMissionFailure);
			//m_blue_player.SetMissionStatus(MissionStatus::kMissionFailure);
		}
		else if (m_world.HasPlayerDied(0))
		{
			m_red_player.SetMissionStatus(MissionStatus::kMissionFailure);
			//m_blue_player.SetMissionStatus(MissionStatus::kMissionSuccess);
		}
		else
		{
			m_red_player.SetMissionStatus(MissionStatus::kMissionSuccess);
			//m_blue_player.SetMissionStatus(MissionStatus::kMissionFailure);
		}
		RequestStackPush(StateID::kGameOver);
	}

	CommandQueue& commands = m_world.GetCommandQueue();
	m_red_player.HandleRealTimeInput(commands);
	//m_blue_player.HandleRealTimeInput(commands);

	return true;
}

/// <summary>
/// Modified: Ben Mc Keever D00254413
/// </summary>
/// <param name="event"></param>
/// <returns></returns>
bool GameState::HandleEvent(const sf::Event& event)
{
	CommandQueue& commands = m_world.GetCommandQueue();
	m_red_player.HandleEvent(event, commands);
	//m_blue_player.HandleEvent(event, commands);

	//Escape should bring up the pause menu
	const auto* joy_pressed = event.getIf<sf::Event::JoystickButtonPressed>();
	if(joy_pressed && joy_pressed->button == static_cast<int>(XboxLayout::Start))
	{
		RequestStackPush(StateID::kPause);
	}
	return true;
}