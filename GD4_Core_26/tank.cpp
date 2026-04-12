#include "tank.hpp"
#include "texture_id.hpp"
#include "data_tables.hpp"
#include "utility.hpp"
#include "projectile.hpp"
#include "projectile_type.hpp"
#include <iostream>
#include "sound_node.hpp"
#include "tank_type.hpp"
#include "turret.hpp"
#include "constants.hpp"
#include "state.hpp"

namespace
{
	const std::vector<TankData> Table = InitializeTankData();
}

TextureID ToTextureID(TankType type)
{
	return TextureID::kEntities;
}

/// <summary>
/// Formerly Aircraft.cpp
/// Modified: Ben with assistance of Claude, Kaylon
/// </summary>
Tank::Tank(TankType type, const TextureHolder& textures, const FontHolder& fonts, uint8_t identifier, PlayerDetails* details)
	: Entity(Table[static_cast<int>(type)].m_hitpoints)
	, m_type(type)
	, m_sprite(textures.Get(Table[static_cast<int>(type)].m_texture), Table[static_cast<int>(type)].m_texture_rect)
	, m_identifier(identifier)
	, m_details(details)
	, m_name(details->m_name)
	, m_colour(details->m_colour)
	, m_name_display(nullptr)
	, m_distance_travelled(0.f)
	, m_directions_index(0)
	, m_fire_rate(1)
	, m_is_firing(false)
	, m_fire_countdown(sf::Time::Zero)
	, m_is_marked_for_removal(false)
	, m_show_explosion(true)
	, m_explosion(textures.Get(TextureID::kExplosion))
	, m_explosion_began(false)
	, m_shot_counter(0)
{
	const std::vector<sf::Color> colours = InitializeTankColours();
	details->m_colour = colours[identifier - 1];
	m_colour = details->m_colour;
	m_sprite.setColor(m_colour);
	m_explosion.SetFrameSize(sf::Vector2i(200, 200));
	m_explosion.SetNumFrames(9);
	m_explosion.SetDuration(sf::seconds(0.5));

	// Center sprite origins for correct rotation/positioning (GPT)
	Utility::CentreOrigin(m_sprite);
	Utility::CentreOrigin(m_explosion);;

	m_fire_command.category = static_cast<int>(ReceiverCategories::kScene);
	m_fire_command.action = [this, &textures](SceneNode& node, sf::Time dt)
	{
		CreateBullet(node, textures);
	};

	// Attach a turret to the tank with the same identifier
	std::unique_ptr<Turret> turret;
	turret = std::unique_ptr<Turret>(new Turret(TurretType::kTurret, textures, m_colour));
	m_turret = turret.get();
	AttachChild(std::move(turret));
	m_turret->SetIdentifier(identifier);

	// (Kaylon) Display username beneath tank
	std::unique_ptr<TextNode> name_display(new TextNode(fonts, m_name));
	m_name_display = name_display.get();
	m_name_display->SetString(m_name);
	AttachChild(std::move(name_display));
	UpdateTexts();
}

/// <summary>
/// Unmodified
/// </summary>
uint8_t	Tank::GetIdentifier() const
{
	return m_identifier;
}

/// <summary>
/// Only one type of tank exists in this version
/// </summary>
unsigned int Tank::GetCategory() const
{
	return static_cast<unsigned int>(ReceiverCategories::kTank);
}

/// <summary>
/// Authored: Kaylon
/// </summary>
void Tank::AddPoints(int points)
{
	m_details->m_score = m_details->m_score + points;
}

/// <summary>
/// Authored: Kaylon
/// </summary>
void Tank::UpdateTexts()
{
	if (IsDestroyed())
	{
		m_name_display->SetString("");
	}
	else
	{
		// Set the texts position relative to the world position to stay below the tank
		float worldAngle = GetWorldRotation().asRadians();
		m_name_display->setPosition(sf::Vector2f(50.f * std::sin(worldAngle), 50.f * std::cos(worldAngle)));
		m_name_display->setRotation(-getRotation());
	}
}

/// <summary>
/// We don't store it on the table anymore so just use a constant
/// Modified: Ben
/// </summary>
float Tank::GetMaxSpeed() const
{
	return kPlayerSpeed;
}

/// <summary>
/// Unmodified
/// </summary>
void Tank::Fire()
{
	if (Table[static_cast<int>(m_type)].m_fire_interval != sf::Time::Zero)
	{
		m_is_firing = true;
	}
}

/// <summary>
/// Spread level isn't a mechanic in our game
/// Modified: Ben
/// </summary>
void Tank::CreateBullet(SceneNode& node, const TextureHolder& textures)
{
	CreateProjectile(node, ProjectileType::kBullet, 0.0f, 0.5f, textures);
}

