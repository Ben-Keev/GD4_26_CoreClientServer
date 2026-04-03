#include "SocketWrapperPCH.hpp"
#include "data_tables.hpp"
#include "tank_type.hpp"
#include "projectile_type.hpp"
#include "wall_type.hpp"
#include "tank.hpp"
#include "SFML/Graphics/Rect.hpp"
#include "particle_type.hpp"

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

	for (int i = 0; i <= static_cast<int>(TankType::kTanTank); ++i)
	{
		auto& tank = data[i];

		tank.m_hitpoints = 10;
		tank.m_fire_interval = sf::seconds(2.5);
		tank.m_texture = TextureID::kEntities;

		// All tanks are red for now
		tank.m_texture_rect = sf::IntRect({ 260, 80 }, { 38, 40 });
	}

	return data;
}

/// <summary>
/// Authored: Kaylon Riordan D00255039
/// Created texture data for new turret objects
/// </summary>
std::vector<TurretData> InitializeTurretData()
{
	std::vector<TurretData> data(static_cast<int>(TurretType::kTurretCount));

	for (int i = 0; i < static_cast<int>(TurretType::kTanTurret); ++i)
	{
		auto& turret = data[i];

		turret.m_texture = TextureID::kEntities;

		// All turrets appear red for now
		turret.m_texture_rect = sf::IntRect({ 343, 108 }, { 16, 50 });
	}

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
	data[static_cast<int>(ProjectileType::kRedBullet)].m_damage = 10;
	data[static_cast<int>(ProjectileType::kRedBullet)].m_speed = 300;
	data[static_cast<int>(ProjectileType::kRedBullet)].m_max_bounces = 2;
	data[static_cast<int>(ProjectileType::kRedBullet)].m_texture = TextureID::kEntities;
	data[static_cast<int>(ProjectileType::kRedBullet)].m_texture_rect = sf::IntRect({ 96, 176 }, { 8, 14 });

	data[static_cast<int>(ProjectileType::kBlueBullet)].m_damage = 10;
	data[static_cast<int>(ProjectileType::kBlueBullet)].m_speed = 300;
	data[static_cast<int>(ProjectileType::kBlueBullet)].m_max_bounces = 2;
	data[static_cast<int>(ProjectileType::kBlueBullet)].m_texture = TextureID::kEntities;
	data[static_cast<int>(ProjectileType::kBlueBullet)].m_texture_rect = sf::IntRect({ 181, 106 }, { 8, 14 });

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
	data[static_cast<int>(WallType::kMetalWall)].m_texture_rect = sf::IntRect({ 56, 367 }, { 56, 28 });
	data[static_cast<int>(WallType::kWoodWall)].m_texture = TextureID::kEntities;
	data[static_cast<int>(WallType::kWoodWall)].m_texture_rect = sf::IntRect({ 0, 367 }, { 28, 28 });
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