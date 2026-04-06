#include "SocketWrapperPCH.hpp"
#include "data_tables.hpp"
#include "tank_type.hpp"
#include "projectile_type.hpp"
#include "wall_type.hpp"
#include "tank.hpp"
#include "SFML/Graphics/Rect.hpp"
#include "particle_type.hpp"
#include "turret_type.hpp"


std::vector<sf::Vector2f> InitializeTankPositions()
{
	std::vector<sf::Vector2f>;

	std::vector<sf::Vector2f> data = {
		// Row 1
		{312.f, 2512.f}, {445.f, 2512.f}, {578.f, 2512.f}, {712.f, 2512.f},

		// Row 2
		{312.f, 2645.f}, {445.f, 2645.f}, {578.f, 2645.f}, {712.f, 2645.f},

		// Row 3
		{312.f, 2778.f}, {445.f, 2778.f}, {578.f, 2778.f}, {712.f, 2778.f},

		// Row 4
		{312.f, 2912.f}, {445.f, 2912.f}, {578.f, 2912.f}, {712.f, 2912.f}
	};

	return data;
}

/// <summary>
/// Modified: Ben Mc Keever D00254413
/// 
/// Modified: Kaylon Riodan D00255039
/// Reduced hit points so every player dies in 1 shot
/// Slowed down fireing to reduce spamming in game
/// Updated texture ids and rectangles to work with new sprite sheet
/// </summary>
std::vector<TankData> InitializeTankData()
{
	std::vector<TankData> data(static_cast<int>(TankType::kTankCount));

	auto& tank = data[0];
	tank.m_hitpoints = 10;
	tank.m_fire_interval = sf::seconds(2.5);
	tank.m_texture = TextureID::kEntities;
	tank.m_texture_rect = sf::IntRect({ 0, 0 }, { 40, 38 });

	return data;
}

/// <summary>
/// Authored: Kaylon Riordan D00255039
/// Created texture data for new turret objects
/// </summary>
std::vector<TurretData> InitializeTurretData()
{
	std::vector<TurretData> data(static_cast<int>(TurretType::kTurretCount));

	auto& turret = data[0];
	turret.m_texture = TextureID::kEntities;
	turret.m_texture_rect = sf::IntRect({ 40, 8 }, { 50, 16 });

	return data;
}

/// <summary>
/// Modified: Kaylon Riodan D00255039
/// Added a setting for how many times bullets can bounce before being destroyed
/// Updated texture ids and rectangles to work with new sprite sheet
/// </summary>
std::vector<ProjectileData> InitializeProjectileData()
{
	std::vector<ProjectileData> data(static_cast<int>(ProjectileType::kProjectileCount));

	auto& bullet = data[0];
	bullet.m_damage = 10;
	bullet.m_speed = 300;
	bullet.m_max_bounces = 2;
	bullet.m_texture = TextureID::kEntities;
	bullet.m_texture_rect = sf::IntRect({ 40, 0 }, { 14, 8 });

	return data;
}

/// <summary>
/// Authored: Kaylon Riordan D00255039
/// Created texture data for new wall objects
/// </summary>
std::vector<WallData> InitializeWallData()
{
	std::vector<WallData> data(static_cast<int>(WallType::kWallCount));

	data[static_cast<int>(WallType::kMetalWall)].m_texture = TextureID::kEntities;
	data[static_cast<int>(WallType::kMetalWall)].m_texture_rect = sf::IntRect({ 0, 38 }, { 112, 28 });
	data[static_cast<int>(WallType::kWoodWall)].m_texture = TextureID::kEntities;
	data[static_cast<int>(WallType::kWoodWall)].m_texture_rect = sf::IntRect({ 0, 66 }, { 28, 28 });
	data[static_cast<int>(WallType::kExterior)].m_texture = TextureID::kExterior;
	data[static_cast<int>(WallType::kExterior)].m_texture_rect = sf::IntRect({ 0, 0 }, { 1920, 28 });
	return data;
}


std::vector<ParticleData> InitializeParticleData()
{
	std::vector<ParticleData> data(static_cast<int>(ParticleType::kParticleCount));

	data[static_cast<int>(ParticleType::kPropellant)].m_color = sf::Color(255, 255, 50);
	data[static_cast<int>(ParticleType::kPropellant)].m_lifetime = sf::seconds(0.1f);

	data[static_cast<int>(ParticleType::kSmoke)].m_color = sf::Color(50, 50, 50);
	data[static_cast<int>(ParticleType::kSmoke)].m_lifetime = sf::seconds(0.3f);
	return data;
}