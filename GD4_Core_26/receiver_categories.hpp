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
    kScene = 1ULL << 0,

    // Walls / Others
    kMetalWall = 1ULL << 7,
    kWoodWall = 1ULL << 8,
    kParticleSystem = 1ULL << 9,
    kSoundEffect = 1ULL << 10,
    kExteriorWall = 1ULL << 11,

    // Tanks (Red/Blue first, then remaining colours)
    kRedTank = 1ULL << 12,
    kBlueTank = 1ULL << 13,
    kGreenTank = 1ULL << 14,
    kYellowTank = 1ULL << 15,
    kPurpleTank = 1ULL << 16,
    kBrownTank = 1ULL << 17,
    kWhiteTank = 1ULL << 18,
    kGrayTank = 1ULL << 19,
    kOrangeTank = 1ULL << 20,
    kPinkTank = 1ULL << 21,
    kCyanTank = 1ULL << 22,
    kLimeTank = 1ULL << 23,
    kNavyTank = 1ULL << 24,
    kPeachTank = 1ULL << 25,
    kGoldTank = 1ULL << 26,
    kTanTank = 1ULL << 27,

    // Turrets (Red/Blue first, then remaining colours)
    kRedTurret = 1ULL << 28,
    kBlueTurret = 1ULL << 29,
    kGreenTurret = 1ULL << 30,
    kYellowTurret = 1ULL << 31,
    kPurpleTurret = 1ULL << 32,
    kBrownTurret = 1ULL << 33,
    kWhiteTurret = 1ULL << 34,
    kGrayTurret = 1ULL << 35,
    kOrangeTurret = 1ULL << 36,
    kPinkTurret = 1ULL << 37,
    kCyanTurret = 1ULL << 38,
    kLimeTurret = 1ULL << 39,
    kNavyTurret = 1ULL << 40,
    kPeachTurret = 1ULL << 41,
    kGoldTurret = 1ULL << 42,
    kTanTurret = 1ULL << 43,

    // Projectiles (Red/Blue first, then remaining colours)
    kRedProjectile = 1ULL << 44,
    kBlueProjectile = 1ULL << 45,
    kGreenProjectile = 1ULL << 46,
    kYellowProjectile = 1ULL << 47,
    kPurpleProjectile = 1ULL << 48,
    kBrownProjectile = 1ULL << 49,
    kWhiteProjectile = 1ULL << 50,
    kGrayProjectile = 1ULL << 51,
    kOrangeProjectile = 1ULL << 52,
    kPinkProjectile = 1ULL << 53,
    kCyanProjectile = 1ULL << 54,
    kLimeProjectile = 1ULL << 55,
    kNavyProjectile = 1ULL << 56,
    kPeachProjectile = 1ULL << 57,
    kGoldProjectile = 1ULL << 58,
    kTanProjectile = 1ULL << 59,

    // Groups
    kTank =
    kRedTank | kBlueTank | kGreenTank | kYellowTank |
    kPurpleTank | kBrownTank | kWhiteTank | kGrayTank |
    kOrangeTank | kPinkTank | kCyanTank | kLimeTank |
    kNavyTank | kPeachTank | kGoldTank | kTanTank,

    kTurret =
    kRedTurret | kBlueTurret | kGreenTurret | kYellowTurret |
    kPurpleTurret | kBrownTurret | kWhiteTurret | kGrayTurret |
    kOrangeTurret | kPinkTurret | kCyanTurret | kLimeTurret |
    kNavyTurret | kPeachTurret | kGoldTurret | kTanTurret,

    kProjectile =
    kRedProjectile | kBlueProjectile | kGreenProjectile | kYellowProjectile |
    kPurpleProjectile | kBrownProjectile | kWhiteProjectile | kGrayProjectile |
    kOrangeProjectile | kPinkProjectile | kCyanProjectile | kLimeProjectile |
    kNavyProjectile | kPeachProjectile | kGoldProjectile | kTanProjectile,

    kWall = kMetalWall | kWoodWall | kExteriorWall
};