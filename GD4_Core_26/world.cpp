#include "SocketWrapperPCH.hpp"
#include "world.hpp"
#include "sprite_node.hpp"
#include <SFML/System/Angle.hpp>
#include "Projectile.hpp"
#include "sound_node.hpp"
#include "particle_node.hpp"
#include "particle_type.hpp"

/// <summary>
/// Modified: Ben Mc Keever D00254413
/// Now stores red/blue information. Camera is centered independently of red_position
/// Modified: Kaylon Riordan D00255039
/// Updated logic to create a center variable which all player, camera and wall positionsa are based off
/// </summary>
World::World(sf::RenderTarget& output_target, FontHolder& font, SoundPlayer& sounds)
	: m_target(output_target)
	, m_camera(output_target.getDefaultView())
	, m_textures()
	, m_fonts(font)
	, m_sounds(sounds)
	, m_scene_graph(ReceiverCategories::kNone)
	, m_scene_layers()
	, m_world_bounds(sf::Vector2f(0.f, 0.f), sf::Vector2f(m_camera.getSize().x, 3000.f))
	, m_center(m_camera.getSize().x / 2.f, m_world_bounds.size.y - m_camera.getSize().y / 2.f)
	, m_red_position(m_center.x - 280, m_center.y + 120)
	, m_blue_position(m_center.x + 280, m_center.y - 120)
	// Set scroll speed to 0 as a temporary solution to removing scrolling
	, m_scroll_speed(0.f)
	, m_red_tank(nullptr)
	, m_blue_tank(nullptr)
{
	m_scene_texture.resize({ m_target.getSize().x, m_target.getSize().y });
	LoadTextures();
	BuildScene();
	m_camera.setCenter(m_center);
}

/// <summary>
/// Modified: Ben Mc Keever D00254413
/// Now adapts velocity of both players
/// </summary>
/// <param name="dt"></param>
void World::Update(sf::Time dt)
{
	m_red_tank->SetVelocity(0.f, 0.f);
	m_blue_tank->SetVelocity(0.f, 0.f);

	DestroyEntitiesOutsideView();

	//Process commands from the scenegraph
	while (!m_command_queue.IsEmpty())
	{
		m_scene_graph.OnCommand(m_command_queue.Pop(), dt);
	}

	m_red_tank->AdaptVelocity();
	m_blue_tank->AdaptVelocity();

	HandleCollisions();
	m_scene_graph.RemoveWrecks();

	m_scene_graph.Update(dt, m_command_queue);
}

void World::Draw()
{
	if (PostEffect::IsSupported())
	{
		m_scene_texture.clear();
		m_scene_texture.setView(m_camera);
		m_scene_texture.draw(m_scene_graph);
		m_scene_texture.display();
		m_bloom_effect.Apply(m_scene_texture, m_target);
	}
	else
	{
		m_target.setView(m_camera);
		m_target.draw(m_scene_graph);
	}
}

CommandQueue& World::GetCommandQueue()
{
	return m_command_queue;
}

/// <summary>
/// Modified: Ben Mc Keever D00254413
/// Now checks both players instead of just one (Copilot)
/// </summary>
/// <returns>Whether every player is alive or not</returns>
bool World::AllPlayersAlive() const
{
	// Check tanks still exist and if they do, whether they're marked for removal
	return (m_red_tank && !m_red_tank->IsMarkedForRemoval())
		&& (m_blue_tank && !m_blue_tank->IsMarkedForRemoval());
}

/// <summary>
/// Authored: Ben Mc Keever D00254413
/// Checks whether a specific player has died
/// </summary>
/// <param name="player">Joystick ID of a player</param>
/// <returns>Whether a given player has died</returns>
bool World::HasPlayerDied(int player) const
{
	if (player == 0)
		return m_red_tank->IsDestroyed();
	if (player == 1)
		return m_blue_tank->IsDestroyed();
}

