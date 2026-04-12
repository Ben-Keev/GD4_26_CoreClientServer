#pragma once
#include "resource_identifiers.hpp"
#include "scene_node.hpp"
#include "scene_layers.hpp"
#include "tank.hpp"
#include "tank_type.hpp"
#include "command_queue.hpp"
#include "bloom_effect.hpp"
#include "sound_player.hpp"
#include "sprite_node.hpp"
#include "network_node.hpp"
#include "wall_type.hpp"
#include "wall.hpp"

#include <SFML/Graphics.hpp>
#include <array>

/// <summary>
/// World
/// Class Authors: John Loane, Ben Mc Keever, Kaylon Riordan, with assistance from Claude AI
/// </summary>
class World
{
public:
	explicit World(sf::RenderTarget& output_target, FontHolder& font, SoundPlayer& sounds, bool networked = false);
	void Update(sf::Time dt);
	void Draw();

	sf::FloatRect GetViewBounds() const;
	CommandQueue& GetCommandQueue();

	Tank* AddAircraft(uint8_t identifier, PlayerDetails* details, sf::Vector2f position);
	void RemoveAircraft(uint8_t identifier);
	void SetCurrentBattleFieldPosition(float line_y);
	void SetWorldHeight(float height);

	bool HasAlivePlayer() const;

	Tank* GetAircraft(int identifier) const;
	sf::FloatRect GetBattleFieldBounds() const;
	bool PollGameAction(GameActions::Action& out);

	const sf::View& GetCamera() const { return m_camera; };

	// (Ben's Claude)
	void DestroyWallAt(sf::Vector2f position);
	
	// (Ben)
	void SetLocalPlayerIdentifier(uint8_t identifier);

	// (Ben's Claude)
	void DestroyProjectile(uint16_t id);

	~World();

private:
	void LoadTextures();
	void BuildScene();
	void AdaptPlayerVelocity();
	void AdaptPlayerPosition();

	void SpawnWall(WallType type, float x, float y, float rotation);
	void AddWalls();

	void HandleCollisions();

	void UpdateSounds();

private:
	struct SpawnPoint
	{
		SpawnPoint(TankType type, float x, float y) :m_type(type), m_x(x), m_y(y)
		{

		}
		TankType m_type;
		float m_x;
		float m_y;
	};

private:
	sf::RenderTarget& m_target;
	sf::RenderTexture m_scene_texture;
	sf::View m_camera;
	TextureHolder m_textures;
	FontHolder& m_fonts;
	SoundPlayer& m_sounds;
	SceneNode m_scene_graph;
	std::array<SceneNode*, static_cast<int>(SceneLayers::kLayerCount)> m_scene_layers;
	sf::FloatRect m_world_bounds;
	sf::Vector2f m_center;

	std::vector<Tank*> m_player_tank;
	uint8_t m_local_player_identifier;

	CommandQueue m_command_queue;

	BloomEffect m_bloom_effect;
	bool m_networked_world;
	NetworkNode* m_network_node;
	SpriteNode* m_finish_sprite;

	std::map<uint8_t, Wall*> m_wall_map;
	uint8_t m_wall_id_counter = 0;

	// (Ben's Claude) Track all projectiles
	std::map<uint16_t, Projectile*> m_projectile_map;

	// (Ben's Claude) Prevent crash when projectile hit's its owner
	std::shared_ptr<bool> m_alive_token = std::make_shared<bool>(true);

	// (Ben's Claude) Fix desync caused by projectiles destroyed before the map could register them
	std::set<uint16_t> m_pending_destroy_ids;
};