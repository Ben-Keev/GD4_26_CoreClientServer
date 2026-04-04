#pragma once
#include "command_queue.hpp"
#include <SFML/Window/Event.hpp>
#include "action.hpp"
#include <map>
#include "command.hpp"
#include "mission_status.hpp"
#include "xbox_layout.hpp"
#include "MemoryStream.hpp"
#include "MemoryBitStream.hpp"
#include <vector>
#include <string>
#include "Math.hpp"
#include "key_binding.hpp"

class Player
{
	public:
		Player();
		uint32_t GetHealth() const;
		uint32_t GetAmmo() const;
		void Write(OutputMemoryStream& out_stream) const;
		void Read(InputMemoryStream& in_stream);
		void Write(OutputMemoryBitStream& out_stream) const;
		void ReadBits(InputMemoryBitStream& in_stream);
		void ToString() const;
	
		sf::Angle CalculateRotation(float x, float y);
		Player(int player_number);
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
		uint32_t health_;
		uint32_t ammo_;
		char name_[128];
		Vector3 position_;
		Quaternion rotation_;
		std::vector<int> weapons_;
		void InitialiseActions();
		static bool IsRealTimeAction(Action action);

	private:

		const KeyBinding* m_key_binding;

		std::map<XboxLayout, Action> m_joystick_binding;

		std::map<Action, Command> m_action_binding;
		MissionStatus m_current_mission_status;
		int player_number;

		uint8_t m_identifier;

		ReceiverCategories tankCategory;
		ReceiverCategories turretCategory;
};