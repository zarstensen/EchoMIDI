#include <iostream>
#include <Windows.h>

#include "Echoer.h"

int main()
{
	std::cout << "IN Devices: " << midiInGetNumDevs() << '\n';

	for (size_t i = 0; i < midiInGetNumDevs(); i++)
	{
		std::cout << i << ": " << getMidiInputName(i) << '\n';
	}
	
	std::cout << "Device: ";

	int dev_id = -1;

	std::cin >> dev_id;
	
	std::cin.get();
	
	std::cout << "OUT Devices: " << midiOutGetNumDevs() << '\n';

	for (size_t i = 0; i < midiOutGetNumDevs(); i++)
	{
		std::cout << i << ": " << getMidiOutputName(i) << '\n';
	}

	std::cin.get();

	Echoer echoer = Echoer(dev_id);

	echoer.add(3);
	echoer.add(4);

	echoer.start();

	std::cin.get();

	echoer.setMute(4, true);

	std::cin.get();
}
