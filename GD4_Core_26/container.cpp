#include "SocketWrapperPCH.hpp"
#include "container.hpp"
#include "xbox_layout.hpp"
#include "application.hpp"

/// <summary>
/// Modified: Ben Mc Keever D00254413
/// </summary>

gui::Container::Container() : m_selected_child(-1)
{
}

void gui::Container::Pack(Component::Ptr component)
{
    m_children.emplace_back(component);
    if (!HasSelection() && component->IsSelectable())
    {
        Select(m_children.size() - 1);
    }
}

bool gui::Container::IsSelectable() const
{
    return false;
}

void gui::Container::HandleRealtimeInput() 
{

}

/// <summary>
/// Author: Ben Mc Keever D00254413
/// </summary>
/// <param name="joystick">Joystick Number</param>
void gui::Container::HandleJoystickInput(int joystick)
{
    bool upHeld = false;
    bool downHeld = false;

    // Returns within range of -100 and 100
    float dpadY = sf::Joystick::getAxisPosition(joystick, sf::Joystick::Axis::PovY);

    // Due to SFML input logic the inputs are flipped for dpad/thumbstick axes
    bool upNow = dpadY > 50.f;
    bool downNow = dpadY < -50.f;

    if (upNow && !upHeld)
        SelectPrevious();

    if (downNow && !downHeld)
        SelectNext();

    upHeld = upNow;
    downHeld = downNow;
}

/// <summary>
/// Modified: Ben Mc Keever D00254413
/// </summary>
/// <param name="event"></param>
void gui::Container::HandleEvent(const sf::Event& event)
{
    const auto* joy_released = event.getIf<sf::Event::JoystickButtonReleased>();
    if (HasSelection() && m_children[m_selected_child]->IsActive())
    {
        m_children[m_selected_child]->HandleEvent(event);
    }
    else if (joy_released)
    {
        if (joy_released->button == static_cast<int>(XboxLayout::A))
        {
            if (HasSelection())
            {
                m_children[m_selected_child]->Activate();
            }
        }
    }

    const auto* joy_moved = event.getIf<sf::Event::JoystickMoved>();
    if (joy_moved)
    {
        if (joy_moved->axis == sf::Joystick::Axis::PovY || joy_moved->axis == sf::Joystick::Axis::Y)
            HandleJoystickInput(joy_moved->joystickId);
    }

    // Kb + M
    const auto* key_released = event.getIf<sf::Event::KeyReleased>();
    if (HasSelection() && m_children[m_selected_child]->IsActive())
    {
        m_children[m_selected_child]->HandleEvent(event);
    }
    else if (key_released)
    {
        if (key_released->scancode == sf::Keyboard::Scancode::W || key_released->scancode == sf::Keyboard::Scancode::Up)
        {
            SelectPrevious();
        }
        else if (key_released->scancode == sf::Keyboard::Scancode::S || key_released->scancode == sf::Keyboard::Scancode::Down)
        {
            SelectNext();
        }
        else if (key_released->scancode == sf::Keyboard::Scancode::Enter || key_released->scancode == sf::Keyboard::Scancode::Space)
        {
            if (HasSelection())
            {
                m_children[m_selected_child]->Activate();
            }
        }
    }
}

void gui::Container::draw(sf::RenderTarget& target, sf::RenderStates states) const
{
    states.transform *= getTransform();
    for (const Component::Ptr& child : m_children)
    {
        target.draw(*child, states);
    }
}

bool gui::Container::HasSelection() const
{
    return m_selected_child >= 0;
}

void gui::Container::Select(std::size_t index)
{
    if (index < m_children.size() && m_children[index]->IsSelectable())
    {
        if (HasSelection())
        {
            m_children[m_selected_child]->Deselect();
        }
        m_children[index]->Select();
        m_selected_child = index;
    }
}

void gui::Container::SelectNext()
{
    if (!HasSelection())
    {
        return;
    }
    //Search for next selectable component
    int next = m_selected_child;
    do
    {
        next = (next + 1) % m_children.size();
    } while (!m_children[next]->IsSelectable());
    Select(next);
}

void gui::Container::SelectPrevious()
{
    if (!HasSelection())
    {
        return;
    }
    //Search for next selectable component
    int prev = m_selected_child;
    do
    {
        prev = (prev + m_children.size() - 1) % m_children.size();
    } while (!m_children[prev]->IsSelectable());
    Select(prev);
}

size_t gui::Container::GetContainerSize()
{
    size_t count = m_children.size();
    return count;
}
