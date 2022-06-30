#include "EchoManager.h"

// TODO: check for optimizations / potential code cleanups.
void EchoManager::syncMidiDevices()
{
	// loop over all the connected midi inputs and remove / add any missing midi devices.

	std::map<std::string, bool> avaliable_devices;
	std::vector<std::string> new_devices;

	for (auto& [name, _] : m_midi_inputs)
		avaliable_devices[name] = false;

	for (UINT id = 0; id < midiInGetNumDevs(); id++)
	{
		std::string input_name = EchoMIDI::getMidiInputName(id);

		if (avaliable_devices.contains(input_name))
		{
			avaliable_devices[input_name] = true;

			// if device has become avaliable, check if it should be sending.

			MidiInProps& midi_input = m_midi_inputs[input_name];

			if (!midi_input.avaliable && midi_input.echo)
			{
				midi_input.echoer.open(EchoMIDI::getMidiInIDByName(input_name));
				midi_input.echoer.start();
				new_devices.push_back(input_name);
			}
		}
		else
		{
			// a new device was discovered.
			m_midi_inputs[input_name] = MidiInProps{ true, false };
			new_devices.push_back(input_name);
		}
	}

	// update the avaliability of all the devices.
	for (auto& [name, props] : m_midi_inputs)
	{
		if (avaliable_devices.contains(name))
		{

			props.avaliable = avaliable_devices[name];

			// device is no longer avaliable, stop echoing.

			if (!props.avaliable)
			{
				if (props.echoer.isEchoing())
					props.echoer.stop();

				if (props.echoer.isOpen())
					props.echoer.close();
			}
		}
	}

	// do the same for the midi outputs

	avaliable_devices.clear();

	for (auto& [name, _] : m_midi_outputs)
		avaliable_devices[name] = false;

	for (UINT id = 0; id < midiOutGetNumDevs(); id++)
	{
		std::string output_name = EchoMIDI::getMidiOutputName(id);

		if (avaliable_devices.contains(output_name))
		{
			avaliable_devices[output_name] = true;

			// if device has become avaliable, make sure all the Echoers have it as a target.

			MidiOutProps& midi_out_props = m_midi_outputs[output_name];

			if (!midi_out_props.avaliable)
			{
				for (auto& [input_name, _] : m_midi_inputs)
				{
					setTargetMute(output_name, input_name, midi_out_props.mute[input_name]);
					setTargetFocusSend(output_name, input_name, midi_out_props.focus_send[input_name]);
				}
			}
		}
		else
		{
			// a new device was discovered.
			m_midi_outputs[output_name] = MidiOutProps{ true };
		}
	}

	for (auto& [name, props] : m_midi_outputs)
	{
		if (avaliable_devices.contains(name))
		{
			props.avaliable = avaliable_devices[name];

			// device is no longer avaliable, remove it as a target from all the active echoers.

			if (!props.avaliable)
			{
				UINT device_id = EchoMIDI::getMidiOutIDByName(name);
				for (auto& [input_name, input_prop] : m_midi_inputs)
					if (input_prop.avaliable && input_prop.echoer.getTargets().contains(device_id))
						input_prop.echoer.remove(device_id);
			}
		}
	}

	// make sure all newly avaliable input devices have the corrent targets
	for (const std::string& new_input : new_devices)
		for (auto& [out_name, _] : m_midi_outputs)
			tryAddTarget(out_name, new_input);
}

void EchoManager::setInEcho(const std::string& name, bool val)
{
	if (m_midi_inputs[name].echo != val && m_midi_inputs[name].avaliable)
		if (val)
		{
			m_midi_inputs[name].echoer.open(EchoMIDI::getMidiInIDByName(name));
			m_midi_inputs[name].echoer.start();
		}
		else
		{
			m_midi_inputs[name].echoer.stop();
			m_midi_inputs[name].echoer.close();
		}

	m_midi_inputs[name].echo = val;
}

void EchoManager::setTargetMute(const std::string& target, const std::string& source, bool val)
{
	m_midi_outputs[target].mute[source] = val;

	tryAddTarget(target, source);

	UINT out_id = EchoMIDI::getMidiOutIDByName(target);

	if (m_midi_inputs[source].avaliable && m_midi_inputs[source].echo && m_midi_inputs[source].echoer.getTargets().contains(out_id))
		m_midi_inputs[source].echoer.setMute(out_id, val);
}

void EchoManager::setTargetFocusSend(const std::string& target, const std::string& source, const std::string& val)
{
	m_midi_outputs[target].focus_send[source] = val;

	tryAddTarget(target, source);

	UINT out_id = EchoMIDI::getMidiOutIDByName(target);

	if (m_midi_inputs[source].avaliable && m_midi_inputs[source].echo && m_midi_inputs[source].echoer.getTargets().contains(out_id))
		m_midi_inputs[source].echoer.focusSend(out_id, val);

}


void EchoManager::saveToFile(std::filesystem::path file)
{
}

void EchoManager::tryAddTarget(const std::string& target, const std::string& source)
{
	MidiInProps& in_props = m_midi_inputs[source];
	MidiOutProps& out_props = m_midi_outputs[target];

	UINT target_id = EchoMIDI::getMidiOutIDByName(target);

	if (in_props.avaliable && out_props.mute.contains(source) && !out_props.mute[source] && !in_props.echoer.getTargets().contains(target_id))
	{
		m_midi_inputs[source].echoer.add(target_id);
		m_midi_inputs[source].echoer.focusSend(target_id, m_midi_outputs[target].focus_send[source]);
		m_midi_inputs[source].echoer.setMute(target_id, false);
	}
}
