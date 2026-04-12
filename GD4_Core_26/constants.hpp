#pragma once
#include <SFML/System/Time.hpp>
constexpr auto kPlayerSpeed = 100.f;
constexpr auto kTimePerFrame = 1.f / 60.f;
constexpr auto kGameOverToMenuPause = 6;
constexpr int kMaxPacketSize = 1300;
constexpr int kMaxPlayers = 16;
constexpr float kLobbyCountdown = 15.f;

// Server Constants
constexpr sf::Time kClientTimeout = sf::seconds(1.f);
constexpr float kTickRate = 1.f / 30.f;
constexpr int kDefaultHP = 10;