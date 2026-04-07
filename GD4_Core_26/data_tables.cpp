#include "SocketWrapperPCH.hpp"
#include "data_tables.hpp"
#include "tank_type.hpp"
#include "projectile_type.hpp"
#include "wall_type.hpp"
#include "tank.hpp"
#include "SFML/Graphics/Rect.hpp"
#include "particle_type.hpp"
#include "turret_type.hpp"

// Chat-GPT generated function to generate based on spacing & start positions
std::vector<sf::Vector2f> InitializeTankPositions()
{
	std::vector<sf::Vector2f> data;

	float startX = 400.f;
	float startY = 200.f;
	float spacingX = 300.f;
	float spacingY = 150.f;

	for (int row = 0; row < 4; ++row)
	{
		for (int col = 0; col < 4; ++col)
		{
			data.emplace_back(
				startX + col * spacingX,
				startY + row * spacingY
			);
		}
	}

	return data;
}

std::vector<sf::Color> InitializeTankColours()
{
	std::vector<sf::Color> data(16);

	data[0] = sf::Color::Red;
	data[1] = sf::Color::Green;
	data[2] = sf::Color::Blue;
	data[3] = sf::Color::Yellow;
	data[4] = sf::Color::Magenta;
	data[5] = sf::Color::Cyan;
	data[6] = sf::Color(255, 128, 0); // Orange
	data[7] = sf::Color(128, 0, 255); // Purple
	data[8] = sf::Color(0, 255, 128); // Aqua
	data[9] = sf::Color(255, 0, 128); // Pink
	data[10] = sf::Color(128, 255, 0); // Lime
	data[11] = sf::Color(255, 255, 0); // Yellow
	data[12] = sf::Color(0, 128, 255); // Sky Blue
	data[13] = sf::Color(128, 255, 255); // Light Cyan
	data[14] = sf::Color(255, 128, 128); // Light Red
	data[15] = sf::Color(128, 128, 255); // Light Blue

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