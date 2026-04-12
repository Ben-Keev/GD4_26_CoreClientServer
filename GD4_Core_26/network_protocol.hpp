#pragma once
#include <SFML/System/Vector2.hpp>
#include <SFML/Network/Packet.hpp>
const unsigned short SERVER_PORT = 50000; //Greater than 49151, in dynamic port range
namespace Server
{
	enum class PacketType
	{
		kBroadcastMessage, //Takes a std::string and sends it to all clients, they show on their screens for a number of seconds
		kInitialState, //takes two float values, the world height, and the initial scrolling in the world, then sf::Int32 with the number of aircraft, then for each aircraft its identifier, position, health and missiles
		kPlayerEvent, //This takes two sf::Int32 variables, the aircraft identifier and the action identifier from action.hpp, this is used to tell that a particular plane has triggered some action
		kPlayerRealtimeChange, //Same as playerevent for real time actions
		kPlayerConnect, //The same as SpawnSelf but indicates that an aircraft from a different client is connecting
		kPlayerDisconnect, //Takes sf::Int32 aircraft identifier that is disconnecting
		kSpawnSelf, //This takes an sf::Int32 for the aircraft identifier and two float values for the initial position. 
		kUpdateClientState, //This takes one float with the current scrolling of the world in the server, and then a sf::Int32 for the number of aircraft. For each aircraft, it packs one sf::Int32 value with the identifier, two floats for position, health, and ammo. Think about enemies. If we don't send anything they will be locally tracked
		kGameStart, // This has no arguments. It informs the client that the game is starting
		kLobbyCountdownReset,
		kLobbyPing, // This is a keep-alive packet that also carries info on the lobby countdown. It is sent by the server every 500ms to prevent timeout in the lobby, and contains a float with the current countdown time and a sf::Int32 with the number of connected players.
		kPlayerList, // This is sent by the server to update the client on the current list of players in the lobby. It contains a sf::Int32 with the number of players, and for each player a sf::Int32 with their identifier and a std::string with their name.
		kReturnToLobby, // Tell Clients connected ingame to return to lobby
		kWallDestroyed // Claude suggested - send wall destructions as an event to the clients to give authority.
	};
}

namespace Client
{
	enum class PacketType
	{
		kPlayerEvent, // Two sf::Int32, aircraft identifer and event. It is used to request the server to trigger an event on the aircraft
		kPlayerRealtimeChange, // The same kPlayerEvent, additionally takes a boolean for real time action
		kStateUpdate, //sf::Int32 with number of local aircraft, for each aircraft send sf::Int32 identifier, two floats for position, health and ammo 
		kGameEvent, //This is for explosions
		kQuit,
		kPlayerDetails,
		kHeartbeat, // This is a keep-alive packet sent periodically by the client to prevent server timeout in the lobby. No parameters
		kVoteSkipCountdown // This is sent by the client when the user presses the "Skip Countdown" button in the lobby. The parameter is a boolean indicating whether the user wants to skip the countdown or not.
	};
}

namespace GameActions
{
	enum Type
	{
		kEnemyExplode,
		kWallDestroyed // Claude suggested - send wall destructions as an event to the server to give authority.
	};

	struct Action
	{
		Action() = default;
		Action(Type type, sf::Vector2f position) :type(type), position(position)
		{

		}

		Action(Type type, sf::Vector2f position, uint16_t identifier) :type(type), position(position), identifier(identifier)
		{
		}

		Type type;
		sf::Vector2f position;
		uint16_t identifier; // An identifier which can carry extra information if needed.
	};
}

namespace PacketStructs
{
	// Claude suggested - a struct to hold the state of an aircraft for packing into the kUpdateClientState packet. Reduce duplicate code.
	struct AircraftStatePacket
	{
		uint8_t identifier;
		float x, y;
		uint8_t hitpoints;
		float turret_rotation;
		float hull_rotation;

		void Write(sf::Packet& p) const { p << identifier << x << y << hitpoints << turret_rotation << hull_rotation; }
		void Read(sf::Packet& p) { p >> identifier >> x >> y >> hitpoints >> turret_rotation >> hull_rotation; }
	};
}

