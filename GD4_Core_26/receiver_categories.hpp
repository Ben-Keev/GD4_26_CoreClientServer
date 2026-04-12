#pragma once

/// <summary>
/// Modified: Ben Mc Keever D00254413
/// Names changed to reflect red/blue
/// Modified: Kaylon Riordan D00255039
/// Added categories for turrets, walls, and projectiles
/// Reordered to group tanks, turrets, and projectiles together
/// Removed refrences to specific colours instead using genral variables
/// </summary>


enum class ReceiverCategories : std::uint64_t
{
    kNone = 0,
    kScene = 1 << 0,

    // Tanks (Red/Blue first, then remaining colours)
    kTank = 1 << 1,

    // Turrets (Red/Blue first, then remaining colours)
    kTurret = 1 << 2,

    // Projectiles (Red/Blue first, then remaining colours)
    kProjectile = 1 << 3,

    // Walls / Others
    kMetalWall = 1 << 7,
    kWoodWall = 1 << 8,
    kParticleSystem = 1 << 9,
    kSoundEffect = 1 << 10,
    kExteriorWall = 1 << 11,
    kNetwork = 1 << 12,

    kWall = kMetalWall | kWoodWall | kExteriorWall
};