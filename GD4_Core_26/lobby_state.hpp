#pragma once
#include "State.hpp"
#include "world.hpp"

class LobbyState : public State
{
public:
	LobbyState(StateStack& stack, Context context, bool firstTime);
	virtual bool Update(sf::Time delta_time) override;
	virtual void Draw() override;
	void ToggleVoteSkipCountdown();
	virtual bool HandleEvent(const sf::Event& event) override;

private:
	// (Claude AI)
	struct PlayerEntry
	{
		uint8_t id;
		std::string name;
		int score;
		int high_score;
	};

	std::vector<PlayerEntry> m_ids_players;

	void HandlePacket(uint8_t packet_type, sf::Packet& packet);

	void UpdateCountdownText(uint8_t countdown);

	void UpdateBroadcastMessage(sf::Time elapsed_time);

	sf::RenderWindow& m_window;

	std::vector<std::string> m_broadcasts;
	sf::Text m_broadcast_text;
	sf::Time m_broadcast_elapsed_time;

	sf::Text m_lobby_countdown_text;
	sf::Text m_failed_connection_text;
	sf::Text m_players_connected_text;
	sf::Text m_players_list_text;

	sf::Clock m_failed_connection_clock;

	bool m_connected;
	std::size_t m_connected_players;

	sf::Time m_start_countdown;
	sf::Time m_heartbeat_timer;

	sf::Time m_client_timeout;
	sf::Time m_time_since_last_packet;

	// Does this client wants to skip the countdown??
	bool m_vote_skip_countdown;
};