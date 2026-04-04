#pragma once

/// <summary>
/// Modified: Ben Mc Keever D00254413
/// Names changed to reflect red/blue
/// Modified: Kaylon Riordan D00255039
/// Added categories for turrets, walls, and projectiles
/// Reordered to group tanks, turrets, and projectiles together
/// </summary>

enum class ReceiverCategories : std::uint64_t
{
    kNone = 0,
    kScene = 1 << 0,

    // Tanks (Red/Blue first, then remaining colours)
    kRedTank = 1 << 1,
    kBlueTank = 1 << 2,

    // Turrets (Red/Blue first, then remaining colours)
    kRedTurret = 1 << 3,
    kBlueTurret = 1 << 4,

    // Projectiles (Red/Blue first, then remaining colours)
    kRedProjectile = 1 << 5,
    kBlueProjectile = 1 << 6,

    // Walls / Others
    kMetalWall = 1 << 7,
    kWoodWall = 1 << 8,
    kParticleSystem = 1 << 9,
    kSoundEffect = 1 << 10,
    kExteriorWall = 1 << 11,
    kNetwork = 1 << 12,



    // Groups
    kTank =
    kRedTank | kBlueTank,

    kTurret =
    kRedTurret | kBlueTurret,

    kProjectile =
    kRedProjectile | kBlueProjectile,

    kWall = kMetalWall | kWoodWall | kExteriorWall
};