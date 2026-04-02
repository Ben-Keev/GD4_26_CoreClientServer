#include "SocketWrapperPCH.hpp"
#include "tank.hpp"
#include "texture_id.hpp"
#include "data_tables.hpp"
#include "utility.hpp"
#include "constants.hpp"
#include "projectile.hpp"
#include "projectile_type.hpp"
#include "sound_node.hpp"

/// <summary>
/// Modified: Ben Mc Keever D00254413
/// Comments wrote by Chat-GPT marked with (GPT)
/// Modified: Kaylon Riordan D00255039
/// </summary>

namespace
{
	// Static lookup table containing configuration data for every tank type (GPT)
	const std::vector<TankData> Table = InitializeTankData();
}

/// <summary>
/// Constructs a Tank object and initializes all gameplay-related components
/// including sprite, turret, commands, explosion animation, and UI text. (GPT)
/// </summary>
Tank::Tank(TankType type, const TextureHolder& textures, const FontHolder& fonts)
	: Entity(Table[static_cast<int>(type)].m_hitpoints)
	, m_type(type)
	, m_sprite(textures.Get(Table[static_cast<int>(type)].m_texture), Table[static_cast<int>(type)].m_texture_rect)
	, m_distance_travelled(0.f)
	, m_directions_index(0)
	, m_fire_rate(1)
	, m_spread_level(1)
	, m_is_firing(false)
	, m_fire_countdown(sf::Time::Zero)
	, m_is_marked_for_removal(false)
	, m_show_explosion(true)
	, m_explosion(textures.Get(TextureID::kExplosion))
	, m_explosion_began(false)
{
	m_explosion.SetFrameSize(sf::Vector2i(200, 200));
	m_explosion.SetNumFrames(9);
	m_explosion.SetDuration(sf::seconds(0.5));

	// Center sprite origins for correct rotation/positioning (GPT)
	Utility::CentreOrigin(m_sprite);
	Utility::CentreOrigin(m_explosion);

	// Setup command for firing bullets via command queue (GPT)
	m_fire_command.category = static_cast<int>(ReceiverCategories::kScene);
	m_fire_command.action = [this, &textures](SceneNode& node, sf::Time dt)
		{
			CreateBullet(node, textures);
		};
	/// <summary>
	// Modified: Kaylon Riordan D00255039
	// Create a pointer to a turret and set the type of turrent, based off type of tank, then attach to tank as child
	/// <summary>
	std::unique_ptr<Turret> turret;
	if (type == TankType::kRedTank)
	{
		turret = std::unique_ptr<Turret>(new Turret(TurretType::kRedTurret, textures));
	}
	else
	{
		turret = std::unique_ptr<Turret>(new Turret(TurretType::kBlueTurret, textures));
	}
	m_turret = turret.get();
	AttachChild(std::move(turret));
}

/// <summary>
/// Returns the category used by the command system for filtering. (GPT)
/// </summary>
unsigned int Tank::GetCategory() const
{
	if (IsAllied())
	{
		return static_cast<unsigned int>(ReceiverCategories::kRedTank);
	}
	return static_cast<unsigned int>(ReceiverCategories::kBlueTank);
}

/// <summary>
/// Enables firing state if tank has a valid fire interval. (GPT)
/// </summary>
void Tank::Fire()
{
	if (Table[static_cast<int>(m_type)].m_fire_interval != sf::Time::Zero)
	{
		m_is_firing = true;
	}
}

/// <summary>
/// Creates bullets depending on the current spread level. (GPT)
/// </summary>
void Tank::CreateBullet(SceneNode& node, const TextureHolder& textures)
{
	// It may be an enemy bullet or an allied bullet.
	ProjectileType type = IsAllied() ? ProjectileType::kRedBullet : ProjectileType::kBlueBullet;

	// How many bullets are fired based on the level accquired
	switch (m_spread_level)
	{
	case 1:
		if (m_is_firing)
		{
			CreateProjectile(node, type, 0.0f, 0.0f, textures);
			m_is_firing = false;
		}
		break;
	case 2:
		if (m_is_firing)
		{
			CreateProjectile(node, type, -0.5f, 0.5f, textures);
			CreateProjectile(node, type, 0.5f, 0.5f, textures);
			m_is_firing = false;
			break;
		}
	case 3:
		if (m_is_firing)
		{
			CreateProjectile(node, type, 0.0f, 0.5f, textures);
			CreateProjectile(node, type, -0.5f, 0.5f, textures);
			CreateProjectile(node, type, 0.5f, 0.5f, textures);
			m_is_firing = false;
			break;
		}
	}
}

