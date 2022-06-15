#include "Echoer.h"

#define BADOUTID(bad_id) BadDeviceID(MIDIIOType::INPUT, bad_id, midiOutGetNumDevs())
#define BADINID(bad_id) BadDeviceID(MIDIIOType::OUTPUT, bad_id, midiInGetNumDevs())


// gets the executable of the passed window
// returns an empty path if access is denied
//std::filesystem::path getHWNDPath(HWND window)
//{
//	DWORD win_pid;
//	WINERRB(GetWindowThreadProcessId(window, &win_pid));
//
//	WCHAR win_path[MAX_PATH_LENGTH];
//
//	HANDLE hProc = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, win_pid);
//
//
//	// not high enough privilages
//	if (hProc == NULL)
//		return "";
//	else
//		WINERRB(hProc);
//
//	DWORD len = MAX_PATH_LENGTH;
//	WINERRB(QueryFullProcessImageName(hProc, NULL, win_path, &len));
//
//	WINERRB(CloseHandle(hProc));
//
//	return win_path;
//}


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

void handleInputErr(MMRESULT err, UINT id)
{
	switch (err)
	{
	case MMSYSERR_NOERROR:
		return;
	case MMSYSERR_BADDEVICEID:
		throw BADINID(id);
	case MMSYSERR_ALLOCATED:
		throw DeviceAllocated(MIDIIOType::INPUT, id);
	default:
		throw MIDIEchoExcept(err, MIDIIOType::INPUT, id);
	}
}

void handleOutputErr(MMRESULT err, UINT id)
{
	switch (err)
	{
	case MMSYSERR_NOERROR:
		return;
	case MMSYSERR_BADDEVICEID:
		throw BADOUTID(id);
	case MMSYSERR_ALLOCATED:
		throw DeviceAllocated(MIDIIOType::OUTPUT, id);
	default:
		throw MIDIEchoExcept(err, MIDIIOType::OUTPUT, id);
	}
}

void CALLBACK midiCallback(HMIDIIN hMidiIn, UINT wMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2)
{
	Echoer* _this = (Echoer*)dwInstance;

	for (auto[id, midi_out] : _this->getTargets())
	{
		if(!midi_out.muted)
			handleOutputErr(midiOutShortMsg(midi_out.device_handle, (DWORD)dwParam1), id);
	}
}

Echoer::Echoer(UINT source)
	: m_midi_id(source)
{
	MMRESULT res = midiInOpen(&m_midi_source, m_midi_id, (DWORD_PTR)&midiCallback, (DWORD_PTR)this, CALLBACK_FUNCTION);
	
	handleInputErr(res, source);
}

Echoer::~Echoer()
{
	if (isEchoing())
		stop();

	MMRESULT res = midiInClose(m_midi_source);

	handleInputErr(res, m_midi_id);

	for (auto [id, target] : getTargets())
		handleOutputErr(midiOutClose(target.device_handle), id);
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

