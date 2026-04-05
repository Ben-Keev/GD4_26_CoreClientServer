#include "SocketWrapperPCH.hpp"
#include "turret.hpp"
#include "texture_id.hpp"
#include "utility.hpp"
#include "constants.hpp"
#include "data_tables.hpp"
#include <iostream>

/// <summary>
/// Authored: Kaylon Riodan
/// </summary>

namespace
{
	const std::vector<TurretData> Table = InitializeTurretData();
}

/// Create  turret using a scene node with its own type and sprites, used as a sprite on top of tank to show aim direction
Turret::Turret(TurretType type, const TextureHolder& textures)
	:m_type(type)
	, m_sprite(textures.Get(Table[static_cast<int>(type)].m_texture), Table[static_cast<int>(type)].m_texture_rect)
{
	Utility::CentreOrigin(m_sprite);
}

unsigned int Turret::GetCategory() const
{
	return static_cast<unsigned int>(ReceiverCategories::kTurret);
}

/// <summary>
/// Authored: Kaylon Riodan
/// Method to stop drawing the turret, used by tank when they die so turret isnt visible inside explosion
/// </summary>
void Turret::Hide() {
	m_visible = false;
}

void Turret::DrawCurrent(sf::RenderTarget& target, sf::RenderStates states) const
{
	if (m_visible)
	{
		target.draw(m_sprite, states);
	}
}