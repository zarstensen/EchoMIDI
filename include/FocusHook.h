#pragma once

#include "Echoer.h"

#include <filesystem>
#include <Windows.h>

/// @brief sets the output of winErr, the underlying windows error message handler function.
/// the output of winErr will be written to lout as well as cout.
/// if clear is set to true, the file passed will be cleared before writing.
void setLogFile(std::filesystem::path lout, bool clear);

/// @brief close and save the log file
void closeLogFile();

/// @brief returns the executable path of the passed window
std::filesystem::path getHWNDPath(HWND window);

/// @brief initializes a windows hook that listens for focus changes.
/// if this is not called, the Echoer::focusMute() function will not work properly. 
void focusMuterInit();
/// @brief cleansup the previously installed windows hook.
/// as soon as this is called, the Echoer focus mute functionallity will not work.
void focusMuterCleanup();

/// @brief get the path of the executable that owns the passed window.
std::filesystem::path getHWNDPath(HWND window);

/// @brief registers an Echoer instance to be monitored for focus changes.
/// @important this function is automaticly called in the constructor of Echoer, and should never be used outside of this.
void registerEchoer(Echoer* echoer);

/// @brief unregisters an Echoer instance to be monitored for focus changes.
/// @important this function is automaticly called in the destructor of Echoer, and should never be used outside of this.
void unregisterEchoer(Echoer* echoer);
