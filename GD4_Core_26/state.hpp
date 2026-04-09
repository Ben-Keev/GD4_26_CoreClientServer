#pragma once
#include <memory>
#include "resource_identifiers.hpp"
#include "player.hpp"
#include <SFML/Graphics/RenderWindow.hpp>
#include "stateid.hpp"
#include "music_player.hpp"
#include "sound_player.hpp"


class StateStack;
class KeyBinding;

class State
{
public:
	typedef std::unique_ptr<State> Ptr;

	struct Context
	{
		Context(sf::RenderWindow& window, TextureHolder& textures, FontHolder& fonts, MusicPlayer& music, SoundPlayer& sound, KeyBinding& keys1, KeyBinding& keys2, sf::TcpSocket& socket);
		//TODO unique_ptr rather than raw pointers here?
		sf::RenderWindow* window;
		TextureHolder* textures;
		FontHolder* fonts;
		MusicPlayer* music;
		SoundPlayer* sound;
		KeyBinding* keys1;
		KeyBinding* keys2;
		sf::TcpSocket* socket;
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

	// Return non-const reference so callers can modify the shared Context. (Github Copilot)
	// Also provide a const overload for const-qualified usage.
	//Context& GetContext() { return m_context; }
	//const Context& GetContext() const { return m_context; }
	Context GetContext() const;
private:
	StateStack* m_stack;
	Context m_context;
};

