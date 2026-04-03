#pragma once
#include <SFML/Graphics.hpp>
#include "resource_identifiers.hpp"
#include "scene_node.hpp"
#include "scene_layers.hpp"
#include "tank.hpp"
#include "command_queue.hpp"
#include "sound_player.hpp"
#include "bloom_effect.hpp"
#include "Wall.hpp"

/// <summary>
/// Modified: Ben Mc Keever D00254413
/// 
/// Modified: Kaylon Riordan D00255039
/// Added methods used to place walls in the map
/// </summary>

class World
{
public:
	explicit World(sf::RenderTarget& output_target, FontHolder& font, SoundPlayer& sounds);
	void Update(sf::Time dt);
	void Draw();

	CommandQueue& GetCommandQueue();

	bool AllPlayersAlive() const;
	bool HasPlayerDied(int player) const;

private:
	void LoadTextures();
	void BuildScene();

	void AddWalls();
	void SpawnWall(WallType type, float x, float y, float rotation);

	Tank* SpawnTank(TankType type);

	sf::FloatRect GetViewBounds() const;
	sf::FloatRect GetBattleFieldBounds() const;

	void HandleCollisions();

	void DestroyEntitiesOutsideView();
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
	sf::Vector2f m_red_position;
	sf::Vector2f m_blue_position;
	float m_scroll_speed;
	
	std::array<Tank*, kMaxPlayers> m_tanks;

	CommandQueue m_command_queue;

	std::vector<SpawnPoint> m_enemy_spawn_points;
	std::vector<Tank*> m_active_enemies;

	BloomEffect m_bloom_effect;
};