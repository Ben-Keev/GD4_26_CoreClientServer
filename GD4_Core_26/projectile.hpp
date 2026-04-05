#pragma once
#include "entity.hpp"
#include "resource_identifiers.hpp"
#include "projectile_type.hpp"
#include "tank.hpp"

/// <summary>
/// Modified: Kaylon Riordan D00255039
/// Added Methods and variables to keep track of wether a bullet can bounce, and how many times it has bounced
/// </summary>

class Projectile : public Entity
{
public:
	Projectile(ProjectileType type, const TextureHolder& textures, Tank* owner);

	virtual unsigned int GetCategory() const override;
	virtual sf::FloatRect GetBoundingRect() const override;

	float GetMaxSpeed() const;
	float GetDamage() const;

	Tank& GetOwner() const;

	int GetBounces() const;
	bool CanBounce() const;
	void AddBounce();

private:
	virtual void UpdateCurrent(sf::Time dt, CommandQueue& commands) override;
	virtual void DrawCurrent(sf::RenderTarget& target, sf::RenderStates states) const override;

private:
	ProjectileType m_type;
	sf::Sprite m_sprite;
	sf::Vector2f m_target_direction;
	Tank* m_owner;
	int m_bounces = 0;
	bool m_has_bounced;
};

