#pragma once
#include "command_queue.hpp"
#include <SFML/Window/Event.hpp>
#include "action.hpp"
#include <map>
#include "command.hpp"
#include "mission_status.hpp"
#include "xbox_layout.hpp"

/// <summary>
/// Modified: Ben Mc Keever D00254413
/// 
/// Modified: Kaylon Riordan D00255039
/// Added reference to turret category so turret can be rotated
/// </summary>

class Player
{
public:
	sf::Angle CalculateRotation(float x, float y);
	Player(int joystick_number);
	void HandleEvent(const sf::Event& event, CommandQueue& command_queue);
	void HandleRealTimeInput(CommandQueue& command_queue);

	Command AnalogueMovement(float x, float y);
	Command AnalogueAiming(float u, float v);

	void AssignKey(Action action, XboxLayout);
	int GetAssignedKey(Action action) const;
	void SetMissionStatus(MissionStatus status);
	MissionStatus GetMissionStatus() const;

	~Player();

private:
	void InitialiseActions();
	static bool IsRealTimeAction(Action action);


private:
	std::map<XboxLayout, Action> m_joystick_binding;
	std::map<Action, Command> m_action_binding;
	MissionStatus m_current_mission_status;
	int joystick_number;
	ReceiverCategories tankCategory;
	ReceiverCategories turretCategory;
};
