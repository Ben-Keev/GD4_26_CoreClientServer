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

World::World(sf::RenderTarget& output_target, FontHolder& font, SoundPlayer& sounds, bool networked)
	: m_target(output_target)
	, m_camera(output_target.getDefaultView())
	, m_textures()
	, m_fonts(font)
	, m_sounds(sounds)
	, m_scene_graph(ReceiverCategories::kNone)
	, m_scene_layers()
	, m_world_bounds(sf::Vector2f(0.f, 0.f), sf::Vector2f(m_camera.getSize().x, 5000.f))
	, m_spawn_position(m_camera.getSize().x / 2.f, m_world_bounds.size.y - m_camera.getSize().y / 2.f)
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
	m_camera.setCenter(m_spawn_position);
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
		m_player_tank.erase(std::find(m_player_tank.begin(), m_player_tank.end(), tank));
	}
}

Tank* World::AddAircraft(uint8_t identifier)
{
	std::unique_ptr<Tank> player(new Tank(TankType::kRedTank, m_textures, m_fonts));
	player->setPosition(m_camera.getCenter());
	std::cout << "World::AddTank " << +identifier << std::endl;
	player->SetIdentifier(identifier);

	m_player_tank.emplace_back(player.get());
	m_scene_layers[static_cast<int>(SceneLayers::kTanks)]->AttachChild(std::move(player));
	return m_player_tank.back();
}

bool World::PollGameAction(GameActions::Action& out)
{
	return m_network_node->PollGameAction(out);
}

void World::SetCurrentBattleFieldPosition(float lineY)
{
	m_camera.setCenter(sf::Vector2f(m_camera.getCenter().x, lineY - m_camera.getSize().y / 2));
	m_spawn_position.y = m_world_bounds.size.y;
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
	return;
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