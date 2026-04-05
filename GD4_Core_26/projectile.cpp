#include "SocketWrapperPCH.hpp"
#include "projectile.hpp"
#include "data_tables.hpp"
#include "utility.hpp"
#include "emitter_node.hpp"
#include "particle_type.hpp"

/// <summary>
/// Modified: Kaylon Riordan D00255039
/// </summary>
namespace
{
    const std::vector<ProjectileData> Table = InitializeProjectileData();
}

Projectile::Projectile(ProjectileType type, const TextureHolder& textures, Tank* owner) : Entity(1), m_type(type),
    m_sprite(textures.Get(Table[static_cast<int>(type)].m_texture), Table[static_cast<int>(type)].m_texture_rect), m_owner(owner)
{
    Utility::CentreOrigin(m_sprite);

    //Add particle system
    std::unique_ptr<EmitterNode> smoke(new EmitterNode(ParticleType::kSmoke));
    smoke->setPosition(sf::Vector2f(0.f, GetBoundingRect().size.y / 2.f));
    AttachChild(std::move(smoke));

    std::unique_ptr<EmitterNode> propellant(new EmitterNode(ParticleType::kPropellant));
    propellant->setPosition(sf::Vector2f(0.f, GetBoundingRect().size.y / 2.f));
    AttachChild(std::move(propellant));
}

unsigned int Projectile::GetCategory() const
{
    return static_cast<int>(ReceiverCategories::kProjectile);
}

sf::FloatRect Projectile::GetBoundingRect() const
{
    return GetWorldTransform().transformRect(m_sprite.getGlobalBounds());
}

float Projectile::GetMaxSpeed() const
{
    return Table[static_cast<int>(m_type)].m_speed;
}

float Projectile::GetDamage() const
{
    return Table[static_cast<int>(m_type)].m_damage;
}

Tank& Projectile::GetOwner() const
{
    return *m_owner;
}

/// <summary>
/// Authored: Kaylon Riordan D00255039
/// return how many times the bullet has bounced, used when determing if the bullet should be destroyed after exceeding bounce limit
/// </summary>
int Projectile::GetBounces() const
{
    return m_bounces;
}

/// <summary>
/// Authored: Kaylon Riordan D00255039
/// returns if the bullet has already bounced this tick, stops bullet behaviour from glitching when bullet hits multiple walls similtaniously
/// </summary>
bool Projectile::CanBounce() const
{
    return !m_has_bounced;
}

/// <summary>
/// Authored: Kaylon Riordan D00255039
/// updates the has bounced vatiable, increments the number of bounces, and destroys the bullet if it has bounced too many times
/// </summary>
void Projectile::AddBounce()
{
    m_has_bounced = true;
    m_bounces++;
    if (m_bounces > Table[static_cast<int>(m_type)].m_max_bounces)
    {
        Destroy();
    }
}

void Projectile::UpdateCurrent(sf::Time dt, CommandQueue& commands)
{
    m_has_bounced = false;
    Entity::UpdateCurrent(dt, commands);
}

void Projectile::DrawCurrent(sf::RenderTarget& target, sf::RenderStates states) const
{
    target.draw(m_sprite, states);
}
