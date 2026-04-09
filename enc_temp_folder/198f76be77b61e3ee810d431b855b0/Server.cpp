#include "SocketWrapperPCH.hpp"
#include "game_server.hpp"

int main()
{
    std::cout << "Starting server..." << std::endl;

    GameServer server(sf::Vector2f({ 1792, 896 }));

    // Block here so main doesn't exit
    std::cout << "Server running. Press Enter to exit." << std::endl;
    std::cin.get();

    // When main ends, server destructor joins thread safely
}