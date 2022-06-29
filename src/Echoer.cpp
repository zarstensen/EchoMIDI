#include "Echoer.h"
#include "FocusHook.h"

namespace EchoMIDI
{
	// ============ Local defines ============

#define BADOUTID(bad_id) BadDeviceID(MIDIIOType::INPUT, bad_id, midiOutGetNumDevs())
#define BADINID(bad_id) BadDeviceID(MIDIIOType::OUTPUT, bad_id, midiInGetNumDevs())

// cast the correct exception, assuming err came from a function which handled input devices
// with all the neccecarry information based on err.
// if err is MMSYSERR_NOERROR, nothing happens.
	void handleInputErr(MMRESULT err, UINT id)
	{
		try
		{
			switch (err)
			{
			case MMSYSERR_NOERROR:
				return;
			case MMSYSERR_BADDEVICEID:
				throw BADINID(id);
			case MMSYSERR_NOMEM:
				throw DeviceAllocated(MIDIIOType::INPUT, id);
			case MMSYSERR_ALLOCATED:
				throw DeviceAllocated(MIDIIOType::INPUT, id);
			default:
				throw MIDIEchoExcept(err, MIDIIOType::INPUT, id);
			}
		}
		catch (...)
		{
			// the id is invalid
			if (id == INVALID_MIDI_ID)
				std::throw_with_nested(BADINID(id));
			else
				throw;
		}
	}

	// see handleInputErrMsg(), except the midi device is assumed to be an output device.
	void handleOutputErr(MMRESULT err, UINT id)
	{
		try
		{
			switch (err)
			{
			case MMSYSERR_NOERROR:
				return;
			case MMSYSERR_BADDEVICEID:
				throw BADOUTID(id);
			case MMSYSERR_NOMEM:
				throw DeviceAllocated(MIDIIOType::OUTPUT, id);
			case MMSYSERR_ALLOCATED:
				throw DeviceAllocated(MIDIIOType::OUTPUT, id);
			default:
				throw MIDIEchoExcept(err, MIDIIOType::OUTPUT, id);
			}
		}
		catch (...)
		{
			// the id is invalid
			if (id == INVALID_MIDI_ID)
				std::throw_with_nested(BADOUTID(id));
			else
				throw;
		}
	}

	// ============ Header defines ============

	bool isValidInID(UINT id)
	{
		return id < midiInGetNumDevs();
	}

	bool isValidOutID(UINT id)
	{
		return id < midiOutGetNumDevs();
	}

	std::string getMidiInputName(UINT midi_in_id)
	{
		MIDIINCAPS midi_info;

		handleInputErr(midiInGetDevCaps(midi_in_id, &midi_info, sizeof(MIDIINCAPS)), midi_in_id);

		return midi_info.szPname;
	}

	std::string getMidiOutputName(UINT midi_out_id)
	{
		MIDIOUTCAPS midi_info;

		handleOutputErr(midiOutGetDevCaps(midi_out_id, &midi_info, sizeof(MIDIOUTCAPS)), midi_out_id);

		return midi_info.szPname;
	}

	UINT getMidiInIDByName(const std::string& name)
	{
		if (name.size() <= MAXPNAMELEN)
		{
			for (UINT i = 0; i < midiInGetNumDevs(); i++)
			{
				if (name == getMidiInputName(i))
					return i;
			}
		}

		return UINT_MAX;
	}

	UINT getMidiOutIDByName(const std::string& name)
	{
		if (name.size() <= MAXPNAMELEN)
		{
			for (UINT i = 0; i < midiOutGetNumDevs(); i++)
			{
				if (name == getMidiOutputName(i))
					return i;
			}
		}

		return UINT_MAX;
	}

	std::string getMidiName(MIDIIOType midi_io_type, UINT midi_id)
	{
		switch (midi_io_type)
		{
		case MIDIIOType::INPUT:
			return getMidiInputName(midi_id);
		case MIDIIOType::OUTPUT:
			return getMidiOutputName(midi_id);
		default:
			return "";
		}
	}

