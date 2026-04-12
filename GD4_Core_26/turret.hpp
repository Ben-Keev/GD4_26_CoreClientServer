#pragma once
#include "scene_node.hpp"
#include "turret_type.hpp"
#include "resource_identifiers.hpp"

/// <summary>
/// Authored: Kaylon Riordan | Modified: Ben Mc Keever
/// </summary>
class Turret : public SceneNode
{
public:

	Turret(TurretType type, const TextureHolder& textures, sf::Color colour);

	static sf::Angle CalculateMouseRotation(const sf::Vector2f& turret_position, const sf::Vector2f& mouse_position);

	void Hide();

	unsigned int GetCategory() const override;
	uint8_t GetIdentifier();
	void SetIdentifier(uint8_t identifier);

protected: 
	virtual void DrawCurrent(sf::RenderTarget& target, sf::RenderStates states) const;

private:
	TurretType m_type;
	sf::Sprite m_sprite;
	bool m_visible = true;

	uint8_t m_identifier;
};