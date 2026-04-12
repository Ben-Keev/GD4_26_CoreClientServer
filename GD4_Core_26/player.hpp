#pragma once
#include "action.hpp"
#include "command.hpp"
#include "command_queue.hpp"
#include "key_binding.hpp"

#include <SFML/Window/Event.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Network/TcpSocket.hpp>
#include <map>

struct PlayerDetails;

/// <summary>
/// Modified to take window as a parameter. This allows for mouse aiming to be implemented.
/// Class modified: Ben with assistance of Claude, Kaylon with assistance of claude
/// </summary>
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

	void DisableAllRealtimeActions(bool enable);
	bool IsLocal() const;

	void PushCombinedMoveCommand(CommandQueue& commands, sf::Vector2f velocity);

private:
	void InitialiseActions();

private:
	const KeyBinding* m_key_binding;
	std::map<Action, Command> m_action_binding;
	std::map<Action, bool> m_action_proxies;
	uint8_t m_identifier;
	sf::TcpSocket* m_socket;
	sf::RenderWindow* m_window;
};

