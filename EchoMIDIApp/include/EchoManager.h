#pragma once

#include <Echoer.h>

/// @brief class reseponsible for handling a system of midi input devices and their corresponding target output devices.
/// stores properties for midi devices that are currently being used / has been used, unless forgetMidi(In/Out)Device() is excplicitly called.
class EchoManager
{
public:

	struct MidiOutProps
	{
		bool avaliable;
		std::map<std::string, bool> mute;
		std::map<std::string, std::string> focus_send;
	};

	struct MidiInProps
	{
		/// @brief wether the midi device is currently plugged in to the system.
		bool avaliable;
		/// @brief wether the midi device should currently be sending data to its targets.
		bool echo;
		EchoMIDI::Echoer echoer;

	};

	/// @brief updates the avaliability of all the currently stored midi devices, and adds any new midi devices connected to the computer.
	void syncMidiDevices();

	/// @brief set the send state for a input device
	/// 
	/// if the send state is true, the input device will be opened and start echoing to its output targets.
	/// if set to true whilst the midi device is unavaliable, the Echoer will be opened as soon as the device is avaliable again.
	/// 
	/// @param name the name of the midi device
	/// @param val the send state
	void setInEcho(const std::string& name, bool val);

	/// @brief set the mute state of the target output, for the sources Echoer instance.
	void setTargetMute(const std::string& target, const std::string& source, bool val);

	void setTargetFocusSend(const std::string& target, const std::string& source, const std::string& val);

	bool inIsAvaliable(const std::string& name)
	{
		return m_midi_inputs[name].avaliable;
	}

	bool outIsAvaliable(const std::string& name)
	{
		return m_midi_outputs[name].avaliable;
	}

	const std::map<std::string, MidiInProps>& getMidiInputs() { return m_midi_inputs; }
	
	const std::map<std::string, MidiOutProps>& getMidiOutputs() { return m_midi_outputs; }

	// I/O functions

	void saveToFile(std::filesystem::path file);
	void loadFromFile(std::filesystem::path file, bool keep_unsaved = true);

private:

	// initializes the source Echoer with the target outptus devices properties, if the target is not muted for the source.
	void tryAddTarget(const std::string& target, const std::string& source);

	std::map<std::string, MidiInProps> m_midi_inputs;
	std::map<std::string, MidiOutProps> m_midi_outputs;
};