/// <summary>
/// Modified: Ben Mc Keever D00254413
/// Now projectiles face the direction of the turret
/// Creates and launches a projectile in the direction of the turret. (GPT)
/// Modified: Kaylon Riordan D00255039
/// Changed projectiles rotation to check turret instead of tank
/// </summary>
void Tank::CreateProjectile(SceneNode& node, ProjectileType type, float x_offset, float y_offset, const TextureHolder& textures) const
{
	if (m_is_firing)
	{
		std::unique_ptr<Projectile> projectile(new Projectile(type, textures));

		sf::Vector2f offset(x_offset * m_sprite.getGlobalBounds().size.x, y_offset * m_sprite.getGlobalBounds().size.y);

		// Subtract 90 degrees but keep the format in radians
		float radians = (m_turret->GetWorldRotation() - sf::degrees(-90.f)).asRadians();

		// Create the unit vector pointing in the angle of our direction
		// https://gamedev.stackexchange.com/questions/117583/how-do-i-get-a-vector-from-an-angle
		sf::Vector2f direction(
			std::cos(radians),
			std::sin(radians)
		);

		projectile->setPosition(GetWorldPosition() + offset + (direction * 25.0f));
		projectile->setRotation(m_turret->GetWorldRotation() - sf::degrees(180.f));
		projectile->SetVelocity(direction * projectile->GetMaxSpeed());
		node.AttachChild(std::move(projectile));
	}
}

/// <summary>
/// Returns the world-space bounding rectangle for collision detection. (GPT)
/// </summary>
sf::FloatRect Tank::GetBoundingRect() const
{
	return GetWorldTransform().transformRect(m_sprite.getGlobalBounds());
}

/// <summary>
/// Determines whether the tank should be removed from the scene. (GPT)
/// </summary>
bool Tank::IsMarkedForRemoval() const
{
	return IsDestroyed() && (m_explosion.IsFinished() || !m_show_explosion);
}

/// <summary>
/// Draws the tank sprite or explosion animation depending on state. (GPT)
/// </summary>
void Tank::DrawCurrent(sf::RenderTarget& target, sf::RenderStates states) const
{
	if (IsDestroyed() && m_show_explosion)
	{
		m_turret->Hide();
		target.draw(m_explosion, states);
	}
	else
	{
		target.draw(m_sprite, states);
	}
}

/// <summary>
/// Updates tank logic each frame including movement, firing, and destruction handling. (GPT)
/// </summary>
void Tank::UpdateCurrent(sf::Time dt, CommandQueue& commands)
{
	if (IsDestroyed())
	{
		m_explosion.Update(dt);
		//Play explosion sound only once
		if (!m_explosion_began)
		{
			SoundEffect soundEffect = (Utility::RandomInt(2) == 0) ? SoundEffect::kExplosion1 : SoundEffect::kExplosion2;
			PlayLocalSound(commands, soundEffect);
			m_explosion_began = true;
		}
		m_is_marked_for_removal = true;
		return;
	}
	Entity::UpdateCurrent(dt, commands);

	//Check if bullets were fired
	CheckProjectileLaunch(dt, commands);
}

/// <summary>
/// Handles projectile firing cooldown and command dispatch. (GPT)
/// </summary>
void Tank::CheckProjectileLaunch(sf::Time dt, CommandQueue& commands)
{
	if (m_is_firing && m_fire_countdown <= sf::Time::Zero)
	{
		PlayLocalSound(commands, IsAllied() ? SoundEffect::kBlueGunfire : SoundEffect::kRedGunfire);

		commands.Push(m_fire_command);
		m_fire_countdown += Table[static_cast<int>(m_type)].m_fire_interval / (m_fire_rate + 1.f);
	}
	else if (m_fire_countdown > sf::Time::Zero)
	{
		m_fire_countdown -= dt;
		m_is_firing = false;
	}
}

/// <summary>
/// Returns true if tank is allied (red tank). (GPT)
/// </summary>
bool Tank::IsAllied() const
{
	return m_type == TankType::kRedTank;
}

/// <summary>
/// Plays a spatial sound effect at the tank's world position. (GPT)
/// </summary>
void Tank::PlayLocalSound(CommandQueue& commands, SoundEffect effect)
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
/// Modified: Ben Mc Keever D00254413
/// No longer adjusts velocity based on scrolling as there is no scrolling. No reason for it to exist in world.cpp anymore.
/// Taken from world.cpp
/// Normalizes diagonal velocity to prevent faster diagonal movement. (GPT)
/// </summary>
void Tank::AdaptVelocity()
{
	sf::Vector2f velocity = GetVelocity();

	//If they are moving diagonally divide by sqrt 2
	if (velocity.x != 0.f && velocity.y != 0.f)
	{
		SetVelocity(velocity / std::sqrt(2.f));
	}
}