#pragma once
#include "entity.hpp"
#include "wall_type.hpp"
#include "resource_identifiers.hpp"

/// <summary>
/// Authored: Kaylon Riordan D00255039
/// Created a wall entity which blocks players paths and bounces projectiles
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