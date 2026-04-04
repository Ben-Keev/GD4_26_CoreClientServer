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

	try
	{
		Application app;
		app.Run();
		app.isJoystickConnected();
	}
	catch (std::runtime_error& e)
	{
		std::cout << e.what() << std::endl;
	}
	
}