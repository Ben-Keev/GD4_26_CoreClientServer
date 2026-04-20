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
#include "projectile.hpp"
#include <string>
#include <functional>

struct PlayerDetails;

/// <summary>
/// Formerly Aircraft.cpp
/// Modified: Ben with assistance of Claude, Kaylon
/// </summary>
class Tank : public Entity
{
public:
	Tank(TankType type, const TextureHolder& textures, const FontHolder& fonts, uint8_t identifier, PlayerDetails* details);
	unsigned int GetCategory() const override;

	uint8_t GetIdentifier() const;
	float GetMaxSpeed() const;

	void AddPoints(int points);

	void UpdateTexts();

	void Fire();
	void CreateBullet(SceneNode& node, const TextureHolder& textures);
	void CreateProjectile(SceneNode& node, ProjectileType type, float x_offset, float y_offset, const TextureHolder& textures);

	sf::FloatRect GetBoundingRect() const override;
	bool IsMarkedForRemoval() const override;
	void Remove() override;
	Turret* GetTurret() const { return m_turret; }
	void PlayLocalSound(CommandQueue& commands, SoundEffect effect);

	std::function<void(Projectile*)> m_on_projectile_fired;
	std::function<void(sf::Vector2f, float)> m_on_fire;

	/// <summary>
	///  Modified: Kaylon's Claude, overide bullets spawn data with data sent by the player that spawned it.
	/// </summary>
	void SetSpawnOverride(sf::Vector2f pos, float rot)
	{
		m_spawn_override_pos = pos;
		m_spawn_override_rot = rot;
		m_has_spawn_override = true;
	}

	sf::Vector2f m_spawn_override_pos{};
	float m_spawn_override_rot = 0.f;
	bool m_has_spawn_override = false;

private:
	virtual void DrawCurrent(sf::RenderTarget& target, sf::RenderStates states) const;
	virtual void UpdateCurrent(sf::Time dt, CommandQueue& commands) override;

	void CheckProjectileLaunch(sf::Time dt, CommandQueue& commands);

private:
	TankType m_type;
	sf::Sprite m_sprite;
	uint8_t m_identifier;
	PlayerDetails* m_details;
	std::string m_name;
	sf::Color m_colour;
	Animation m_explosion;

	TextNode* m_name_display;

	float m_distance_travelled;
	int m_directions_index;

	Command m_fire_command;
	unsigned int m_fire_rate;
	bool m_is_firing;
	sf::Time m_fire_countdown;

	bool m_is_marked_for_removal;
	bool m_show_explosion;
	bool m_explosion_began;

	uint8_t m_shot_counter;
	Turret* m_turret;
};