/// <summary>
/// Add an ID to each projectile. Change offset of projectiles with change of sprite orientation
/// CA1 Modified: Ben
/// </summary>
void Tank::CreateProjectile(SceneNode& node, ProjectileType type, float x_offset, float y_offset, const TextureHolder& textures)
{
	// (Ben's Claude) - Create a projectile id
	// The top byte is the owner's id and the bottom is the shot number e.g client 1's third shot is 0x0103
	uint16_t projectile_id = (static_cast<uint16_t>(m_identifier) << 8) | (m_shot_counter & 0xFF);

	// (Ben's Claude) Tally how many shots were made.
	++m_shot_counter;

	// Initalise projectile
	std::unique_ptr<Projectile> projectile(new Projectile(type, textures, m_colour, this, projectile_id));

	// Convert to radians
	float radians = (m_turret->GetWorldRotation()).asRadians();

	// Create the unit vector pointing in the angle of our direction
	// https://gamedev.stackexchange.com/questions/117583/how-do-i-get-a-vector-from-an-angle
	sf::Vector2f direction(
		std::cos(radians),
		std::sin(radians)
	);

	projectile->setPosition(GetWorldPosition()  + (direction * 25.0f));
	projectile->setRotation(m_turret->GetWorldRotation());
	projectile->SetVelocity(direction * projectile->GetMaxSpeed());

	// (Ben's Claude) notify callback to inform world.cpp of the projectile
	if (m_on_projectile_fired)
		m_on_projectile_fired(projectile.get());

	//std::cout << "Just created projectile, GetIdentifier()=" << std::to_string(projectile->GetIdentifier())
	//	<< " projectile_id was=" << std::to_string(projectile_id) << std::endl;

	node.AttachChild(std::move(projectile));
}

/// <summary>
/// Unmodified
/// </summary>
sf::FloatRect Tank::GetBoundingRect() const
{
	return GetWorldTransform().transformRect(m_sprite.getGlobalBounds());
}

/// <summary>
/// Unmodified
/// </summary>
bool Tank::IsMarkedForRemoval() const
{
	return IsDestroyed() && (m_explosion.IsFinished() || !m_show_explosion);
}

/// <summary>
/// Unmodified
/// </summary>
void Tank::PlayLocalSound(CommandQueue & commands, SoundEffect effect)
{
	sf::Vector2f world_position = GetWorldPosition();

	Command command;
	command.category = static_cast<int>(ReceiverCategories::kSoundEffect);
	command.action = DerivedAction<SoundNode>(
		[effect, world_position](SoundNode& node, sf::Time)
		{
			node.PlaySound(effect, world_position);
		});
	commands.Push(command);
}

/// <summary>
/// Unmodified
/// </summary>
void Tank::DrawCurrent(sf::RenderTarget & target, sf::RenderStates states) const
{
	if (IsDestroyed() && m_show_explosion)
	{
		target.draw(m_explosion, states);
	}
	else
	{
		target.draw(m_sprite, states);
	}
}

/// <summary>
/// Remove isAllied check. Not relevant for our game.
/// Modified: Ben
/// </summary>
void Tank::UpdateCurrent(sf::Time dt, CommandQueue & commands)
{
	UpdateTexts();
	if (IsDestroyed())
	{
		m_turret->Hide();
		m_explosion.Update(dt);

		//Play explosion sound only once
		if (!m_explosion_began)
		{
			SoundEffect soundEffect = (Utility::RandomInt(2) == 0) ? SoundEffect::kExplosion1 : SoundEffect::kExplosion2;
			PlayLocalSound(commands, soundEffect);

			m_explosion_began = true;
		}
		return;
	}
	Entity::UpdateCurrent(dt, commands);

	//Check if bullets or missiles were fired
	CheckProjectileLaunch(dt, commands);
}

/// <summary>
/// Remove isAllied check. Not relevant for our game.
/// Modified: Ben
/// </summary>
void Tank::CheckProjectileLaunch(sf::Time dt, CommandQueue & commands)
{
	if (m_is_firing && m_fire_countdown <= sf::Time::Zero)
	{
		// (Ben) Alternate between formerly red vs blue gunfire sfx
		SoundEffect soundEffect = (Utility::RandomInt(2) == 0) ? SoundEffect::kGunfire1 : SoundEffect::kGunfire2;
		PlayLocalSound(commands, soundEffect);
		commands.Push(m_fire_command);
		m_fire_countdown += Table[static_cast<int>(m_type)].m_fire_interval / (m_fire_rate + 1.f);
		m_is_firing = false;
	}
	else if (m_fire_countdown > sf::Time::Zero)
	{
		m_fire_countdown -= dt;
		m_is_firing = false;
	}
}

/// <summary>
/// Unmodified
/// </summary>
void Tank::Remove()
{
	Entity::Remove();
	m_show_explosion = false;
}
