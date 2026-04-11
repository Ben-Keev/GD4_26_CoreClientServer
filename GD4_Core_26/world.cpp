#include "SocketWrapperPCH.hpp"
#include "world.hpp"
#include "sprite_node.hpp"
#include <iostream>
#include "state.hpp"
#include <SFML/System/Angle.hpp>
#include "Projectile.hpp"
#include "particle_node.hpp"
#include "particletype.hpp"
#include "sound_node.hpp"
#include "data_tables.hpp"

namespace
{
	const std::vector<sf::Color> TankColours = InitializeTankColours();
}

World::World(sf::RenderTarget& output_target, FontHolder& font, SoundPlayer& sounds, bool networked)
	: m_target(output_target)
	, m_camera(output_target.getDefaultView())
	, m_textures()
	, m_fonts(font)
	, m_sounds(sounds)
	, m_scene_graph(ReceiverCategories::kNone)
	, m_scene_layers()
	, m_world_bounds(sf::Vector2f(0.f, 0.f), sf::Vector2f(m_camera.getSize().x, m_camera.getSize().y))
	, m_center(m_camera.getSize().x / 2.f, m_world_bounds.size.y - m_camera.getSize().y / 2.f)
	, m_scroll_speed(0)
	, m_scrollspeed_compensation(1.f)
	, m_player_tank()
	, m_enemy_spawn_points()
	, m_active_enemies()
	, m_networked_world(networked)
	, m_network_node(nullptr)
	, m_finish_sprite(nullptr)
{
	m_scene_texture.resize({ m_target.getSize().x, m_target.getSize().y });
	LoadTextures();
	BuildScene();
	m_camera.setCenter(m_center);
}

