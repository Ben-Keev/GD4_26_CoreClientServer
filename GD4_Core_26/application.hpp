#pragma once
#include <SFML/System/Clock.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include "player.hpp"
#include "resource_holder.hpp"
#include "resource_identifiers.hpp"
#include "statestack.hpp"
#include "music_player.hpp"
#include "sound_player.hpp"

class Application
{
public:
	Application();
	void Run();
	static void isJoystickConnected();
	static bool m_joystick;

private:
	void ProcessInput();
	void Update(sf::Time dt);
	void Render();
	void RegisterStates();

private:
	sf::RenderWindow m_window;
	Player m_local_player;

	TextureHolder m_textures;
	FontHolder m_fonts;

	MusicPlayer m_music;
	SoundPlayer m_sound;

	StateStack m_stack;

	KeyBinding m_key_binding;
};

