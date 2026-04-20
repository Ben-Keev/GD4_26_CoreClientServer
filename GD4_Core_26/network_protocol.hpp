#pragma once
#include <SFML/System/Vector2.hpp>
#include <SFML/Network/Packet.hpp>

const unsigned short SERVER_PORT = 50000; //Greater than 49151, in dynamic port range

/// <summary>
/// Modified: Ben & Kaylon
/// </summary>
namespace Server
{
	enum class PacketType
	{
		kBroadcastMessage, // Unmodified
		kInitialState, // (Ben) Once the game has started and spawnself is called tell client to spawn remote tanks
		kPlayerEvent, // Unmodified
		kPlayerRealtimeChange, // Unmodified
		kPlayerConnect, // Player has connected
		kPlayerDisconnect, // Player has left
		kSpawnSelf, // Tells client to spawn their local tank
		kUpdateClientState, // (Ben's Claude) Struct sent to update clients of tank/turret positions
		kGameStart, // (Ben) Signal clients to boot into multiplayer_game_state
		kLobbyCountdownReset, // (Ben) Updates clients with total connected players and resets countdown. Triggered on connect/disconnect
		kLobbyPing, // (Ben) Heartbeat that communicates countdown to clients
		kPlayerList, // (Ben & Kaylon) List of all player's usernames to display on lobby
		kReturnToLobby, // (Ben) Tell clients to return to lobby
		kWallDestroyed, // (Ben) Notifies clients that a wall has been destroyed with X, Y and id to destory wall and projectile.
		kHealthUpdate, // (Kaylon's Claude) Server broadcasts authoritative health after a hit
		kResetSkipVote, // (Kaylon's Claude) Tells clients to reset their skip vote state after post game delay
		kAwardBonusPoints // (Kaylon's Claude) Awards bonus points to the last surviving player
	};
}

/// <summary>
/// Modified: Ben & Kaylon
/// </summary>
namespace Client
{
	enum class PacketType
	{
		kPlayerEvent, // Unmodified
		kPlayerRealtimeChange, // Unmodified
		kStateUpdate, // (Ben) Uses struct to update clients on movements of tanks
		kGameEvent, // (Ben) No longer for explosions. Send X and Y position with an ID. Used for wall destruction
		kQuit,
		kPlayerDetails, // (Kaylon) Sent to share player's names and scores across the network
		kHeartbeat, // (Ben) Sent to prevent timeout
		kVoteSkipCountdown // (Ben) Bool indicates whether a player wants to skip
	};
}

/// <summary>
/// Modified: Ben with the assistance of Claude
/// </summary>
namespace GameActions
{
	enum Type
	{
		kEnemyExplode, // Unmodified
		kWallDestroyed, // (Ben's Claude) send wall destructions as an event to the server to give authority.
		kProjectileHit // (Kaylon's Claude) A bullet hit a tank
	};

	struct Action
	{
		Action() = default;
		Action(Type type, sf::Vector2f position)
			: type(type), position(position) {}
		Action(Type type, sf::Vector2f position, uint16_t identifier)
			: type(type), position(position), identifier(identifier) {}
		Action(Type type, sf::Vector2f position, uint16_t identifier, uint8_t victim, uint8_t dmg)
			: type(type), position(position), identifier(identifier), victim_id(victim), damage(dmg) {}


		Type type;
		sf::Vector2f position;
		uint16_t identifier;	// (Ben) An identifier which can carry extra information if needed.
		uint8_t victim_id = 0;	// (Kaylon's Claude) ID of the tank that was hit
		uint8_t damage = 0;		// (Kaylon Claude) Damage to apply
	};
}

/// <summary>
/// Modified: Kaylon
/// Changed the position vatialbes to int 16 and rotation variables to int 8, together saving 8 bytes per packet
/// </summary>
namespace PacketStructs
{
	// (Ben's Claude) A struct keeps state updates consistent and reduces scope for error
	struct AircraftStatePacket
	{
		uint8_t identifier;
		uint16_t x, y;
		uint8_t hitpoints;
		uint8_t turret_rotation;
		uint8_t hull_rotation;

		void Write(sf::Packet& p) const { p << identifier << x << y << hitpoints << turret_rotation << hull_rotation; }
		void Read(sf::Packet& p) { p >> identifier >> x >> y >> hitpoints >> turret_rotation >> hull_rotation; }
	};
}

