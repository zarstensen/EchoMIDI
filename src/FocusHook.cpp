// Any HWIND specific code will be defined in this file

#include "FocusHook.h"
#include "Echoer.h"

#include <iostream>
#include <fstream>
#include <set>
#include <thread>

namespace EchoMIDI
{

#define BADOUTID(bad_id) BadDeviceID(MIDIIOType::INPUT, bad_id, midiOutGetNumDevs())
#define BADINID(bad_id) BadDeviceID(MIDIIOType::OUTPUT, bad_id, midiInGetNumDevs())

	std::ofstream log_file;

	HWINEVENTHOOK focus_hook;

	std::set<Echoer*> registered_echoers;

	std::thread msg_thread;

	HRESULT winErr(HRESULT err, const char* file, size_t line)
	{
		std::stringstream msg;

		if (err != ERROR_SUCCESS)
		{
			msg << std::system_category().message(err) << " (" << err << "): at " << file << " : " << line << '\n';

			log_file << msg.str();
			std::cout << msg.str();
		}

		return err;
	}

	std::ptrdiff_t winErrB(std::ptrdiff_t success, const char* file, size_t line)
	{
		if (!success)
			winErr(GetLastError(), file, line);

		return success;
	}

#define WINERR(err) winErr((DWORD) err, TEXT(__RELATIVE_FILE__), __LINE__)
#define WINERRB(err) winErrB((std::ptrdiff_t) err, TEXT(__RELATIVE_FILE__), __LINE__)

	void setLogFile(std::filesystem::path lout, bool clear)
	{
		if (!clear && std::filesystem::exists(lout))
			log_file.open(lout, std::ios_base::app);
		else
			log_file.open(lout);

		log_file << "======================== LOG STARTED ========================\n";
	}

	// close and save the log file
	void closeLogFile()
	{
		log_file << "========================= LOG ENDED =========================\n";
		log_file.close();
	}

	std::filesystem::path getHWNDPath(HWND window)
	{
		DWORD win_pid;
		WINERRB(GetWindowThreadProcessId(window, &win_pid));

		TCHAR win_path[MAX_PATH];

		HANDLE hProc = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, win_pid);

		// not high enough privilages
		if (hProc == NULL)
			return "";
		else
			WINERRB(hProc);

		DWORD len = MAX_PATH;
		// retrieve the name of the executable image.
		WINERRB(QueryFullProcessImageName(hProc, NULL, win_path, &len));

		WINERRB(CloseHandle(hProc));

		return win_path;
	}

	void registerEchoer(Echoer* echoer)
	{
		registered_echoers.insert(echoer);
	}

	void unregisterEchoer(Echoer* echoer)
	{
		registered_echoers.erase(echoer);
	}

	// In order to reduce overhead, a single global hook is used for all Echoer instances.
	void focusHook(HWINEVENTHOOK hwin_hook, DWORD event_id, HWND window, LONG id_object, LONG id_child, DWORD id_event_thread, DWORD event_time)
	{
		if (event_id == EVENT_OBJECT_FOCUS)
		{
			std::filesystem::path window_path = getHWNDPath(window);

			// ignore if invalid window was passed.
			if (window_path == "")
				return;

			for (Echoer* echoer : registered_echoers)
			{
				for (auto& [id, midi_out] : *echoer)
				{
					if (midi_out.focus_mute_path.empty())
						continue;

					if (// check if the two paths match excactly
						midi_out.focus_mute_path.has_parent_path() && midi_out.focus_mute_path == window_path ||
						// if the path is relative, only check the executable name, accounting for a lack of extension aswell
						!midi_out.focus_mute_path.has_parent_path() &&
						(midi_out.focus_mute_path.has_extension() && midi_out.focus_mute_path.filename() == window_path.filename() ||
							midi_out.focus_mute_path.filename() == window_path.filename().replace_extension("")))
					{
						midi_out.focus_muted = false;
					}
					else
					{
						midi_out.focus_muted = true;
					}
				}
			}
		}
	}

	void msgThread()
	{
		focus_hook = SetWinEventHook(EVENT_OBJECT_FOCUS, EVENT_OBJECT_FOCUS, NULL, focusHook, NULL, NULL, WINEVENT_OUTOFCONTEXT | WINEVENT_SKIPOWNTHREAD);
		WINERRB(focus_hook);

		MSG msg;

		while (GetMessage(&msg, NULL, NULL, NULL))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		WINERRB(UnhookWinEvent(focus_hook));
	}

	void EchoMIDIInit()
	{
		msg_thread = std::thread(msgThread);
	}

	void EchoMIDICleanup()
	{
		PostThreadMessage(GetThreadId(msg_thread.native_handle()), WM_QUIT, NULL, NULL);

		msg_thread.join();
	}
}