void World::LoadTextures()
{
	m_textures.Load(TextureID::kEntities, "Media/Textures/SpriteSheet.png");
	m_textures.Load(TextureID::kExplosion, "Media/Textures/Explosion.png");
	m_textures.Load(TextureID::kLandscape, "Media/Textures/Background.png");
	m_textures.Load(TextureID::kParticle, "Media/Textures/Particle.png");
	m_textures.Load(TextureID::kExterior, "Media/Textures/Exterior.png");
}

void World::BuildScene()
{
	//Initialise the different layers
	for (int i = 0; i < static_cast<int>(SceneLayers::kLayerCount); i++)
	{
		ReceiverCategories category = (i == static_cast<int>(SceneLayers::kTanks)) ? ReceiverCategories::kScene : ReceiverCategories::kNone;
		SceneNode::Ptr layer(new SceneNode(category));
		m_scene_layers[i] = layer.get();
		m_scene_graph.AttachChild(std::move(layer));
	}

	//Prepare the background
	sf::Texture& texture = m_textures.Get(TextureID::kLandscape);
	sf::IntRect textureRect(m_world_bounds);
	texture.setRepeated(true);

	//Add the background sprite to the world
	std::unique_ptr<SpriteNode> background_sprite(new SpriteNode(texture, textureRect));
	background_sprite->setPosition(sf::Vector2f(m_world_bounds.position.x, m_world_bounds.position.y+120));
	m_scene_layers[static_cast<int>(SceneLayers::kBackground)]->AttachChild(std::move(background_sprite));

	//Add the walls
	AddWalls();

	// Red Tank
	std::unique_ptr<Tank> red(new Tank(TankType::kRedTank, m_textures, m_fonts));
	m_red_tank = red.get();
	m_red_tank->setPosition(m_red_position);
	m_red_tank->setRotation(sf::degrees(-90.f));
	m_red_tank->SetVelocity(40.f, 0.f);
	m_scene_layers[static_cast<int>(SceneLayers::kTanks)]->AttachChild(std::move(red));

	// Blue Tank
	std::unique_ptr<Tank> blue(new Tank(TankType::kBlueTank, m_textures, m_fonts));
	m_blue_tank = blue.get();
	m_blue_tank->setPosition(m_blue_position);
	m_blue_tank->setRotation(sf::degrees(90.f));
	m_blue_tank->SetVelocity(40.f, 0.f);
	m_scene_layers[static_cast<int>(SceneLayers::kTanks)]->AttachChild(std::move(blue));

	//Add sound effect node
	std::unique_ptr<SoundNode> soundNode(new SoundNode(m_sounds));
	m_scene_graph.AttachChild(std::move(soundNode));

	//Add the particle nodes to the scene
	std::unique_ptr<ParticleNode> smokeNode(new ParticleNode(ParticleType::kSmoke, m_textures));
	m_scene_layers[static_cast<int>(SceneLayers::kParticles)]->AttachChild(std::move(smokeNode));

	std::unique_ptr<ParticleNode> propellantNode(new ParticleNode(ParticleType::kPropellant, m_textures));
	m_scene_layers[static_cast<int>(SceneLayers::kParticles)]->AttachChild(std::move(propellantNode));
}

