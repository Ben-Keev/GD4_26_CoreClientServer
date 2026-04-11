#include "wall.hpp"
#include "utility.hpp"
#include "data_tables.hpp"

/// <summary>
/// Authored: Kaylon Riodan
/// Create a wall tyoe which is a basic entity, used in colsion logic to block players movment and bounce projectiles
/// </summary>

namespace
{
	const std::vector<WallData> Table = InitializeWallData();
}

Wall::Wall(WallType type, const TextureHolder& textures)
	: Entity(10)
	, m_type(type)
	, m_sprite(textures.Get(Table[static_cast<int>(type)].m_texture), Table[static_cast<int>(type)].m_texture_rect)
	, m_is_marked_for_removal(false)
{
	Utility::CentreOrigin(m_sprite);
}

unsigned int Wall::GetCategory() const
{
	if (m_type == WallType::kMetalWall)
	{
		return static_cast<unsigned int>(ReceiverCategories::kMetalWall);
	}
	else if (m_type == WallType::kExterior)
	{
		return static_cast<unsigned int>(ReceiverCategories::kExteriorWall);
	}
	return static_cast<unsigned int>(ReceiverCategories::kWoodWall);
}

sf::FloatRect Wall::GetBoundingRect() const
{
	return GetWorldTransform().transformRect(m_sprite.getGlobalBounds());
}

bool Wall::IsMarkedForRemoval() const
{
	return m_is_marked_for_removal;
}

void Wall::DrawCurrent(sf::RenderTarget& target, sf::RenderStates states) const
{
	if (!IsDestroyed())
	{
		target.draw(m_sprite, states);
	}
}

void Wall::SetIdentifier(uint8_t id)
{	
	m_identifier = id;
}

uint8_t Wall::GetIdentifier() const
{
	return m_identifier;
}

void Wall::UpdateCurrent(sf::Time dt, CommandQueue& commands)
{
	Entity::UpdateCurrent(dt, commands);
}