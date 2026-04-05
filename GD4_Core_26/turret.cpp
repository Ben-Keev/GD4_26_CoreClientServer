#include "SocketWrapperPCH.hpp"
#include "turret.hpp"
#include "utility.hpp"
#include "data_tables.hpp"

/// <summary>
/// Authored: Kaylon Riodan
/// </summary>

namespace
{
	const std::vector<TurretData> Table = InitializeTurretData();
}

sf::Angle Turret::CalculateMouseRotation(const sf::Vector2f& turret_position, const sf::Vector2f& mouse_position)
{
	sf::Vector2f delta = mouse_position - turret_position;
	float radians = std::atan2(delta.y, delta.x);

	// Add -90 degrees so the turret sprite points "up" correctly
	return sf::radians(radians) + sf::degrees(-90.f);
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

uint8_t	Turret::GetIdentifier()
{
	return m_identifier;
}

void Turret::SetIdentifier(uint8_t identifier)
{
	m_identifier = identifier;
}


