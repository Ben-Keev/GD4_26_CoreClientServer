#include "SocketWrapperPCH.hpp"
#include "Entity.hpp"
#include "command_queue.hpp"

Entity::Entity(int hitpoints) :m_hitpoints(hitpoints)
{
	m_previous_position = getPosition();
	m_previous_rotation = getRotation();
}


void Entity::SetVelocity(sf::Vector2f velocity)
{
	m_velocity = velocity;
}

void Entity::SetVelocity(float vx, float vy)
{
	m_velocity.x = vx;
	m_velocity.y = vy;
}

sf::Vector2f Entity::GetVelocity() const
{
	return m_velocity;
}

void Entity::Accelerate(sf::Vector2f velocity)
{
	m_velocity += velocity;
}

void Entity::Accelerate(float vx, float vy)
{
	m_velocity.x += vx;
	m_velocity.y += vy;
}

int Entity::GetHitPoints() const
{
	return m_hitpoints;
}

void Entity::SetHitpoints(int points)
{
	//assert(points > 0);
	m_hitpoints = points;
}

void Entity::Repair(int points)
{
	assert(points >= 0);
	m_hitpoints += points;
}

void Entity::Damage(int points)
{
	assert(points > 0);
	m_hitpoints -= points;
}

void Entity::Destroy()
{
	m_hitpoints = 0;
}

bool Entity::IsDestroyed() const
{
	return m_hitpoints <= 0;
}

/// <summary>
/// Modified: Kaylon Riordan D00255039
/// Update previous position variables to current position every frame
/// </summary>
void Entity::UpdateCurrent(sf::Time dt, CommandQueue& commands)
{
	// Store position and rotation as previous position and rotation before updating, used in collision logic
	m_previous_position = getPosition();
	m_previous_rotation = getRotation();
	move(m_velocity * dt.asSeconds());
}

void Entity::Remove()
{
	Destroy();
}

/// <summary>
/// Authored: Kaylon Riordan D00255039
/// Created methods that track the players position and rotation on the previous tick, Data is used to restore previous position after colliding into a wall
/// </summary>
sf::Vector2f Entity::GetPreviousPosition() const
{
	return m_previous_position;
}

sf::Angle Entity::GetPreviousRotation() const
{
	return m_previous_rotation;
}

