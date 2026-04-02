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

	data[static_cast<int>(TankType::kRedTank)].m_hitpoints = 10;
	data[static_cast<int>(TankType::kRedTank)].m_fire_interval = sf::seconds(2.5);
	data[static_cast<int>(TankType::kRedTank)].m_texture = TextureID::kEntities;
	data[static_cast<int>(TankType::kRedTank)].m_texture_rect = sf::IntRect({ 260, 80 }, { 38, 40 });

	data[static_cast<int>(TankType::kBlueTank)].m_hitpoints = 10;
	data[static_cast<int>(TankType::kBlueTank)].m_fire_interval = sf::seconds(2.5);
	data[static_cast<int>(TankType::kBlueTank)].m_texture = TextureID::kEntities;
	data[static_cast<int>(TankType::kBlueTank)].m_texture_rect = sf::IntRect({ 216, 0 }, { 42, 42 });

	return data;
}

/// <summary>
/// Authored: Kaylon Riordan D00255039
/// Created texture data for new turret objects
/// </summary>
std::vector<TurretData> InitializeTurretData()
{
	std::vector<TurretData> data(static_cast<int>(TurretType::kTurretCount));

	data[static_cast<int>(TurretType::kRedTurret)].m_texture = TextureID::kEntities;
	data[static_cast<int>(TurretType::kRedTurret)].m_texture_rect = sf::IntRect({ 343, 108 }, { 16, 50 });
	data[static_cast<int>(TurretType::kBlueTurret)].m_texture = TextureID::kEntities;
	data[static_cast<int>(TurretType::kBlueTurret)].m_texture_rect = sf::IntRect({ 360, 138 }, { 12, 50 });

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