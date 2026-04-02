#pragma once

/// <summary>
/// Authored: Kaylon Riordan D00255039
/// Differentiate between walls for textures and because wooden walls can be broken
/// </summary>

enum class WallType
{
	kMetalWall,
	kWoodWall,
	kExterior,
	kWallCount
};