#include "game_server.hpp"

#include <iostream>

int main()
{
    std::cout << "Starting server..." << std::endl;

    GameServer server(sf::Vector2f({ 1792, 896 }));

    std::cout << "Server running. Press Enter to exit." << std::endl;
    std::cin.get();
}