#include "Client.hpp"
#include "application.hpp"
#include <iostream>

int main()
{
	std::cout << "YOU IN THE CLIENT BOAH" << std::endl;

	// RUN THE CA1 GAME ====================================================================

	sf::Joystick::update();

	try
	{
		Application app;
		app.Run();
	}
	catch (std::runtime_error& e)
	{
		std::cout << e.what() << std::endl;
	}

	// RUN THE CA1 GAME ====================================================================
}