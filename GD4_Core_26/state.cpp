#include "state.hpp"
#include "statestack.hpp"

State::State(StateStack& stack, Context context) : m_stack(&stack), m_context(context)
{
}

State::~State()
{
}

/// <summary>
/// Add socket, Player details, and keys
/// Modified: Ben
/// </summary>
/// <param name="keys">One player, one keybinding</param>
/// <param name="socket">Connection is maintained between states</param>
State::Context::Context(sf::RenderWindow& window, TextureHolder& textures, FontHolder& fonts, MusicPlayer& music, SoundPlayer& sound, KeyBinding& keys, sf::TcpSocket& socket, PlayerDetails& player_details) : window(&window), textures(&textures), fonts(&fonts), music(&music), sound(&sound), keys(&keys), socket(&socket), player_details(&player_details)
{
}

void State::RequestStackPush(StateID state_id)
{
	m_stack->PushState(state_id);
}

void State::RequestStackPop()
{
	m_stack->PopState();
}

void State::RequestStackClear()
{
	m_stack->ClearStack();
}

State::Context State::GetContext() const
{
	return m_context;
}

void State::OnActivate()
{

}

void State::OnDestroy()
{

}