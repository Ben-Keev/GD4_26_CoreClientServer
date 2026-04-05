#pragma once
#include "command_queue.hpp"
#include <SFML/Window/Event.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include "action.hpp"
#include <map>
#include "command.hpp"
#include "mission_status.hpp"
#include "key_binding.hpp"
#include <SFML/Network/TcpSocket.hpp>


struct PlayerDetails
{
	std::string m_name;
	sf::Color m_colour;
	int m_score;
};

class Player
{
public:
	Player(sf::TcpSocket* socket, uint8_t identifier, const KeyBinding* binding, sf::RenderWindow* window);
	Command AnalogueAiming(const sf::Vector2f& mousePos);
	void HandleEvent(const sf::Event& event, CommandQueue& command_queue);
	void HandleRealTimeInput(CommandQueue& command_queue);
	void HandleRealtimeNetworkInput(CommandQueue& commands);

	//React to events or realtime state changes recevied over the network
	void HandleNetworkEvent(Action action, CommandQueue& commands);
	void HandleNetworkRealtimeChange(Action action, bool action_enabled);

	void SetMissionStatus(MissionStatus status);
	MissionStatus GetMissionStatus() const;

	void DisableAllRealtimeActions(bool enable);
	bool IsLocal() const;

	PlayerDetails GetDetails();

private:
	void InitialiseActions();

private:
	PlayerDetails m_details;
	const KeyBinding* m_key_binding;
	std::map<Action, Command> m_action_binding;
	std::map<Action, bool> m_action_proxies;
	MissionStatus m_current_mission_status;
	uint8_t m_identifier;
	sf::TcpSocket* m_socket;
	sf::RenderWindow* m_window;
};

