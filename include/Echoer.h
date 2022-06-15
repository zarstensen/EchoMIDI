#pragma once

#include <Windows.h>
#include <cassert>
#include <map>
#include <string>
#include <stdexcept>
#include <format>
#include <filesystem>

enum class MIDIIOType : uint8_t
{
	UNKNOWN,
	INPUT,
	OUTPUT
};


/// @brief gets the name of the midi input device id passed.
/// 
/// @throw MIDIEchoExcept
std::string getMidiInputName(UINT midi_in_id);
/// @brief gets the name of the midi output device id passed.
/// 
/// @throw MIDIEchoExcept
std::string getMidiOutputName(UINT midi_out_id);

/// @brief same as getMidiInputName if midi_io_type is MIDIIOType::INPUT,
/// and same as getMidiOutputName if midi_io_type is MIDIIOType::OUTPUT.
/// if midi_io_type is MIDIIOType::UNKNOWN, an empty string ("") is returned.
/// 
/// @throw MIDIEchoExcept
std::string getMidiName(MIDIIOType midi_io_type, UINT midi_id);

/// @brief echoes the output of a midi output device into 1 or more midi input devices.
///
/// the Echoer only echoes midi data once start() is called, and stops again if stop() is called.
/// 
/// use add and remove in order to modify the target input devices.
/// if a device should temporarily be muted, meaning no data will be sent to it, use the setMute() function.
/// 
class Echoer
{
public:
	/// @brief struct used for storing a midi output device and its current mute status.
	struct MIDIOutDevice
	{
		bool muted = false;
		HMIDIOUT device_handle;
	};

public:
	/// @brief constructs a Echoer, the source id cannot be changed after construction.
	/// @param source the midi input id which input should be echoed out to the midi output targets.
	/// 
	/// @throw MIDIEchoExcept
	/// @throw BadDeviceID
	/// @throw DeviceAllocated
	Echoer(UINT source);
	/// @throw MIDIEchoExcept
	~Echoer();
	
	/// @brief adds a midi output target.
	/// @return the wether the id was added (true) or not (false).
	/// 
	/// @throw MIDIEchoExcept
	/// @throw BadDeviceID
	/// @throw DeviceAllocated
	bool add(UINT id);
	/// @brief removes an id from the targets list.
	/// if the id is not present, nothing happens.
	/// 
	/// if this is only temporary, setMute() should be used as a better alternative
	/// 
	/// @throw MIDIEchoExcept
	void remove(UINT id);

	/// @brief sets the mute status of the target output device.
	void setMute(UINT id, bool state)
	{
		assert(m_midi_targets.contains(id));
		m_midi_targets[id].muted = state;
	}

	/// @return returns wether the device is muted or not. 
	bool isMuted(UINT id)
	{
		assert(m_midi_targets.contains(id));
		m_midi_targets[id].muted;
	}

	void focusMute(std::filesystem::path exec);

	std::filesystem::path getFocusMuteExec();

	/// @brief retrieve the current midi output devices which are recieving data from the midi input device.
	/// @return a map from the device id and its midi output handler and mute status.
	const std::map<UINT, MIDIOutDevice>& getTargets()
	{
		return m_midi_targets;
	}
	
	/// @brief 
	/// 
	/// @throw MIDIEchoExcept
	void reset();

	/// @brief starts echoing the midi data from the source device into the target devices.
	/// throws if already started.
	/// @see isLitening(), stop()
	/// 
	/// @throw MIDIEchoExcept
	void start();
	/// @brief stops echoing midi data from the target device
	/// throws if start() has not been called.
	/// @see isLitening(), stop()
	void stop();

	/// @return returns wether the midi source device currently is getting its midi data echoed into the target devices. 
	bool isEchoing()
	{
		return m_is_echoing;
	}

private:
	HMIDIIN m_midi_source = NULL;
	UINT m_midi_id;
	std::map<UINT, MIDIOutDevice> m_midi_targets;

	bool m_is_echoing = false;
};

// ============ EXCEPTIONS ============

/// @brief generic exception class for the EchoMIDI library
class MIDIEchoExcept : public std::runtime_error
{
public:
	MIDIEchoExcept(std::string msg, std::string short_msg, MMRESULT err_code, MIDIIOType type = MIDIIOType::UNKNOWN, UINT device_id = -1)
		: std::runtime_error(msg), short_msg(short_msg), err_code(err_code), type(type), device_id(device_id)
	{}

	MIDIEchoExcept(MMRESULT err_code, MIDIIOType type = MIDIIOType::UNKNOWN, UINT device_id = -1)
		: MIDIEchoExcept(std::format("Error occured!\nMMSYRESULT: {}\nID: {}\nI/O TYPE: {}", err_code, device_id, (short)type),
			"UNKNOWN", err_code, type, device_id)
	{}

	/// @brief the underlying MMRESULT code.
	MMRESULT err_code = MMSYSERR_NOERROR;
	/// @brief the io type of the id that caused the exception.
	MIDIIOType type = MIDIIOType::UNKNOWN;
	/// @brief the device id that caused the exception.
	UINT device_id = -1;
	/// @brief a smaller message describing the error in as few words as possible.
	std::string short_msg;
};

/// @brief gets thrown if an invalid device id was passed to a function.
/// MIDIEchoExcept::device_id:
///		the invalid id that caused the exception to be thrown.
/// 
/// MIDIEchoExcept::type:
///		the supposed midi I/O type of the invalid id
class BadDeviceID : public MIDIEchoExcept
{
public:

	/// @param 
	/// @param bad_id the invalid id passed.
	/// @param max_id the maximum valid id, used for generating the error messag.
	BadDeviceID(MIDIIOType type, UINT bad_id, UINT max_id)
		: MIDIEchoExcept(std::format(
			"Bad device ID passed to function: {}\n"
			"Device ID must be in the range [0 - {}]",
			bad_id, max_id
		), "BADID", MMSYSERR_BADDEVICEID, type, bad_id)
	{}
};

/// @brief gets thrown if the device id already is opened.
/// this can occur if another application already has control over the passed midi device.
/// MIDIEchoExcept::device_id:
///		the device id that has already been allocated.
/// 
/// MIDIEchoExcept::type:
///		the I/O type of the already allocated device.
class DeviceAllocated : public MIDIEchoExcept
{
public:
	DeviceAllocated(MIDIIOType type, UINT device_id)
		: MIDIEchoExcept(std::format(
			"Device has already been allocated\n"
			"ID:\t{}\n"
			"Name:\t'{}'",
			device_id,
			getMidiName(type, device_id)), "OCCUPIED", MMSYSERR_ALLOCATED, type, device_id)
	{}
};

/// @brief cast the correct exception, assuming err came from a function which handled input devices
/// with all the neccecarry information based on err.
/// if err is MMSYSERR_NOERROR, nothing happens.
void handleInputErr(MMRESULT err, UINT id);
/// @brief see handleInputErrMsg(), except the midi device is assumed to be an output device.
void handleOutputErr(MMRESULT err, UINT id);