/// <summary>
/// Authored: Kaylon Riordan D00255039
/// Call the spawn wall function for each individual wall in the level
/// </summary>
void World::AddWalls()
{
	// Left side wall
	SpawnWall(WallType::kMetalWall, m_center.x - 340, m_center.y - 140, 90);
	SpawnWall(WallType::kMetalWall, m_center.x - 340, m_center.y - 84, 90);
	SpawnWall(WallType::kMetalWall, m_center.x - 340, m_center.y - 28, 90);
	SpawnWall(WallType::kMetalWall, m_center.x - 340, m_center.y + 28, 90);
	SpawnWall(WallType::kMetalWall, m_center.x - 326, m_center.y + 42, 0);
	SpawnWall(WallType::kWoodWall, m_center.x - 284, m_center.y + 42, 0);
	SpawnWall(WallType::kWoodWall, m_center.x - 256, m_center.y + 42, 0);
	SpawnWall(WallType::kWoodWall, m_center.x - 228, m_center.y + 42, 0);
	SpawnWall(WallType::kMetalWall, m_center.x - 186, m_center.y + 42, 0);
	SpawnWall(WallType::kMetalWall, m_center.x - 172, m_center.y + 84, 90);
	SpawnWall(WallType::kMetalWall, m_center.x - 172, m_center.y + 140, 90);
	SpawnWall(WallType::kMetalWall, m_center.x - 130, m_center.y + 154, 0);
	SpawnWall(WallType::kWoodWall, m_center.x - 88, m_center.y + 154, 0);
	SpawnWall(WallType::kMetalWall, m_center.x - 46, m_center.y + 154, 0);
	SpawnWall(WallType::kWoodWall, m_center.x - 4, m_center.y + 154, 0);
	SpawnWall(WallType::kMetalWall, m_center.x + 38, m_center.y + 154, 0);
	SpawnWall(WallType::kWoodWall, m_center.x + 80, m_center.y + 154, 0);
	SpawnWall(WallType::kMetalWall, m_center.x + 122, m_center.y + 154, 0);

	// Right side wall
	SpawnWall(WallType::kMetalWall, m_center.x + 340, m_center.y + 140, 90);
	SpawnWall(WallType::kMetalWall, m_center.x + 340, m_center.y + 84, 90);
	SpawnWall(WallType::kMetalWall, m_center.x + 340, m_center.y + 28, 90);
	SpawnWall(WallType::kMetalWall, m_center.x + 340, m_center.y - 28, 90);
	SpawnWall(WallType::kMetalWall, m_center.x + 326, m_center.y - 42, 0);
	SpawnWall(WallType::kWoodWall, m_center.x + 284, m_center.y - 42, 0);
	SpawnWall(WallType::kWoodWall, m_center.x + 256, m_center.y - 42, 0);
	SpawnWall(WallType::kWoodWall, m_center.x + 228, m_center.y - 42, 0);
	SpawnWall(WallType::kMetalWall, m_center.x + 186, m_center.y - 42, 0);
	SpawnWall(WallType::kMetalWall, m_center.x + 172, m_center.y - 84, 90);
	SpawnWall(WallType::kMetalWall, m_center.x + 172, m_center.y - 140, 90);
	SpawnWall(WallType::kMetalWall, m_center.x + 130, m_center.y - 154, 0);
	SpawnWall(WallType::kWoodWall, m_center.x + 88, m_center.y - 154, 0);
	SpawnWall(WallType::kMetalWall, m_center.x + 46, m_center.y - 154, 0);
	SpawnWall(WallType::kWoodWall, m_center.x + 4, m_center.y - 154, 0);
	SpawnWall(WallType::kMetalWall, m_center.x - 38, m_center.y - 154, 0);
	SpawnWall(WallType::kWoodWall, m_center.x - 80, m_center.y - 154, 0);
	SpawnWall(WallType::kMetalWall, m_center.x - 122, m_center.y - 154, 0);

	// Add Exterior Walls
	SpawnWall(WallType::kExterior, m_center.x + 526, m_center.y, 90);
	SpawnWall(WallType::kExterior, m_center.x - 526, m_center.y, 90);
	SpawnWall(WallType::kExterior, m_center.x, m_center.y + 302, 0);
	SpawnWall(WallType::kExterior, m_center.x, m_center.y - 302, 0);
}

