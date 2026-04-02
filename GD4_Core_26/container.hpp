#pragma once
#include "component.hpp"
namespace gui
{
	class Container : public Component
	{
	public:
		typedef std::shared_ptr<Container> Ptr;

	public:
		Container();
		void Pack(Component::Ptr component);
		virtual bool IsSelectable() const override;
		void HandleRealtimeInput();
		void HandleJoystickInput(int joystick);
		size_t GetContainerSize();
		virtual void HandleEvent(const sf::Event& event) override;

	private:
		virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const override;
		bool HasSelection() const;
		void Select(std::size_t index);
		void SelectNext();
		void SelectPrevious();



	private:
		std::vector<Component::Ptr> m_children;
		int m_selected_child;
	};
}

