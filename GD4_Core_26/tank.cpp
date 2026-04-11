#include "tank.hpp"
#include "texture_id.hpp"
#include "data_tables.hpp"
#include "utility.hpp"
#include "projectile.hpp"
#include "projectile_type.hpp"
#include <iostream>
#include "sound_node.hpp"
#include "network_node.hpp"
#include "tank_type.hpp"
#include "turret.hpp"

namespace
{
	const std::vector<TankData> Table = InitializeTankData();
}

TextureID ToTextureID(TankType type)
{
	return TextureID::kEntities;
}

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
	, m_spread_level(1)
	, m_is_firing(false)
	, m_is_launching_missile(false)
	, m_fire_countdown(sf::Time::Zero)
	, m_missile_ammo(2)
	, m_is_marked_for_removal(false)
	, m_spawned_pickup(false)
	, m_show_explosion(true)
	, m_explosion(textures.Get(TextureID::kExplosion))
	, m_explosion_began(false)
	, m_pickups_enabled(true)
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

	std::unique_ptr<Turret> turret;
	turret = std::unique_ptr<Turret>(new Turret(TurretType::kTurret, textures, m_colour));
	m_turret = turret.get();
	AttachChild(std::move(turret));
	m_turret->SetIdentifier(identifier);

	std::unique_ptr<TextNode> name_display(new TextNode(fonts, m_name));
	m_name_display = name_display.get();
	m_name_display->SetString(m_name);
	AttachChild(std::move(name_display));
	UpdateTexts();
}

uint8_t	Tank::GetIdentifier()
{
	return m_identifier;
}

/// <summary>
/// Returns the category used by the command system for filtering. (GPT)
/// </summary>
unsigned int Tank::GetCategory() const
{
	return static_cast<unsigned int>(ReceiverCategories::kTank);
}

void Tank::IncreaseFireRate()
{
	if (m_fire_rate < 3)
	{
		++m_fire_rate;
	}
}

void Tank::IncreaseFireSpread()
{
	if (m_spread_level < 3)
	{
		++m_spread_level;
	}
}

void Tank::AddPoints(int points)
{
	m_details->m_score = m_details->m_score + points;

	std::cout << "Score: " << m_details->m_score << "\n";
}

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

void Tank::UpdateMovementPattern(sf::Time dt)
{
	////Enemy AI
	//const std::vector<Direction>& directions = Table[static_cast<int>(m_type)].m_directions;
	//if (!directions.empty())
	//{
	//	//Move along the current direction for distance and then change direction
	//	if (m_distance_travelled > directions[m_directions_index].m_distance)
	//	{
	//		m_directions_index = (m_directions_index + 1) % directions.size();
	//		m_distance_travelled = 0;
	//	}

	//	//Compute the velocity
	//	//Add 90 to move down the screen, 0 degrees is to the right
	//	double radians = Utility::ToRadians(directions[m_directions_index].m_angle + 90.f);
	//	float vx = GetMaxSpeed() * std::cos(radians);
	//	float vy = GetMaxSpeed() * std::sin(radians);

	//	SetVelocity(sf::Vector2f(vx, vy));
	//	m_distance_travelled += GetMaxSpeed() * dt.asSeconds();
	//}
}

float Tank::GetMaxSpeed() const
{
	return 100.f;

	//Table[static_cast<int>(m_type)].m_speed;
}

void Tank::Fire()
{
	if (Table[static_cast<int>(m_type)].m_fire_interval != sf::Time::Zero)
	{
		m_is_firing = true;
	}
}

void Tank::CreateBullet(SceneNode& node, const TextureHolder& textures)
{
	// It may be an enemy bullet or an allied bullet.
	ProjectileType type = ProjectileType::kBullet;

	// How many bullets are fired based on the level accquired
	switch (m_spread_level)
	{
	case 1:
		CreateProjectile(node, type, 0.0f, 0.5f, textures);
		break;
	case 2:
		CreateProjectile(node, type, -0.5f, 0.5f, textures);
		CreateProjectile(node, type, 0.5f, 0.5f, textures);
		break;
	case 3:
		CreateProjectile(node, type, 0.0f, 0.5f, textures);
		CreateProjectile(node, type, -0.5f, 0.5f, textures);
		CreateProjectile(node, type, 0.5f, 0.5f, textures);
		break;
	}
}

/// <summary>
/// Modified: Ben Mc Keever D00254413
/// Now projectiles face the direction of the turret
/// Creates and launches a projectile in the direction of the turret. (GPT)
/// Modified: Kaylon Riordan D00255039
/// Changed projectiles rotation to check turret instead of tank
/// </summary>
void Tank::CreateProjectile(SceneNode& node, ProjectileType type, float x_offset, float y_offset, const TextureHolder& textures)
{
	// Claude - Create a projectile id
	// The top byte is the owner's id and the bottom is the shot number e.g client 1's third shot is 0x0103
	uint16_t projectile_id = (static_cast<uint16_t>(m_identifier) << 8) | (m_shot_counter & 0xFF);

	++m_shot_counter;

	std::unique_ptr<Projectile> projectile(new Projectile(type, textures, m_colour, this, projectile_id));

	//sf::Vector2f offset(x_offset * m_sprite.getGlobalBounds().size.x, y_offset * m_sprite.getGlobalBounds().size.y);

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

	// Claude - callback to inform the world of the projectile
	if (m_on_projectile_fired)
		m_on_projectile_fired(projectile.get());

	std::cout << "Just created projectile, GetIdentifier()=" << std::to_string(projectile->GetIdentifier())
		<< " projectile_id was=" << std::to_string(projectile_id) << std::endl;

	node.AttachChild(std::move(projectile));
}

sf::FloatRect Tank::GetBoundingRect() const
{
	return GetWorldTransform().transformRect(m_sprite.getGlobalBounds());
}

bool Tank::IsMarkedForRemoval() const
{
	return IsDestroyed() && (m_explosion.IsFinished() || !m_show_explosion);
}

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

			//Emit network game action for enemy explodes
			if (!IsAllied())
			{
				sf::Vector2f position = GetWorldPosition();

				Command command;
				command.category = static_cast<int>(ReceiverCategories::kNetwork);
				command.action = DerivedAction<NetworkNode>([position](NetworkNode& node, sf::Time)
					{
						node.NotifyGameAction(GameActions::kEnemyExplode, position);
					});

				commands.Push(command);
			}
			m_explosion_began = true;
		}
		return;
	}
	Entity::UpdateCurrent(dt, commands);
	UpdateMovementPattern(dt);
	//Check if bullets or missiles were fired
	CheckProjectileLaunch(dt, commands);
}

void Tank::CheckProjectileLaunch(sf::Time dt, CommandQueue & commands)
{
	if (!IsAllied())
	{
		Fire();
	}

	if (m_is_firing && m_fire_countdown <= sf::Time::Zero)
	{
		PlayLocalSound(commands, IsAllied() ? SoundEffect::kRedGunfire : SoundEffect::kRedGunfire);
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
/// Returns true if tank is allied (red tank). (GPT)
/// </summary>
bool Tank::IsAllied() const
{
	return m_type == TankType::kTank;
}

void Tank::Remove()
{
	Entity::Remove();
	m_show_explosion = false;
}
