#pragma once
#include "entity.hpp"
#include "tank_type.hpp"
#include "resource_identifiers.hpp"
#include "text_node.hpp"
#include "projectile_type.hpp"
#include "command_queue.hpp"
#include "animation.hpp"
#include "turret.hpp"
#include "player.hpp"
#include "state.hpp"

struct PlayerDetails;

class Tank : public Entity
{
public:
	Tank(TankType type, const TextureHolder& textures, const FontHolder& fonts, uint8_t identifier, PlayerDetails* details);
	unsigned int GetCategory() const override;

	void DisablePickups();
	uint8_t GetIdentifier();
	//int GetMissileAmmo() const;
	//void SetMissileAmmo(int ammo);

	void IncreaseFireRate();
	void IncreaseFireSpread();
	//void CollectMissile(unsigned int count);

	void AddPoints(int points);

	void UpdateTexts();
	void UpdateMovementPattern(sf::Time dt);

	float GetMaxSpeed() const;
	void Fire();
	//void LaunchMissile();
	void CreateBullet(SceneNode& node, const TextureHolder& textures);
	void CreateProjectile(SceneNode& node, ProjectileType type, float x_offset, float y_offset, const TextureHolder& textures);

	sf::FloatRect GetBoundingRect() const override;
	bool IsMarkedForRemoval() const override;
	void Remove() override;
	Turret* GetTurret() const { return m_turret; }
	void PlayLocalSound(CommandQueue& commands, SoundEffect effect);

private:
	virtual void DrawCurrent(sf::RenderTarget& target, sf::RenderStates states) const;
	virtual void UpdateCurrent(sf::Time dt, CommandQueue& commands) override;

	void CheckProjectileLaunch(sf::Time dt, CommandQueue& commands);
	bool IsAllied() const;
	//void CreatePickup(SceneNode& node, const TextureHolder& textures) const;
	//void CheckPickupDrop(CommandQueue& commands);
	//void UpdateRollAnimation();

private:
	TankType m_type;
	sf::Sprite m_sprite;
	uint8_t m_identifier;
	PlayerDetails* m_details;
	string m_name;
	sf::Color m_colour;
	Animation m_explosion;

	TextNode* m_name_display;

	float m_distance_travelled;
	int m_directions_index;

	Command m_fire_command;
	Command m_missile_command;
	Command m_drop_pickup_command;

	unsigned int m_fire_rate;
	unsigned int m_spread_level;
	unsigned int m_missile_ammo;

	bool m_is_firing;
	bool m_is_launching_missile;
	bool m_spawned_pickup;

	sf::Time m_fire_countdown;

	bool m_is_marked_for_removal;
	bool m_show_explosion;
	bool m_explosion_began;

	bool m_pickups_enabled;

	Turret* m_turret;
};

