#pragma once
#include "scene_node.hpp"
#include "network_protocol.hpp"

#include <queue>
class NetworkNode : public SceneNode
{
public:
	/// <summary>
	/// Modified: Kaylon with Claude, add code to send packet when hiting other tanks
	/// </summary>
	NetworkNode();
	void NotifyGameAction(GameActions::Type type, sf::Vector2f position, uint16_t identifier);
	void NotifyGameAction(GameActions::Type type, sf::Vector2f position);
	void NotifyGameAction(GameActions::Type type, sf::Vector2f position, uint16_t identifier, uint8_t victim_id, uint8_t damage);
	bool PollGameAction(GameActions::Action& out);
	virtual unsigned int GetCategory() const override;

private:
	std::queue<GameActions::Action> m_pending_actions;
};

