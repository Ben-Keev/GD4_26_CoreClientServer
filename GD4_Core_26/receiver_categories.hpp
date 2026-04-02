#pragma once

/// <summary>
/// Modified: Ben Mc Keever D00254413
/// Names changed to reflect red/blue
/// Modified: Kaylon Riordan D00255039
/// Added categories for turrets and walls, removed unused categories for enemies
/// </summary>

enum class ReceiverCategories
{
	kNone = 0,
	kScene = 1 << 0,
	kRedTank = 1 << 1,
	kBlueTank = 1 << 2, 
	kRedTurret = 1 << 3,
	kBlueTurret = 1 << 4,
	kRedProjectile = 1 << 5,
	kBlueProjectile = 1 << 6,
	kMetalWall = 1 << 7,
	kWoodWall = 1 << 8,
	kParticleSystem = 1 << 9,
    kSoundEffect = 1 << 10,
	kExteriorWall = 1 << 11,

	kTank = kRedTank | kBlueTank,
	kTurret = kRedTurret | kBlueTurret,
	kProjectile = kRedProjectile | kBlueProjectile,
	kWall = kMetalWall | kWoodWall | kExteriorWall
};

//A message that would be sent to all aircraft would be
//unsigned int all_aircraft = ReceiverCategories::kPlayer | ReceiverCategories::kAlloedAircraft | ReceiverCategories::kEnemyAircraft