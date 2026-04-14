#pragma once
#include <SFML/System/Time.hpp>

/// <summary>
/// Modified: Ben
/// </summary>
/// 
constexpr auto kPlayerSpeed = 100.f;
constexpr auto kTimePerFrame = 1.f / 60.f;
constexpr auto kGameOverToMenuPause = 6;
constexpr int kMaxPacketSize = 1300;
constexpr int kMaxPlayers = 2;

// (Ben) How long to wait on the lobby
constexpr uint8_t kLobbyCountdown = 15;

// (Ben) Server Constants
constexpr sf::Time kClientTimeout = sf::seconds(1.f);
constexpr float kTickRate = 1.f / 30.f;
constexpr int kDefaultHP = 10;