	void CALLBACK midiCallback(HMIDIIN hMidiIn, UINT wMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2)
	{
		Echoer* _this = (Echoer*)dwInstance;

		// Only midi data should be sent to the outputs.
		if (wMsg == MIM_DATA)
		{
			for (auto [id, midi_out] : _this->getTargets())
			{
				if (!(midi_out.user_muted || midi_out.focus_muted))
					handleOutputErr(midiOutShortMsg(midi_out.device_handle, (DWORD)dwParam1), id);

			}
		}
		else if (wMsg == MIM_LONGDATA)
		{
			for (auto [id, midi_out] : _this->getTargets())
			{
				if (!(midi_out.user_muted || midi_out.focus_muted))
					handleOutputErr(midiOutLongMsg(midi_out.device_handle, (LPMIDIHDR)dwParam1, (UINT)dwParam2), id);
			}
		}
	}

	// ============ Echoer ============

	Echoer::Echoer()
		: m_midi_id(INVALID_MIDI_ID)
	{
		registerEchoer(this);
	}

	Echoer::Echoer(UINT source)
		: m_midi_id(source)
	{
		registerEchoer(this);
	}

	Echoer::~Echoer()
	{
		if (isEchoing())
			stop();
		
		if(isOpen())
			close();

		for (auto [id, target] : getTargets())
		{
			handleOutputErr(midiOutReset(target.device_handle), id);
			handleOutputErr(midiOutClose(target.device_handle), id);
		}

		unregisterEchoer(this);
	}

	bool Echoer::add(UINT id)
	{
		MMRESULT res = midiOutOpen(&m_midi_targets[id].device_handle, id, NULL, NULL, CALLBACK_NULL);

		if (res != MMSYSERR_NOERROR)
			m_midi_targets.erase(id);

		handleOutputErr(res, id);

		return res == MMSYSERR_NOERROR;
	}

	void Echoer::remove(UINT id)
	{
		if (m_midi_targets.contains(id))
		{
			handleInputErr(midiOutClose(m_midi_targets[id].device_handle), id);
			m_midi_targets.erase(id);
		}
	}

	void Echoer::open(UINT id)
	{
		if (isOpen())
			throw MIDIEchoExcept("Cannot open an already open Echoer", "Open Err", NULL, MIDIIOType::INPUT, m_midi_id);

		handleInputErr(midiInOpen(&m_midi_source, id, (DWORD_PTR)&midiCallback, (DWORD_PTR)this, CALLBACK_FUNCTION), id);
		
		m_midi_id = id;
		m_is_open = true;
	}

	void Echoer::open()
	{
		open(m_midi_id);
	}

	void Echoer::close()
	{
		if (isEchoing())
			throw MIDIEchoExcept("Cannot close midi device if it is currently echoing", "Close Err", NULL, MIDIIOType::INPUT, m_midi_id);

		handleInputErr(midiInReset(m_midi_source), m_midi_id);
		handleInputErr(midiInClose(m_midi_source), m_midi_id);

		m_is_open = false;
	}

	void Echoer::reset()
	{
		handleOutputErr(midiInReset(m_midi_source), m_midi_id);
	}

	void Echoer::start()
	{
		m_is_echoing = true;
		handleOutputErr(midiInStart(m_midi_source), m_midi_id);
	}

	void Echoer::stop()
	{
		m_is_echoing = false;
		handleOutputErr(midiInStop(m_midi_source), m_midi_id);
	}

	void Echoer::focusMute(UINT id, std::filesystem::path exec)
	{
		if (!m_midi_targets.contains(id))
		{
			throw BADOUTID(id);
			return;
		}

		// as a focus event does not occur when this is called, the top window needs to be retrieved manually.
		// if the GUI is used, this will almost always be false, as the GUI window always will be in focus, when this is called.
		if (exec != "")
			m_midi_targets[id].focus_muted = exec != getHWNDPath(GetTopWindow(NULL));
		else
			m_midi_targets[id].focus_muted = false;

		m_midi_targets[id].focus_mute_path = exec;
	}
}
