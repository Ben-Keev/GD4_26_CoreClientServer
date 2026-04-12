#pragma once
#include <memory>
#include "resource_identifiers.hpp"
#include "stateid.hpp"
#include "music_player.hpp"
#include "sound_player.hpp"

#include <string>
#include <SFML/Graphics/Color.hpp>
#include <SFML/Network/TcpSocket.hpp>
#include <SFML/Graphics/RenderWindow.hpp>

/// <summary>
/// Authored by Kaylon. Modified by Ben to be passed into context.
/// Authored: Kaylon | Modified: Ben
/// </summary>
struct PlayerDetails
{
	std::string m_name;
	sf::Color m_colour;
	int m_score;
};

class StateStack;
class KeyBinding;

class State
{
public:
	typedef std::unique_ptr<State> Ptr;

	/// <summary>
	/// Remove two sets of keybindings. Add Socket so connection can be accessed between states. Add player details so this is also accessible from every state
	/// Modified: Ben
	/// </summary>
	struct Context
	{
		Context(sf::RenderWindow& window, TextureHolder& textures, FontHolder& fonts, MusicPlayer& music, SoundPlayer& sound, KeyBinding& keys, sf::TcpSocket& socket, PlayerDetails& player_details);
		sf::RenderWindow* window;
		TextureHolder* textures;
		FontHolder* fonts;
		MusicPlayer* music;
		SoundPlayer* sound;
		KeyBinding* keys;
		sf::TcpSocket* socket;
		PlayerDetails* player_details;
	};

public:
	State(StateStack& stack, Context context);
	virtual ~State();
	virtual void Draw() = 0;
	virtual bool Update(sf::Time dt) = 0;
	virtual bool HandleEvent(const sf::Event& event) = 0;
	virtual void OnActivate();
	virtual void OnDestroy();

protected:
	void RequestStackPush(StateID state_id);
	void RequestStackPop();
	void RequestStackClear();

	Context GetContext() const;
private:
	StateStack* m_stack;
	Context m_context;
};