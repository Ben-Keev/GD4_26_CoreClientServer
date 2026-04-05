#pragma once
#include "texture_id.hpp"
#include <SFML/System/Time.hpp>
#include "tank.hpp"
#include <SFML/System/Vector2.hpp>

/// <summary>
/// Modified: Kaylon Riordan D00255039
/// </summary>

struct Direction
{
	Direction(float angle, float distance)
		: m_angle(angle), m_distance(distance) {
	}
	float m_angle;
	float m_distance;
};

struct TankData
{
	int m_hitpoints;
	TextureID m_texture;
	sf::IntRect m_texture_rect;
	sf::Color m_colour;
	sf::Time m_fire_interval;
	std::vector<Direction> m_directions;
	bool m_has_roll_animation;
};

struct TurretData
{
	TextureID m_texture;
	sf::IntRect m_texture_rect;
	sf::Color m_colour;
};

/// <summary>
/// Modified: Kaylon Riordan D00255039
/// Added int to track bullet bounces
/// </summary>
struct ProjectileData
{
	int m_damage;
	float m_speed;
	int m_max_bounces;
	TextureID m_texture;
	sf::IntRect m_texture_rect;
	sf::Color m_colour;
};

struct WallData
{
	TextureID m_texture;
	sf::IntRect m_texture_rect;
};

struct ParticleData
{
	sf::Color m_color;
	sf::Time m_lifetime;
};

/// <summary>
/// Modified: Kaylon Riordan D00255039
/// Initialized wall and turret data
/// </summary>

std::vector<sf::Vector2f> InitializeTankPositions();
std::vector<TankData> InitializeTankData();
std::vector<TurretData> InitializeTurretData();
std::vector<ProjectileData> InitializeProjectileData();
std::vector<WallData> InitializeWallData();
std::vector<ParticleData> InitializeParticleData();





