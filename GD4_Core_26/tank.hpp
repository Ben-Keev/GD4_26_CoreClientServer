#pragma once
#include "entity.hpp"
#include "tank_type.hpp"
#include "resource_identifiers.hpp"
#include "text_node.hpp"
#include "projectile_type.hpp"
#include "command_queue.hpp"
#include "turret.hpp"
#include "animation.hpp"

/// <summary>
/// Modified: Kaylon Riordan D00255039
/// Added pointer to the tank's turret
/// </summary>

class Tank : public Entity
{
public:
	Tank(TankType type, const TextureHolder& textures, const FontHolder& fonts);
	unsigned int GetCategory() const override;

	void Fire();
	void CreateBullet(SceneNode& node, const TextureHolder& textures);
	void CreateProjectile(SceneNode& node, ProjectileType type, float x_offset, float y_offset, const TextureHolder& textures) const;

	sf::FloatRect GetBoundingRect() const override;
	bool IsMarkedForRemoval() const override;
	void PlayLocalSound(CommandQueue& commands, SoundEffect effect);

	void AdaptVelocity();

protected:
	virtual void DrawCurrent(sf::RenderTarget& target, sf::RenderStates states) const;
	virtual void UpdateCurrent(sf::Time dt, CommandQueue& commands) override;

	void CheckProjectileLaunch(sf::Time dt, CommandQueue& commands);
	bool IsAllied() const;

private:
	TankType m_type;
	sf::Sprite m_sprite;
	Animation m_explosion;

	Turret* m_turret;

	float m_distance_travelled;
	int m_directions_index;

	Command m_fire_command;

	unsigned int m_fire_rate;
	unsigned int m_spread_level;

	bool m_is_firing;

	sf::Time m_fire_countdown;

	bool m_is_marked_for_removal;
	bool m_show_explosion;
	bool m_explosion_began;
};