/// <summary>
/// Authored: Kaylon Riordan D00255039
/// Spawn a wall, on the wall layer, based on specified type, position quordinates, and rotation given in degrees
/// </summary>
void World::SpawnWall(WallType type, float x, float y, float rotation)
{
	std::unique_ptr<Wall> wall(new Wall(type, m_textures));
	wall->setPosition(sf::Vector2f(x, y));
	wall->setRotation(sf::degrees(rotation));
	m_scene_layers[static_cast<int>(SceneLayers::kWalls)]->AttachChild(std::move(wall));
}

sf::FloatRect World::GetViewBounds() const
{
	return sf::FloatRect(m_camera.getCenter() - m_camera.getSize() / 2.f, m_camera.getSize());;
}

sf::FloatRect World::GetBattleFieldBounds() const
{
	//Return camera bounds + a small area off screen where the enemies spawn
	sf::FloatRect bounds = GetViewBounds();
	bounds.position.y -= 100.f;
	bounds.size.y += 100.f;
	return bounds;
}

bool MatchesCategories(SceneNode::Pair& colliders, ReceiverCategories type1, ReceiverCategories type2)
{
	unsigned int category1 = colliders.first->GetCategory();
	unsigned int category2 = colliders.second->GetCategory();

	if ((static_cast<int>(type1) & category1) && (static_cast<int>(type2) & category2))
	{
		return true;
	}
	else if ((static_cast<int>(type1) & category2) && (static_cast<int>(type2) & category1))
	{
		std::swap(colliders.first, colliders.second);
		return true;
	}
	else
	{
		return false;
	}

}

