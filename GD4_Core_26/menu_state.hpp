#pragma once
#include "state.hpp"
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Text.hpp>
#include "container.hpp"

class MenuState : public State
{
public:
	MenuState(StateStack& stack, Context context);
	virtual void Draw() override;
	virtual bool Update(sf::Time dt) override;
	virtual bool HandleEvent(const sf::Event& event) override;

private:
	sf::Sprite m_background_sprite;
	gui::Container m_gui_container;
	sf::Text m_name_input_text;
	std::string m_name_input;
	bool m_name_field_active = false;
	sf::RectangleShape m_name_input_box;
};

