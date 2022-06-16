#include <iostream>
#include <Windows.h>

#include "Echoer.h"
#include "FocusHook.h"

using namespace EchoMIDI;

void printExcept(std::exception& e, size_t level = 0)
{
	try
	{
		std::cout << '[' << level << "] Exception:\n" << e.what() << "\n\n";

		std::rethrow_if_nested(e);
	}
	catch (std::exception& e)
	{
		printExcept(e, level + 1);
	}
}

int main()
{
	EchoMIDIInit();

	try
	{
		std::cout << "IN Devices: " << midiInGetNumDevs() << '\n';

		for (UINT i = 0; i < midiInGetNumDevs(); i++)
		{
			std::cout << i << ": " << getMidiInputName(i) << '\n';
		}

		std::cout << "\nInput Device: ";

		UINT dev_id = INVALID_MIDI_ID;

		std::cin >> dev_id;

		std::cin.get();

		std::cout << "\nActive MIDI Device: " << getMidiInputName(dev_id) << '\n';

		Echoer echoer = Echoer(dev_id);

		std::cout << "\nOUT Devices: " << midiOutGetNumDevs() << '\n';

		for (UINT i = 0; i < midiOutGetNumDevs(); i++)
		{
			std::cout << i << ": " << getMidiOutputName(i) << '\n';
		}

		size_t output_count = 1;

		while (true)
		{
			std::cout << "\nOut Device (#" << output_count << "): ";

			UINT out_device = INVALID_MIDI_ID;

			std::cin >> out_device;

			std::cin.get();

			if (!isValidOutID(out_device))
				break;

			echoer.add(out_device);

			std::cout << "Added [" << getMidiOutputName(out_device) << "]\n";

			std::cout << "Focus mute: ";

			std::string focus_mute_app;

			std::cin >> focus_mute_app;

			echoer.focusMute(out_device, focus_mute_app);

			output_count++;
		}
		
		std::cout << "Starting EchoMIDI\n";

		echoer.start();

		echoer.focusMute(getMidiOutIDByName("loopMIDI Port A"), "reaper");
		echoer.focusMute(getMidiOutIDByName("loopMIDI Port B"), "MuseScore3");

		std::cout << "Currently Echoing\n";

		std::cin.get();

		std::cout << "Stopping EchoMIDI\n";
	}
	catch (std::exception& e)
	{
		printExcept(e);
	}

	EchoMIDICleanup();
}