void World::Update(sf::Time dt)
{
	for (Tank* t : m_player_tank)
	{
		t->SetVelocity(0.f, 0.f);
	}

	DestroyEntitiesOutsideView();

	//Process commands from the scenegraph
	while (!m_command_queue.IsEmpty())
	{
		m_scene_graph.OnCommand(m_command_queue.Pop(), dt);
	}

	AdaptPlayerVelocity();

	HandleCollisions();

	auto first_to_remove = std::remove_if(m_player_tank.begin(), m_player_tank.end(), std::mem_fn(&Tank::IsMarkedForRemoval));
	m_player_tank.erase(first_to_remove, m_player_tank.end());
	m_scene_graph.RemoveWrecks();

	SpawnEnemies();

	m_scene_graph.Update(dt, m_command_queue);
	AdaptPlayerPosition();
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

Tank* World::GetAircraft(int identifier) const
{
	for (Tank* t : m_player_tank)
	{
		if (t->GetIdentifier() == identifier)
		{
			return t;
		}
	}
	return nullptr;
}

void World::RemoveAircraft(uint8_t identifier)
{
	Tank* tank = GetAircraft(identifier);
	if (tank)
	{
		tank->Destroy();
		//m_player_tank.erase(std::find(m_player_tank.begin(), m_player_tank.end(), tank));
	}
}

Tank* World::AddAircraft(uint8_t identifier, PlayerDetails* details, sf::Vector2f position)
{
	std::unique_ptr<Tank> tank(new Tank(TankType::kTank, m_textures, m_fonts, identifier, details));

	tank->setPosition(position);

	// std::cout << "Spawning player at position " << player->getPosition().x << ", " << player->getPosition().y << std::endl;
	std::cout << "World::AddTank " << +identifier << std::endl;

	m_player_tank.emplace_back(tank.get());
	m_scene_layers[static_cast<int>(SceneLayers::kTanks)]->AttachChild(std::move(tank));
	return m_player_tank.back();
}

bool World::PollGameAction(GameActions::Action& out)
{
	return m_network_node->PollGameAction(out);
}

void World::SetCurrentBattleFieldPosition(float lineY)
{
	m_camera.setCenter(sf::Vector2f(m_camera.getCenter().x, lineY - m_camera.getSize().y / 2));
	m_center.y = m_world_bounds.size.y;
}

void World::SetWorldHeight(float height)
{
	m_world_bounds.size.y = height;
}

CommandQueue& World::GetCommandQueue()
{
	return m_command_queue;
}

bool World::HasAlivePlayer() const
{
	return !m_player_tank.empty();
}

bool World::HasPlayerReachedEnd() const
{
	if (Tank* tank = GetAircraft(1))
	{
		return !m_world_bounds.contains(tank->getPosition());
	}
	return false;
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
	sf::IntRect texture_rect(m_world_bounds);
	texture.setRepeated(true);

	float view_height = m_camera.getSize().y;
	texture_rect.size.y += static_cast<int>(view_height);

	//Add the background sprite to the world
	std::unique_ptr<SpriteNode> background_sprite(new SpriteNode(texture, texture_rect));
	background_sprite->setPosition(sf::Vector2f(m_world_bounds.position.x, m_world_bounds.position.y - view_height));
	m_scene_layers[static_cast<int>(SceneLayers::kBackground)]->AttachChild(std::move(background_sprite));

	AddWalls();

	//Add sound effect node
	std::unique_ptr<SoundNode> soundNode(new SoundNode(m_sounds));
	m_scene_graph.AttachChild(std::move(soundNode));

	if (m_networked_world)
	{
		std::unique_ptr<NetworkNode> network_node(new NetworkNode());
		m_network_node = network_node.get();
		m_scene_graph.AttachChild(std::move(network_node));
	}
	AddEnemies();

	//Add the particle nodes to the scene
	std::unique_ptr<ParticleNode> smokeNode(new ParticleNode(ParticleType::kSmoke, m_textures));
	m_scene_layers[static_cast<int>(SceneLayers::kParticles)]->AttachChild(std::move(smokeNode));

	std::unique_ptr<ParticleNode> propellantNode(new ParticleNode(ParticleType::kPropellant, m_textures));
	m_scene_layers[static_cast<int>(SceneLayers::kParticles)]->AttachChild(std::move(propellantNode));
}

void World::AdaptPlayerVelocity()
{
	for (Tank* tank : m_player_tank)
	{
		sf::Vector2f velocity = tank->GetVelocity();

		//If they are moving diagonally divide by sqrt 2
		if (velocity.x != 0.f && velocity.y != 0.f)
		{
			tank->SetVelocity(velocity / std::sqrt(2.f));
		}
		//Add scrolling velocity
		tank->Accelerate(0.f, m_scroll_speed);
	}
}

void World::AdaptPlayerPosition()
{
	//keep player on the screen
	sf::FloatRect view_bounds = GetViewBounds();
	const float border_distance = 40.f;

	for (Tank* tank : m_player_tank)
	{
		sf::Vector2f position = tank->getPosition();
		position.x = std::min(position.x, view_bounds.position.x + view_bounds.size.x - border_distance);
		position.x = std::max(position.x, view_bounds.position.x + border_distance);
		position.y = std::min(position.y, view_bounds.position.y + view_bounds.size.y - border_distance);
		position.y = std::max(position.y, view_bounds.position.y + border_distance);
		tank->setPosition(position);
	}
}

void World::SpawnEnemies() 
{
    return;
}
	 
/// <summary>
/// Authored: Kaylon Riordan D00255039
/// Call the spawn wall function for each individual wall in the level
/// </summary>
void World::AddWalls()
{
	//Top Left Quadrant
	SpawnWall(WallType::kWoodWall, m_center.x - 14, m_center.y - 154, 0);
	SpawnWall(WallType::kWoodWall, m_center.x - 42, m_center.y - 154, 0);
	SpawnWall(WallType::kMetalWall, m_center.x - 70, m_center.y - 196, 90);
	SpawnWall(WallType::kMetalWall, m_center.x - 70, m_center.y - 280, 90);
	SpawnWall(WallType::kWoodWall, m_center.x - 70, m_center.y - 350, 0);
	SpawnWall(WallType::kWoodWall, m_center.x - 70, m_center.y - 378, 0);
	SpawnWall(WallType::kWoodWall, m_center.x - 70, m_center.y - 406, 0);
	SpawnWall(WallType::kWoodWall, m_center.x - 70, m_center.y - 434, 0);
	SpawnWall(WallType::kWoodWall, m_center.x - 98, m_center.y - 154, 0);
	SpawnWall(WallType::kWoodWall, m_center.x - 126, m_center.y - 154, 0);
	SpawnWall(WallType::kWoodWall, m_center.x - 154, m_center.y - 154, 0);
	SpawnWall(WallType::kWoodWall, m_center.x - 182, m_center.y - 154, 0);
	SpawnWall(WallType::kMetalWall, m_center.x - 210, m_center.y - 112, 90);
	SpawnWall(WallType::kWoodWall, m_center.x - 210, m_center.y - 14, 0);
	SpawnWall(WallType::kWoodWall, m_center.x - 210, m_center.y - 42, 0);
	SpawnWall(WallType::kWoodWall, m_center.x - 238, m_center.y - 70, 0);
	SpawnWall(WallType::kMetalWall, m_center.x - 308, m_center.y - 70, 0);
	SpawnWall(WallType::kWoodWall, m_center.x - 378, m_center.y - 70, 0);
	SpawnWall(WallType::kMetalWall, m_center.x - 448, m_center.y - 70, 0);
	SpawnWall(WallType::kWoodWall, m_center.x - 518, m_center.y - 70, 0);
	SpawnWall(WallType::kMetalWall, m_center.x - 588, m_center.y - 70, 0);
	SpawnWall(WallType::kWoodWall, m_center.x - 658, m_center.y - 70, 0);
	SpawnWall(WallType::kWoodWall, m_center.x - 686, m_center.y - 70, 0);
	SpawnWall(WallType::kWoodWall, m_center.x - 714, m_center.y - 70, 0);
	SpawnWall(WallType::kWoodWall, m_center.x - 742, m_center.y - 70, 0);
	SpawnWall(WallType::kMetalWall, m_center.x - 770, m_center.y - 112, 90);
	SpawnWall(WallType::kWoodWall, m_center.x - 770, m_center.y - 42, 0);
	SpawnWall(WallType::kWoodWall, m_center.x - 770, m_center.y - 14, 0);
	SpawnWall(WallType::kWoodWall, m_center.x - 798, m_center.y - 154, 0);
	SpawnWall(WallType::kWoodWall, m_center.x - 826, m_center.y - 154, 0);
	SpawnWall(WallType::kWoodWall, m_center.x - 854, m_center.y - 154, 0);
	SpawnWall(WallType::kWoodWall, m_center.x - 882, m_center.y - 154, 0);
	SpawnWall(WallType::kMetalWall, m_center.x - 770, m_center.y - 196, 90);
	SpawnWall(WallType::kWoodWall, m_center.x - 658, m_center.y - 238, 0);
	SpawnWall(WallType::kWoodWall, m_center.x - 686, m_center.y - 238, 0);
	SpawnWall(WallType::kWoodWall, m_center.x - 714, m_center.y - 238, 0);
	SpawnWall(WallType::kWoodWall, m_center.x - 742, m_center.y - 238, 0);
	SpawnWall(WallType::kMetalWall, m_center.x - 630, m_center.y - 280, 90);
	SpawnWall(WallType::kWoodWall, m_center.x - 630, m_center.y - 350, 0);
	SpawnWall(WallType::kWoodWall, m_center.x - 630, m_center.y - 378, 0);
	SpawnWall(WallType::kWoodWall, m_center.x - 630, m_center.y - 406, 0);
	SpawnWall(WallType::kWoodWall, m_center.x - 630, m_center.y - 434, 0);
	SpawnWall(WallType::kWoodWall, m_center.x - 602, m_center.y - 322, 0);
	SpawnWall(WallType::kMetalWall, m_center.x - 532, m_center.y - 322, 0);
	SpawnWall(WallType::kWoodWall, m_center.x - 462, m_center.y - 322, 0);
	SpawnWall(WallType::kMetalWall, m_center.x - 392, m_center.y - 322, 0);
	SpawnWall(WallType::kWoodWall, m_center.x - 322, m_center.y - 322, 0);
	SpawnWall(WallType::kMetalWall, m_center.x - 252, m_center.y - 322, 0);
	SpawnWall(WallType::kWoodWall, m_center.x - 98, m_center.y - 322, 0);
	SpawnWall(WallType::kWoodWall, m_center.x - 126, m_center.y - 322, 0);
	SpawnWall(WallType::kWoodWall, m_center.x - 154, m_center.y - 322, 0);
	SpawnWall(WallType::kWoodWall, m_center.x - 182, m_center.y - 322, 0);

	//Top Right Quadrant
	SpawnWall(WallType::kWoodWall, m_center.x + 14, m_center.y - 154, 0);
	SpawnWall(WallType::kWoodWall, m_center.x + 42, m_center.y - 154, 0);
	SpawnWall(WallType::kMetalWall, m_center.x + 70, m_center.y - 196, 90);
	SpawnWall(WallType::kMetalWall, m_center.x + 70, m_center.y - 280, 90);
	SpawnWall(WallType::kWoodWall, m_center.x + 70, m_center.y - 350, 0);
	SpawnWall(WallType::kWoodWall, m_center.x + 70, m_center.y - 378, 0);
	SpawnWall(WallType::kWoodWall, m_center.x + 70, m_center.y - 406, 0);
	SpawnWall(WallType::kWoodWall, m_center.x + 70, m_center.y - 434, 0);
	SpawnWall(WallType::kWoodWall, m_center.x + 98, m_center.y - 154, 0);
	SpawnWall(WallType::kWoodWall, m_center.x + 126, m_center.y - 154, 0);
	SpawnWall(WallType::kWoodWall, m_center.x + 154, m_center.y - 154, 0);
	SpawnWall(WallType::kWoodWall, m_center.x + 182, m_center.y - 154, 0);
	SpawnWall(WallType::kMetalWall, m_center.x + 210, m_center.y - 112, 90);
	SpawnWall(WallType::kWoodWall, m_center.x + 210, m_center.y - 14, 0);
	SpawnWall(WallType::kWoodWall, m_center.x + 210, m_center.y - 42, 0);
	SpawnWall(WallType::kWoodWall, m_center.x + 238, m_center.y - 70, 0);
	SpawnWall(WallType::kMetalWall, m_center.x + 308, m_center.y - 70, 0);
	SpawnWall(WallType::kWoodWall, m_center.x + 378, m_center.y - 70, 0);
	SpawnWall(WallType::kMetalWall, m_center.x + 448, m_center.y - 70, 0);
	SpawnWall(WallType::kWoodWall, m_center.x + 518, m_center.y - 70, 0);
	SpawnWall(WallType::kMetalWall, m_center.x + 588, m_center.y - 70, 0);
	SpawnWall(WallType::kWoodWall, m_center.x + 658, m_center.y - 70, 0);
	SpawnWall(WallType::kWoodWall, m_center.x + 686, m_center.y - 70, 0);
	SpawnWall(WallType::kWoodWall, m_center.x + 714, m_center.y - 70, 0);
	SpawnWall(WallType::kWoodWall, m_center.x + 742, m_center.y - 70, 0);
	SpawnWall(WallType::kMetalWall, m_center.x + 770, m_center.y - 112, 90);
	SpawnWall(WallType::kWoodWall, m_center.x + 770, m_center.y - 42, 0);
	SpawnWall(WallType::kWoodWall, m_center.x + 770, m_center.y - 14, 0);
	SpawnWall(WallType::kWoodWall, m_center.x + 798, m_center.y - 154, 0);
	SpawnWall(WallType::kWoodWall, m_center.x + 826, m_center.y - 154, 0);
	SpawnWall(WallType::kWoodWall, m_center.x + 854, m_center.y - 154, 0);
	SpawnWall(WallType::kWoodWall, m_center.x + 882, m_center.y - 154, 0);
	SpawnWall(WallType::kMetalWall, m_center.x + 770, m_center.y - 196, 90);
	SpawnWall(WallType::kWoodWall, m_center.x + 658, m_center.y - 238, 0);
	SpawnWall(WallType::kWoodWall, m_center.x + 686, m_center.y - 238, 0);
	SpawnWall(WallType::kWoodWall, m_center.x + 714, m_center.y - 238, 0);
	SpawnWall(WallType::kWoodWall, m_center.x + 742, m_center.y - 238, 0);
	SpawnWall(WallType::kMetalWall, m_center.x + 630, m_center.y - 280, 90);
	SpawnWall(WallType::kWoodWall, m_center.x + 630, m_center.y - 350, 0);
	SpawnWall(WallType::kWoodWall, m_center.x + 630, m_center.y - 378, 0);
	SpawnWall(WallType::kWoodWall, m_center.x + 630, m_center.y - 406, 0);
	SpawnWall(WallType::kWoodWall, m_center.x + 630, m_center.y - 434, 0);
	SpawnWall(WallType::kWoodWall, m_center.x + 602, m_center.y - 322, 0);
	SpawnWall(WallType::kMetalWall, m_center.x + 532, m_center.y - 322, 0);
	SpawnWall(WallType::kWoodWall, m_center.x + 462, m_center.y - 322, 0);
	SpawnWall(WallType::kMetalWall, m_center.x + 392, m_center.y - 322, 0);
	SpawnWall(WallType::kWoodWall, m_center.x + 322, m_center.y - 322, 0);
	SpawnWall(WallType::kMetalWall, m_center.x + 252, m_center.y - 322, 0);
	SpawnWall(WallType::kWoodWall, m_center.x + 98, m_center.y - 322, 0);
	SpawnWall(WallType::kWoodWall, m_center.x + 126, m_center.y - 322, 0);
	SpawnWall(WallType::kWoodWall, m_center.x + 154, m_center.y - 322, 0);
	SpawnWall(WallType::kWoodWall, m_center.x + 182, m_center.y - 322, 0);

	//Bottom Left Quadrant
	SpawnWall(WallType::kWoodWall, m_center.x - 14, m_center.y + 154, 0);
	SpawnWall(WallType::kWoodWall, m_center.x - 42, m_center.y + 154, 0);
    SpawnWall(WallType::kMetalWall, m_center.x - 70, m_center.y + 196, 90);
	SpawnWall(WallType::kMetalWall, m_center.x - 70, m_center.y + 280, 90);
	SpawnWall(WallType::kWoodWall, m_center.x - 70, m_center.y + 350, 0);
	SpawnWall(WallType::kWoodWall, m_center.x - 70, m_center.y + 378, 0);
	SpawnWall(WallType::kWoodWall, m_center.x - 70, m_center.y + 406, 0);
	SpawnWall(WallType::kWoodWall, m_center.x - 70, m_center.y + 434, 0);
	SpawnWall(WallType::kWoodWall, m_center.x - 98, m_center.y + 154, 0);
	SpawnWall(WallType::kWoodWall, m_center.x - 126, m_center.y + 154, 0);
	SpawnWall(WallType::kWoodWall, m_center.x - 154, m_center.y + 154, 0);
	SpawnWall(WallType::kWoodWall, m_center.x - 182, m_center.y + 154, 0);
	SpawnWall(WallType::kMetalWall, m_center.x - 210, m_center.y + 112, 90);
	SpawnWall(WallType::kWoodWall, m_center.x - 210, m_center.y + 14, 0);
	SpawnWall(WallType::kWoodWall, m_center.x - 210, m_center.y + 42, 0);
	SpawnWall(WallType::kWoodWall, m_center.x - 238, m_center.y + 70, 0);
	SpawnWall(WallType::kMetalWall, m_center.x - 308, m_center.y + 70, 0);
	SpawnWall(WallType::kWoodWall, m_center.x - 378, m_center.y + 70, 0);
	SpawnWall(WallType::kMetalWall, m_center.x - 448, m_center.y + 70, 0);
	SpawnWall(WallType::kWoodWall, m_center.x - 518, m_center.y + 70, 0);
	SpawnWall(WallType::kMetalWall, m_center.x - 588, m_center.y + 70, 0);
	SpawnWall(WallType::kWoodWall, m_center.x - 658, m_center.y + 70, 0);
	SpawnWall(WallType::kWoodWall, m_center.x - 686, m_center.y + 70, 0);
	SpawnWall(WallType::kWoodWall, m_center.x - 714, m_center.y + 70, 0);
	SpawnWall(WallType::kWoodWall, m_center.x - 742, m_center.y + 70, 0);
	SpawnWall(WallType::kMetalWall, m_center.x - 770, m_center.y + 112, 90);
	SpawnWall(WallType::kWoodWall, m_center.x - 770, m_center.y + 42, 0);
	SpawnWall(WallType::kWoodWall, m_center.x - 770, m_center.y + 14, 0);
	SpawnWall(WallType::kWoodWall, m_center.x - 798, m_center.y + 154, 0);
	SpawnWall(WallType::kWoodWall, m_center.x - 826, m_center.y + 154, 0);
	SpawnWall(WallType::kWoodWall, m_center.x - 854, m_center.y + 154, 0);
	SpawnWall(WallType::kWoodWall, m_center.x - 882, m_center.y + 154, 0);
	SpawnWall(WallType::kMetalWall, m_center.x - 770, m_center.y + 196, 90);
	SpawnWall(WallType::kWoodWall, m_center.x - 658, m_center.y + 238, 0);
	SpawnWall(WallType::kWoodWall, m_center.x - 686, m_center.y + 238, 0);
	SpawnWall(WallType::kWoodWall, m_center.x - 714, m_center.y + 238, 0);
	SpawnWall(WallType::kWoodWall, m_center.x - 742, m_center.y + 238, 0);
	SpawnWall(WallType::kMetalWall, m_center.x - 630, m_center.y + 280, 90);
	SpawnWall(WallType::kWoodWall, m_center.x - 630, m_center.y + 350, 0);
	SpawnWall(WallType::kWoodWall, m_center.x - 630, m_center.y + 378, 0);
	SpawnWall(WallType::kWoodWall, m_center.x - 630, m_center.y + 406, 0);
	SpawnWall(WallType::kWoodWall, m_center.x - 630, m_center.y + 434, 0);
	SpawnWall(WallType::kWoodWall, m_center.x - 602, m_center.y + 322, 0);
	SpawnWall(WallType::kMetalWall, m_center.x - 532, m_center.y + 322, 0);
	SpawnWall(WallType::kWoodWall, m_center.x - 462, m_center.y + 322, 0);
	SpawnWall(WallType::kMetalWall, m_center.x - 392, m_center.y + 322, 0);
	SpawnWall(WallType::kWoodWall, m_center.x - 322, m_center.y + 322, 0);
	SpawnWall(WallType::kMetalWall, m_center.x - 252, m_center.y + 322, 0);
	SpawnWall(WallType::kWoodWall, m_center.x - 98, m_center.y + 322, 0);
	SpawnWall(WallType::kWoodWall, m_center.x - 126, m_center.y + 322, 0);
	SpawnWall(WallType::kWoodWall, m_center.x - 154, m_center.y + 322, 0);
	SpawnWall(WallType::kWoodWall, m_center.x - 182, m_center.y + 322, 0);

	//Bottom Right Quadrant
	SpawnWall(WallType::kWoodWall, m_center.x + 14, m_center.y + 154, 0);
	SpawnWall(WallType::kWoodWall, m_center.x + 42, m_center.y + 154, 0);
	SpawnWall(WallType::kMetalWall, m_center.x + 70, m_center.y + 196, 90);
	SpawnWall(WallType::kMetalWall, m_center.x + 70, m_center.y + 280, 90);
	SpawnWall(WallType::kWoodWall, m_center.x + 70, m_center.y + 350, 0);
	SpawnWall(WallType::kWoodWall, m_center.x + 70, m_center.y + 378, 0);
	SpawnWall(WallType::kWoodWall, m_center.x + 70, m_center.y + 406, 0);
	SpawnWall(WallType::kWoodWall, m_center.x + 70, m_center.y + 434, 0);
	SpawnWall(WallType::kWoodWall, m_center.x + 98, m_center.y + 154, 0);
	SpawnWall(WallType::kWoodWall, m_center.x + 126, m_center.y + 154, 0);
	SpawnWall(WallType::kWoodWall, m_center.x + 154, m_center.y + 154, 0);
	SpawnWall(WallType::kWoodWall, m_center.x + 182, m_center.y + 154, 0);
	SpawnWall(WallType::kMetalWall, m_center.x + 210, m_center.y + 112, 90);
	SpawnWall(WallType::kWoodWall, m_center.x + 210, m_center.y + 14, 0);
	SpawnWall(WallType::kWoodWall, m_center.x + 210, m_center.y + 42, 0);
	SpawnWall(WallType::kWoodWall, m_center.x + 238, m_center.y + 70, 0);
	SpawnWall(WallType::kMetalWall, m_center.x + 308, m_center.y + 70, 0);
	SpawnWall(WallType::kWoodWall, m_center.x + 378, m_center.y + 70, 0);
	SpawnWall(WallType::kMetalWall, m_center.x + 448, m_center.y + 70, 0);
	SpawnWall(WallType::kWoodWall, m_center.x + 518, m_center.y + 70, 0);
	SpawnWall(WallType::kMetalWall, m_center.x + 588, m_center.y + 70, 0);
	SpawnWall(WallType::kWoodWall, m_center.x + 658, m_center.y + 70, 0);
	SpawnWall(WallType::kWoodWall, m_center.x + 686, m_center.y + 70, 0);
	SpawnWall(WallType::kWoodWall, m_center.x + 714, m_center.y + 70, 0);
	SpawnWall(WallType::kWoodWall, m_center.x + 742, m_center.y + 70, 0);
	SpawnWall(WallType::kMetalWall, m_center.x + 770, m_center.y + 112, 90);
	SpawnWall(WallType::kWoodWall, m_center.x + 770, m_center.y + 42, 0);
	SpawnWall(WallType::kWoodWall, m_center.x + 770, m_center.y + 14, 0);
	SpawnWall(WallType::kWoodWall, m_center.x + 798, m_center.y + 154, 0);
	SpawnWall(WallType::kWoodWall, m_center.x + 826, m_center.y + 154, 0);
	SpawnWall(WallType::kWoodWall, m_center.x + 854, m_center.y + 154, 0);
	SpawnWall(WallType::kWoodWall, m_center.x + 882, m_center.y + 154, 0);
	SpawnWall(WallType::kMetalWall, m_center.x + 770, m_center.y + 196, 90);
	SpawnWall(WallType::kWoodWall, m_center.x + 658, m_center.y + 238, 0);
	SpawnWall(WallType::kWoodWall, m_center.x + 686, m_center.y + 238, 0);
	SpawnWall(WallType::kWoodWall, m_center.x + 714, m_center.y + 238, 0);
	SpawnWall(WallType::kWoodWall, m_center.x + 742, m_center.y + 238, 0);
	SpawnWall(WallType::kMetalWall, m_center.x + 630, m_center.y + 280, 90);
	SpawnWall(WallType::kWoodWall, m_center.x + 630, m_center.y + 350, 0);
	SpawnWall(WallType::kWoodWall, m_center.x + 630, m_center.y + 378, 0);
	SpawnWall(WallType::kWoodWall, m_center.x + 630, m_center.y + 406, 0);
	SpawnWall(WallType::kWoodWall, m_center.x + 630, m_center.y + 434, 0);
	SpawnWall(WallType::kWoodWall, m_center.x + 602, m_center.y + 322, 0);
	SpawnWall(WallType::kMetalWall, m_center.x + 532, m_center.y + 322, 0);
	SpawnWall(WallType::kWoodWall, m_center.x + 462, m_center.y + 322, 0);
	SpawnWall(WallType::kMetalWall, m_center.x + 392, m_center.y + 322, 0);
	SpawnWall(WallType::kWoodWall, m_center.x + 322, m_center.y + 322, 0);
	SpawnWall(WallType::kMetalWall, m_center.x + 252, m_center.y + 322, 0);
	SpawnWall(WallType::kWoodWall, m_center.x + 98, m_center.y + 322, 0);
	SpawnWall(WallType::kWoodWall, m_center.x + 126, m_center.y + 322, 0);
	SpawnWall(WallType::kWoodWall, m_center.x + 154, m_center.y + 322, 0);
	SpawnWall(WallType::kWoodWall, m_center.x + 182, m_center.y + 322, 0);

    // Add Exterior Walls
    SpawnWall(WallType::kExterior, m_center.x + 910, m_center.y, 90);
    SpawnWall(WallType::kExterior, m_center.x - 910, m_center.y, 90);
    SpawnWall(WallType::kExterior, m_center.x, m_center.y + 462, 0);
    SpawnWall(WallType::kExterior, m_center.x, m_center.y - 462, 0);
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

	// Claude - Store a pointer to the wooden walls.
	if (type == WallType::kWoodWall)
	{
		Wall* wall_ptr = wall.get();
		m_wall_map[m_wall_id_counter++] = wall_ptr;
		wall_ptr->SetIdentifier(m_wall_id_counter - 1);
	}


    m_scene_layers[static_cast<int>(SceneLayers::kWalls)]->AttachChild(std::move(wall));
}

void World::AddEnemies()
{
	return;
}

sf::FloatRect World::GetViewBounds() const
{
	return sf::FloatRect(m_camera.getCenter() - m_camera.getSize() / 2.f, m_camera.getSize());
}

sf::FloatRect World::GetBattleFieldBounds() const
{
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

void World::HandleCollisions()
{
	std::set<SceneNode::Pair> collision_pairs;
	m_scene_graph.CheckSceneCollision(m_scene_graph, collision_pairs);
	

	for (SceneNode::Pair pair : collision_pairs)
	{
		// Handle collision for 2 tanks
		// destroy both tnaks on collision
		if (MatchesCategories(pair, ReceiverCategories::kTank, ReceiverCategories::kTank))
		{
			auto& one = static_cast<Tank&>(*pair.first);
			auto& two = static_cast<Tank&>(*pair.second);

			one.AddPoints(1);
			two.AddPoints(1);

			one.Destroy();
			two.Destroy();
		}
		// Handle projectile/tank collision
		// If its the player's own bullet then only deal damage if the bullet has already bounced once, stops bullet imediately killing player as it can spawn inside the players bounding box depending on rotation
		else if (MatchesCategories(pair, ReceiverCategories::kTank, ReceiverCategories::kProjectile))
		{
			auto& tank = static_cast<Tank&>(*pair.first);
			auto& projectile = static_cast<Projectile&>(*pair.second);
			if (&tank == &projectile.GetOwner() && projectile.GetBounces() >= 1)
			{
				projectile.GetOwner().AddPoints(-1);
				tank.Damage(projectile.GetDamage());
				projectile.Destroy();
			}
			else if (&tank != &projectile.GetOwner())
			{
				projectile.GetOwner().AddPoints(1);
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
		// Handle projectile/breakable wall collisions
		else if (MatchesCategories(pair, ReceiverCategories::kProjectile, ReceiverCategories::kWoodWall))
		{
			// Claude -> send a wall destruction as a game event
			auto& projectile = static_cast<Projectile&>(*pair.first);
			auto& wall = static_cast<Wall&>(*pair.second);

			projectile.Destroy();

			// Don't damage the wall locally — send to server instead
			if (m_networked_world)
			{
				// Let the server decide wall destruction — only the projectile owner sends the event
				if (m_network_node && projectile.IsLocallyOwned())
				{
					m_network_node->NotifyGameAction(GameActions::kWallDestroyed, wall.getPosition());
				}
				// Don't damage the wall here — wait for server kWallDestroyed packet
			}

		}
		// Handle projectile/durable wall collisions
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
				projectile.setRotation(sf::degrees((std::atan2(projectile.GetVelocity().y, projectile.GetVelocity().x) * 180 / 3.14159265f)));
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

void World::DestroyWallAt(sf::Vector2f position)
{
	for (auto& pair : m_wall_map)
	{
		if (pair.second &&
			std::abs(pair.second->getPosition().x - position.x) < 1.f &&
			std::abs(pair.second->getPosition().y - position.y) < 1.f)
		{
			pair.second->Destroy();
			break;
		}
	}
}

void World::DestroyEntitiesOutsideView()
{
	return;
}

void World::UpdateSounds()
{
	sf::Vector2f listener_position;

	if (m_player_tank.empty())
	{
		listener_position = m_camera.getCenter();
	}
	else
	{
		for (Tank* tank : m_player_tank)
		{
			listener_position += tank->GetWorldPosition();
		}
		listener_position /= static_cast<float>(m_player_tank.size());
	}

	m_sounds.SetListenerPosition(listener_position);
	m_sounds.RemoveStoppedSounds();
}
