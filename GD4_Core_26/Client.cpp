#include "application.hpp"

#include <iostream>

/// <summary>
/// Start server from a separate project
/// Authored: Ben Mc Keever
/// </summary>
int main()
{
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
}