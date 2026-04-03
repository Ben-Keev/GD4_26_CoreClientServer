#pragma once

/// <summary>
/// Modified: Ben Mc Keever D00254413
/// Names changed to reflect red/blue
/// Modified: Kaylon Riordan D00255039
/// Added categories for turrets and walls, removed unused categories for enemies
/// </summary>

// Use 1ULL (unsigned long long = 64-bit) for all shifts instead

enum class ReceiverCategories : std::uint64_t
{
    kNone = 0,
    kScene = 1ULL << 0,

    kRedTank = 1ULL << 1,
    kBlueTank = 1ULL << 2,
    kRedTurret = 1ULL << 3,
    kBlueTurret = 1ULL << 4,
    kRedProjectile = 1ULL << 5,
    kBlueProjectile = 1ULL << 6,
    kMetalWall = 1ULL << 7,
    kWoodWall = 1ULL << 8,
    kParticleSystem = 1ULL << 9,
    kSoundEffect = 1ULL << 10,
    kExteriorWall = 1ULL << 11,

    // Additional Tanks
    kGreenTank = 1ULL << 12,
    kYellowTank = 1ULL << 13,
    kPurpleTank = 1ULL << 14,
    kBrownTank = 1ULL << 15,
    kWhiteTank = 1ULL << 16,
    kGrayTank = 1ULL << 17,
    kOrangeTank = 1ULL << 18,
    kPinkTank = 1ULL << 19,
    kCyanTank = 1ULL << 20,
    kLimeTank = 1ULL << 21,
    kNavyTank = 1ULL << 22,
    kPeachTank = 1ULL << 23,
    kGoldTank = 1ULL << 24,
    kTanTank = 1ULL << 25,

    // Additional Turrets
    kGreenTurret = 1ULL << 26,
    kYellowTurret = 1ULL << 27,
    kPurpleTurret = 1ULL << 28,
    kBrownTurret = 1ULL << 29,
    kWhiteTurret = 1ULL << 30,
    kGrayTurret = 1ULL << 31,
    kOrangeTurret = 1ULL << 32,
    kPinkTurret = 1ULL << 33,
    kCyanTurret = 1ULL << 34,
    kLimeTurret = 1ULL << 35,
    kNavyTurret = 1ULL << 36,
    kPeachTurret = 1ULL << 37,
    kGoldTurret = 1ULL << 38,
    kTanTurret = 1ULL << 39,

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

    kProjectile = kRedProjectile | kBlueProjectile,
    kWall = kMetalWall | kWoodWall | kExteriorWall
};

//A message that would be sent to all aircraft would be
//unsigned int all_aircraft = ReceiverCategories::kPlayer | ReceiverCategories::kAlloedAircraft | ReceiverCategories::kEnemyAircraft