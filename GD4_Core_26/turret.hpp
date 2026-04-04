#pragma once
#include "scene_node.hpp"
#include "turret_type.hpp"
#include "resource_identifiers.hpp"

/// <summary>
/// Authored: Kaylon Riordan D00255039
/// Created a Turret class which is attached to a tank and visualy indicated what direction the tank is aiming
/// </summary>

// Make SceneNode the parrent instead of entity because, this is a visual element with no need for collision or other interactions
class Turret : public SceneNode
{
public:
	Turret(TurretType type, const TextureHolder& textures);
	unsigned int GetCategory() const override;
	void Hide();

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