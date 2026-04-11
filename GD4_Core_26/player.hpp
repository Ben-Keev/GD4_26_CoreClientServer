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

struct PlayerDetails;

class Player
{
public:
	Player(sf::TcpSocket* socket, uint8_t identifier, const KeyBinding* binding, sf::RenderWindow* window);
	Command AnalogueAiming(const sf::Vector2f& mousePos);
	void HandleEvent(const sf::Event& event, CommandQueue& command_queue);
	void HandleRealTimeInput(CommandQueue& command_queue, const sf::View& world_view);
	void HandleRealtimeNetworkInput(CommandQueue& commands);

	//React to events or realtime state changes recevied over the network
	void HandleNetworkEvent(Action action, CommandQueue& commands);
	void HandleNetworkRealtimeChange(Action action, bool action_enabled);

	void SetMissionStatus(MissionStatus status);
	MissionStatus GetMissionStatus() const;

	void DisableAllRealtimeActions(bool enable);
	void SetNetworkVelocity(sf::Vector2f velocity);
	bool IsLocal() const;

	void PushCombinedMoveCommand(CommandQueue& commands, sf::Vector2f velocity);
	sf::Vector2f GetCombinedNetworkVelocity() const;

private:
	void InitialiseActions();

private:
	const KeyBinding* m_key_binding;
	std::map<Action, Command> m_action_binding;
	std::map<Action, bool> m_action_proxies;
	MissionStatus m_current_mission_status;
	uint8_t m_identifier;
	sf::TcpSocket* m_socket;
	sf::RenderWindow* m_window;
	sf::Vector2f m_current_network_velocity;
};

