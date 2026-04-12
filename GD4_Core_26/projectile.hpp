#pragma once
#include "entity.hpp"
#include "resource_identifiers.hpp"
#include "projectile_type.hpp"
#include <functional>

/// <summary>
/// Modified: Kaylon & Ben
/// Add an ID to index projectiles
/// </summary>

class Tank;

class Projectile : public Entity
{
public:
	Projectile(ProjectileType type, const TextureHolder& textures, sf::Color colour, Tank* owner, uint16_t identifier);

	virtual unsigned int GetCategory() const override;
	virtual sf::FloatRect GetBoundingRect() const override;

	float GetMaxSpeed() const;
	float GetDamage() const;

	Tank& GetOwner() const;

	int GetBounces() const;
	bool CanBounce() const;
	void AddBounce();

	bool IsMarkedForRemoval() const override;
	uint16_t GetIdentifier() const;

	std::function<void(uint16_t)> m_on_destroyed;

	~Projectile();

private:
	virtual void UpdateCurrent(sf::Time dt, CommandQueue& commands) override;
	virtual void DrawCurrent(sf::RenderTarget& target, sf::RenderStates states) const override;


private:
	ProjectileType m_type;
	sf::Sprite m_sprite;
	sf::Vector2f m_target_direction;

	// (Ben's Claude) An ID to index projectiles for world
	uint16_t projectile_id;

	Tank* m_owner;
	int m_bounces = 0;
	bool m_has_bounced;
};