/// <summary>
/// Modified: Kaylon Riordan D00255039
/// </summary>
void World::HandleCollisions()
{
	std::set<SceneNode::Pair> collision_pairs;
	m_scene_graph.CheckSceneCollision(m_scene_graph, collision_pairs);

	for (SceneNode::Pair pair : collision_pairs)
	{
		// Handle collision for 2 tanks
		// destroy both tnaks on collision
		if (MatchesCategories(pair, ReceiverCategories::kRedTank, ReceiverCategories::kBlueTank))
		{
			auto& red = static_cast<Tank&>(*pair.first);
			auto& blue = static_cast<Tank&>(*pair.second);

			red.Destroy();
			blue.Destroy();
		}
		// Handle collision for enemy projectiles
		// Damage the player based on the projectiles damage value
		else if (MatchesCategories(pair, ReceiverCategories::kRedTank, ReceiverCategories::kBlueProjectile) || MatchesCategories(pair, ReceiverCategories::kBlueTank, ReceiverCategories::kRedProjectile))
		{
			auto& tank = static_cast<Tank&>(*pair.first);
			auto& projectile = static_cast<Projectile&>(*pair.second);
			// Collision response
			tank.Damage(projectile.GetDamage());
			projectile.Destroy();
		}
		// Handle collision for friendly projectiles
		// Only deal damage if the bullet ahs already bounced once, stops bullet imediately killing player as it can spawn inside the players bounding box depending on rotation
		else if (MatchesCategories(pair, ReceiverCategories::kRedTank, ReceiverCategories::kRedProjectile) || MatchesCategories(pair, ReceiverCategories::kBlueTank, ReceiverCategories::kBlueProjectile))
		{
			auto& tank = static_cast<Tank&>(*pair.first);
			auto& projectile = static_cast<Projectile&>(*pair.second);
			if (projectile.GetBounces() > 0)
			{
				// Collision response
				tank.Damage(projectile.GetDamage());
				projectile.Destroy();
			}
		}

		// Handle player/wall collisions
		else if (MatchesCategories(pair, ReceiverCategories::kTank, ReceiverCategories::kWall))
		{
			auto& tank = static_cast<Tank&>(*pair.first);
			auto& wall = static_cast<Wall&>(*pair.second);

			// Store position before changing it
			sf::Vector2f currentPosition = tank.getPosition();

			// Check if collision is caused by the rotation, if so revert to previous rotation
			tank.setPosition(sf::Vector2f(tank.GetPreviousPosition().x, tank.GetPreviousPosition().y));
			if (Collision(tank, wall))
			{
				tank.setRotation(tank.GetPreviousRotation());
			}

			// Check if collision is caused by the x axis, if so revert to previous x position
			tank.setPosition(sf::Vector2f(currentPosition.x, tank.GetPreviousPosition().y));
			if (Collision(tank, wall))
			{
				currentPosition = sf::Vector2f(tank.GetPreviousPosition().x, currentPosition.y);
			}

			// Check if collision is caused by the y axis, if so revert to previous y position
			tank.setPosition(sf::Vector2f(tank.GetPreviousPosition().x, currentPosition.y));
			if (Collision(tank, wall))
			{
				currentPosition = sf::Vector2f(currentPosition.x, tank.GetPreviousPosition().y);
			}

			// Update position to one with no wall collision
			tank.setPosition(sf::Vector2f(currentPosition.x, currentPosition.y));
		}
		// Handle projectile/wall collisions
		else if (MatchesCategories(pair, ReceiverCategories::kProjectile, ReceiverCategories::kWall))
		{
			auto& projectile = static_cast<Projectile&>(*pair.first);
			auto& wall = static_cast<Wall&>(*pair.second);
			if (projectile.CanBounce())
			{
				bool corner = true;
				projectile.AddBounce();

				// Collision response
				sf::Vector2f currentPosition = projectile.getPosition();

				// If collision is was on the x axis, invert the x velocity to bounce projectile
				projectile.setPosition(sf::Vector2f(currentPosition.x, projectile.GetPreviousPosition().y));
				if (Collision(projectile, wall))
				{
					currentPosition = sf::Vector2f(projectile.GetPreviousPosition().x, currentPosition.y);
					projectile.SetVelocity(-projectile.GetVelocity().x, projectile.GetVelocity().y);
					corner = false;
				}

				// If collision is was on the y axis, invert the y velocity to bounce projectile
				projectile.setPosition(sf::Vector2f(projectile.GetPreviousPosition().x, currentPosition.y));
				if (Collision(projectile, wall))
				{
					currentPosition = sf::Vector2f(currentPosition.x, projectile.GetPreviousPosition().y);
					projectile.SetVelocity(projectile.GetVelocity().x, -projectile.GetVelocity().y);
					corner = false;
				}

				// If neither x or y alone are triggered, then the bullet hit perfectly on the corner of a wall, invert both velocities
				if (corner)
				{
					currentPosition = sf::Vector2f(projectile.GetPreviousPosition().x, projectile.GetPreviousPosition().y);
					projectile.SetVelocity(-projectile.GetVelocity().x, -projectile.GetVelocity().y);
				}

				// Rotate projectile to face the direction they are moving after bounce
				projectile.setRotation(sf::degrees((std::atan2(projectile.GetVelocity().y, projectile.GetVelocity().x) * 180 / 3.14159265f) + 90));
				projectile.setPosition(sf::Vector2f(currentPosition.x, currentPosition.y));

				// If the wall is wooden, break it after bounce
				if (wall.GetCategory() == static_cast<unsigned int>(ReceiverCategories::kWoodWall))
				{
					wall.Damage(projectile.GetDamage());
				}
			}
		}
	}
}

void World::DestroyEntitiesOutsideView()
{
	Command command;
	command.category = static_cast<int>(ReceiverCategories::kProjectile);
	command.action = DerivedAction<Entity>([this](Entity& e, sf::Time dt)
	{
		//Does the object intersect with the battlefield
		if (GetBattleFieldBounds().findIntersection(e.GetBoundingRect()) == std::nullopt)
		{
			e.Destroy();
		}
	});
	m_command_queue.Push(command);

}

void World::UpdateSounds()
{
	sf::Vector2f listener_position;

	listener_position = m_camera.getCenter();
	m_sounds.SetListenerPosition(listener_position);
	m_sounds.RemoveStoppedSounds();
}