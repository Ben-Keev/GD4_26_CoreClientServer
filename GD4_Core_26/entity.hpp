#pragma once
#include "scene_node.hpp"

class Entity : public SceneNode
{
public:
	Entity(int hitpoints);
	void SetVelocity(sf::Vector2f velocity);
	void SetVelocity(float vx, float vy);
	sf::Vector2f GetVelocity() const;
	void Accelerate(sf::Vector2f velocity);
	void Accelerate(float vx, float vy);
	sf::Vector2f GetPreviousPosition() const;
	sf::Angle GetPreviousRotation() const;

	int GetHitPoints() const;
	void Repair(int points);
	void Damage(int points);
	void Destroy();
	virtual bool IsDestroyed() const override;

protected:
	virtual void UpdateCurrent(sf::Time dt, CommandQueue& commands) override;

private:
	sf::Vector2f m_velocity;
	sf::Vector2f m_previous_position;
	sf::Angle m_previous_rotation;
	int m_hitpoints;
};