#pragma once
#include "container.hpp"

#include <SFML/System/Vector2.hpp>
#include <SFML/Network/TcpSocket.hpp>
#include <SFML/Network/TcpListener.hpp>
#include <SFML/System/Clock.hpp>
#include <SFML/Graphics/Rect.hpp>
#include <thread>
#include <cstdint>
#include <map>

/// <summary>
/// Server
/// Modified: Ben Mc Keever, Kaylon Riordan, Assisted by Claude
/// </summary>
/// 
class GameServer
{
public:
	explicit GameServer(sf::Vector2f battlefield_size);
	~GameServer();
	void NotifyPlayerSpawn(uint8_t aircraft_identifier);
	void NotifyPlayerRealtimeChange(uint8_t aircraft_identifier, uint8_t action, bool action_enabled);
	void NotifyPlayerEvent(uint8_t aircraft_identifier, uint8_t action);

private:
	struct RemotePeer
	{
		RemotePeer();
		sf::TcpSocket m_socket;
		sf::Time m_last_packet_time;
		std::vector<uint8_t> m_aircraft_identifiers;
		bool m_ready;
		bool m_timed_out;
		std::string m_name;
		int m_score = 0;
		int m_high_score = 0;
	};

	// (Ben & Kaylon) Add extra parameters
	struct AircraftInfo
	{
		sf::Vector2f m_position;
		uint8_t m_hitpoints;
		float m_turret_rotation;
		float m_aircraft_rotation;
		std::map<uint8_t, bool> m_real_time_actions;
		std::string m_name;
		int m_score;
	};

	typedef std::unique_ptr<RemotePeer> PeerPtr;

private:
	void SetListening(bool enable);
	void ExecutionThread();
	void Tick();
	sf::Time Now() const;

	void HandleIncomingPackets();
	void HandleIncomingPackets(sf::Packet& packet, RemotePeer& receiving_peer, bool& detected_timeout);

	void HandleIncomingConnections();
	void SendPlayerList();
	void SendLobbyPacket(bool connected);
	void HandleDisconnections();

	void InformWorldState(sf::TcpSocket& socket);
	void BroadcastMessage(const std::string& message);
	void SendToAll(sf::Packet& packet);
	void UpdateClientState();

	void ResetGameState();

private:
	std::thread m_thread;
	sf::Clock m_clock;
	sf::TcpListener m_listener_socket;
	bool m_listening_state;
	sf::Time m_client_timeout;

	std::size_t m_max_connected_players;
	std::size_t m_connected_players;
	std::size_t m_total_skip_countdown;

	float m_world_height;
	sf::FloatRect m_battlefield_rect;

	std::map<uint8_t, AircraftInfo> m_aircraft_info;

	std::vector<PeerPtr> m_peers;
	uint8_t m_aircraft_identifier_counter;
	bool m_waiting_thread_end;

	sf::Time m_heartbeat_timer;

	bool m_lobby_active;
	sf::Time m_lobby_countdown;
	bool m_game_started;

	gui::Container m_gui_container;
};

