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
	std::vector<sf::Vector2f> data(15);

	data[0] = sf::Vector2f(56, 56);
	data[1] = sf::Vector2f(1736, 840);
	data[2] = sf::Vector2f(1736, 56);
	data[3] = sf::Vector2f(56, 840);
	data[4] = sf::Vector2f(896, 56);
	data[5] = sf::Vector2f(896, 840);
	data[6] = sf::Vector2f(56, 448);
	data[7] = sf::Vector2f(1736, 448);
	data[8] = sf::Vector2f(504, 56);
	data[9] = sf::Vector2f(1288, 840);
	data[10] = sf::Vector2f(1288, 56);
	data[11] = sf::Vector2f(504, 840);
	data[12] = sf::Vector2f(448, 448);
	data[13] = sf::Vector2f(1344, 448);
	data[14] = sf::Vector2f(896, 448);

	return data;
}

std::vector<sf::Color> InitializeTankColours()
{
	std::vector<sf::Color> data(15);

	data[0] = sf::Color(230,70,70);			// Red
	data[1] = sf::Color(50, 160, 50);		// Green
	data[2] = sf::Color(80, 140, 210);		// Blue
	data[3] = sf::Color(220, 220, 70);		// Yellow
	data[4] = sf::Color(160, 90, 230);		// Purple
	data[5] = sf::Color(240, 120, 240);		// Pink
	data[6] = sf::Color(240, 120, 70);		// Orange
	data[7] = sf::Color(100, 220, 220);		// Cyan
	data[8] = sf::Color(50, 50, 50);		// Black
	data[9] = sf::Color(250, 250, 250);		// White
	data[10] = sf::Color(150, 150, 150);	// Grey
	data[11] = sf::Color(120, 90, 70);		// Brown
	data[12] = sf::Color(220, 120, 140);	// Tan
	data[13] = sf::Color(40, 50, 100);		// Navy
	data[14] = sf::Color(110, 130, 90);		// Lime

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