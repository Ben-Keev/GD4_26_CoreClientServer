#include "SocketWrapperPCH.hpp"
#include "application.hpp"
#include <iostream>

/// <summary>
/// Modified: Ben Mc Keever D00254413
/// </summary>
/// <returns></returns>
int main()
{
	sf::Joystick::update();

	for (unsigned int i = 0; i < sf::Joystick::Count; ++i)
	{
		if (sf::Joystick::isConnected(i))
		{
			auto identification = sf::Joystick::getIdentification(i);

			std::cout << "Joystick " << i << " is connected\n";
			std::cout << "Name: " << identification.name.toAnsiString() << "\n";
			std::cout << "Vendor ID: " << identification.vendorId << "\n";
			std::cout << "Product ID: " << identification.productId << "\n\n";
		}
	}

	try
	{
		Application app;
		app.Run();
	}
	catch (std::runtime_error& e)
	{
		std::cout << e.what() << std::endl;
	}
	
}