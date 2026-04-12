#pragma once
#include "entity.hpp"
#include "wall_type.hpp"
#include "resource_identifiers.hpp"

/// <summary>
/// Create a wall tyoe which is a basic entity, used in colsion logic to block players movment and bounce projectiles
/// Authored: Kaylon Riordan | Modified: Ben Mc Keever
/// </summary>
class Wall : public Entity
{
public:
	Wall(WallType type, const TextureHolder& textures);
	unsigned int GetCategory() const override;

	sf::FloatRect GetBoundingRect() const override;
	bool IsMarkedForRemoval() const override;

	void SetIdentifier(uint8_t id);
	uint8_t GetIdentifier() const;

protected:
	virtual void DrawCurrent(sf::RenderTarget& target, sf::RenderStates states) const;
	virtual void UpdateCurrent(sf::Time dt, CommandQueue& commands) override;

private:
	WallType m_type;
	sf::Sprite m_sprite;
	bool m_is_marked_for_removal;

	uint8_t m_identifier;
};