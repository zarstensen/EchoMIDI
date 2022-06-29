#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include <wx/statline.h>
#include <wx/editlbox.h>
#include <wx/dataview.h>
#include <wx/grid.h>

#include <rapidjson/document.h>
#include <rapidjson/istreamwrapper.h>

#include <Windows.h>

#include <iostream>
#include <type_traits>
#include <fstream>

#include "Echoer.h"
#include "FocusHook.h"


#include "EchoManager.h"

#define print(str, ...) { auto _con_out = std::format(str, __VA_ARGS__); WriteConsoleA(GetStdHandle(STD_OUTPUT_HANDLE), _con_out.data(), _con_out.length(), NULL, NULL); }

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

/// @brief 
class MidiTable : public wxWindow
{
public:
	MidiTable(wxWindow* parent, EchoManager& manager)
		: wxWindow(parent, wxID_ANY), m_manager(manager)
	{
		// setup the data view list ctrl
		m_sizer->Add(m_listctrl, wxSizerFlags().Expand().Border(wxALL, 10));

		m_sizer->AddGrowableCol(0, 1);
		m_sizer->AddGrowableRow(0, 1);

		m_listctrl->Bind(wxEVT_SIZE, &MidiTable::OnResize, this);

		m_listctrl->SetAlternateRowColour(m_listctrl->GetBackgroundColour());

		SetSizer(m_sizer);
	}

	// on a resize event, resize the columns manually so they fill up the entire window width.
	void OnResize(wxSizeEvent& e)
	{
		int scroll_bar_width = (m_listctrl->GetScrollLines(wxVERTICAL) > 0) ? wxSystemSettings::GetMetric(wxSYS_VSCROLL_X) : 0;
		int col_width = (e.GetSize().x - m_listctrl->GetWindowBorderSize().x * 2 - scroll_bar_width) / m_listctrl->GetColumnCount();

		for (unsigned int i = 0; i < m_listctrl->GetColumnCount(); i++)
			m_listctrl->GetColumn(i)->SetWidth(col_width);

		e.Skip();
	}

	wxDataViewListCtrl* getList() { return m_listctrl; }

protected:
	EchoManager& m_manager;

	wxFlexGridSizer* m_sizer = new wxFlexGridSizer(1);
	wxDataViewListCtrl* m_listctrl = new wxDataViewListCtrl(this, wxID_ANY);
};

class MidiInputsTable : public MidiTable
{
public:
	MidiInputsTable(wxWindow* parent, EchoManager& manager)
		: MidiTable(parent, manager)
	{
		// setup the data view list ctrl
		m_listctrl->AppendTextColumn("MIDI Input Device", wxDATAVIEW_CELL_INERT, wxCOL_WIDTH_AUTOSIZE, wxALIGN_LEFT, wxDATAVIEW_COL_SORTABLE);
		m_listctrl->AppendTextColumn("Status", wxDATAVIEW_CELL_INERT, wxCOL_WIDTH_AUTOSIZE, wxALIGN_LEFT, wxDATAVIEW_COL_SORTABLE);
		m_listctrl->AppendToggleColumn("Echo", wxDATAVIEW_CELL_ACTIVATABLE, wxCOL_WIDTH_AUTOSIZE, wxALIGN_LEFT, wxDATAVIEW_COL_SORTABLE);

		m_listctrl->Bind(wxEVT_DATAVIEW_ITEM_VALUE_CHANGED, &MidiInputsTable::OnEchoToggle, this);
	}
	
	void updateTable()
	{
		// use this variable to remove any unavaliable devices currently shown in the table
		std::map<std::string, bool> avaliable_devices;

		for (auto& [input_name, input_props] : m_manager.getMidiInputs())
		{
			wxVector<wxVariant> row;

			// if there is a row witht he given input, it should be modified or removed.
			if (m_list_rows.contains(input_name) && input_props.avaliable)
			{
				m_listctrl->SetToggleValue(input_props.echo, m_listctrl->ItemToRow(m_list_rows[input_name]), 2);
			}
			else if (m_list_rows.contains(input_name) && !input_props.avaliable)
			{
				m_listctrl->DeleteItem(m_listctrl->ItemToRow(m_list_rows[input_name]));
				m_list_rows.erase(input_name);
			}
			else if (input_props.avaliable)
			{
				row.clear();
				row.reserve(3);

				row.push_back(input_name);
				row.push_back("Closed");
				row.push_back(input_props.echo);

				m_listctrl->AppendItem(row);

				m_list_rows[input_name] = m_listctrl->GetTopItem();
			}
		}
	}

protected:

	// store the modified data in the m_midi_inputs map
	void OnEchoToggle(wxDataViewEvent& e)
	{
		// only process the event if the column is the echo column
		if (e.GetColumn() == 2)
		{
			int row = m_listctrl->ItemToRow(e.GetItem());

			std::string device = m_listctrl->GetTextValue(row, 0).ToStdString();

			// any errors should be reported in the status column, TODO(and be logged in a log file)
			try
			{
				bool echo = m_listctrl->GetToggleValue(row, 2);
				m_manager.setInEcho(device, echo);

				m_listctrl->SetTextValue(echo ? "Open" : "Closed", row, 1);
			}
			catch (EchoMIDI::DeviceAllocated&)
			{
				m_listctrl->SetTextValue("[ERR] Already Opened", row, 1);
				m_listctrl->SetToggleValue(false, row, 2);
			}
			catch (EchoMIDI::MIDIEchoExcept&)
			{
				m_listctrl->SetTextValue("[ERR] Unknown", row, 1);
				m_listctrl->SetToggleValue(false, row, 2);
			}
		}
	}
	
	std::map<std::string, wxDataViewItem> m_list_rows;
};

class MidiOutputsTable : public MidiTable
{
public:
	MidiOutputsTable(wxWindow* parent, EchoManager& manager)
		: MidiTable(parent, manager)
	{
		// setup the data view list ctrl
		m_listctrl->AppendTextColumn("MIDI Output Device", wxDATAVIEW_CELL_INERT, wxCOL_WIDTH_AUTOSIZE, wxALIGN_LEFT, wxDATAVIEW_COL_SORTABLE);
		m_listctrl->AppendTextColumn("Focus Mute", wxDATAVIEW_CELL_EDITABLE, wxCOL_WIDTH_AUTOSIZE, wxALIGN_LEFT, wxDATAVIEW_COL_SORTABLE);
		m_listctrl->AppendToggleColumn("Send", wxDATAVIEW_CELL_ACTIVATABLE, wxCOL_WIDTH_AUTOSIZE, wxALIGN_LEFT, wxDATAVIEW_COL_SORTABLE);

		m_listctrl->Bind(wxEVT_DATAVIEW_ITEM_VALUE_CHANGED, &MidiOutputsTable::onSendToggle, this);
		m_listctrl->Bind(wxEVT_DATAVIEW_ITEM_EDITING_DONE, &MidiOutputsTable::onFocusMuteChange, this);
	}

	void updateTable()
	{
		if (getMidiInIDByName(m_active_source) != EchoMIDI::INVALID_MIDI_ID)
		{
			m_listctrl->Enable();

			// use this variable to remove any unavaliable devices currently shown in the table
			std::map<std::string, bool> avaliable_devices;

			for (auto& [output_name, output_props] : m_manager.getMidiOutputs())
			{
				wxVector<wxVariant> row;

				// if there is a row associated with the given input, it should be modified or removed.
				if (output_props.avaliable)
				{
					// make sure the output target has the required properties avaliable, if they are not present, they will be set to their default values..
					if (!output_props.mute.contains(m_active_source))
						m_manager.setTargetMute(output_name, m_active_source, true);

					if (!output_props.focus_mute.contains(m_active_source))
						m_manager.setTargetFocusMute(output_name, m_active_source, "");
					
					if (m_list_rows.contains(output_name))
					{
						auto test = m_listctrl->ItemToRow(m_list_rows[output_name]);
						m_listctrl->SetTextValue(output_props.focus_mute.at(m_active_source), m_listctrl->ItemToRow(m_list_rows[output_name]), 1);
						m_listctrl->SetToggleValue(!output_props.mute.at(m_active_source), m_listctrl->ItemToRow(m_list_rows[output_name]), 2);
					}
					else
					{
						row.clear();
						row.reserve(3);

						row.push_back(output_name);
						row.push_back(output_props.focus_mute.at(m_active_source));
						row.push_back(!output_props.mute.at(m_active_source));

						m_listctrl->AppendItem(row);

						m_list_rows[output_name] = m_listctrl->RowToItem(m_listctrl->GetItemCount() - 1);
					}
				}
				else if (m_list_rows.contains(output_name))
				{
					m_listctrl->DeleteItem(m_listctrl->ItemToRow(m_list_rows[output_name]));
					m_list_rows.erase(output_name);
				}
			}
		}
		else
		{
			m_listctrl->Disable();
			m_listctrl->Refresh();
		}
	}

	void setActiveSource(const std::string& name)
	{
		m_active_source = name;
	}

protected:
	// event handlers
	
	void onSendToggle(wxDataViewEvent& e)
	{
		// only process the event if the column is the send column
		if (e.GetColumn() == 2)
		{
			int row = m_listctrl->ItemToRow(e.GetItem());

			std::string device = m_listctrl->GetTextValue(row, 0).ToStdString();

			try
			{
				bool send = m_listctrl->GetToggleValue(row, 2);
				// the send value should have the opposite value of the mute value, hence the not operator on the send value.
				m_manager.setTargetMute(device, m_active_source, !send);
			}
			catch (EchoMIDI::MIDIEchoExcept&)
			{
				// TODO: do something here
			}
		}
	}

	void onFocusMuteChange(wxDataViewEvent& e)
	{
		int row = m_listctrl->ItemToRow(e.GetItem());

		std::string device = m_listctrl->GetTextValue(row, 0).ToStdString();
		
		try
		{
			m_manager.setTargetFocusMute(device, m_active_source, e.GetValue().GetString().ToStdString());
		}
		catch (EchoMIDI::MIDIEchoExcept&)
		{
			// TOOD: do something here aswell
		}
	}

	std::map<std::string, wxDataViewItem> m_list_rows;
	std::string m_active_source;
};


/// @brief main window holding the midi input and midi output frame, as well as holding the EchoManager instance.
class EchoMidiWindow : public wxWindow
{
public:
	EchoMidiWindow(
		wxWindow* parent,
		wxWindowID id,
		const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize,
		long style = 0,
		const wxString& name = wxASCII_STR(wxPanelNameStr))
		: wxWindow(parent, id, pos, size, style, name)
	{
		m_midi_input_frame = new wxStaticBoxSizer(wxVERTICAL, this, "MIDI Inputs");
		m_midi_inputs = new MidiInputsTable(m_midi_input_frame->GetStaticBox(), m_manager);

		m_midi_output_frame = new wxStaticBoxSizer(wxVERTICAL, this, "MIDI Outputs [NONE]");
		m_midi_outputs = new MidiOutputsTable(m_midi_output_frame->GetStaticBox(), m_manager);

		m_manager.syncMidiDevices();

		// setup sizer
		m_sizer = new wxBoxSizer(wxVERTICAL);

		m_midi_input_frame->Add(m_midi_inputs, wxSizerFlags(1).Expand().Border(wxALL, 10));
		m_midi_output_frame->Add(m_midi_outputs, wxSizerFlags(1).Expand().Border(wxALL, 10));

		m_sizer->Add(m_midi_input_frame, wxSizerFlags(1).Expand().Border(wxALL, 10));
		m_sizer->Add(m_midi_output_frame, wxSizerFlags(1).Expand().Border(wxALL, 10));

		m_midi_inputs->updateTable();
		m_midi_outputs->updateTable();

		// setup events

		m_midi_inputs->getList()->Bind(wxEVT_DATAVIEW_ITEM_ACTIVATED, &EchoMidiWindow::onInputSelect, this);

		SetSizerAndFit(m_sizer);
	}

	void onInputSelect(wxDataViewEvent& e)
	{
		std::string new_source = m_midi_inputs->getList()->GetTextValue(m_midi_inputs->getList()->ItemToRow(e.GetItem()), 0).ToStdString();
		// update the active input source, when the user has activated a new row.
		m_midi_outputs->setActiveSource(new_source);
		m_midi_outputs->updateTable();

		m_midi_output_frame->GetStaticBox()->SetLabelText(
			std::format(NAME_FORMAT_STRING, EchoMIDI::getMidiInIDByName(new_source) != EchoMIDI::INVALID_MIDI_ID ? new_source : "NONE"));
	}

private:

	constexpr static const char* NAME_FORMAT_STRING = "MIDI Outputs [{}]";

	EchoManager m_manager;

	// wxWidgets
	wxBoxSizer* m_sizer;

	wxStaticBoxSizer* m_midi_input_frame;
	MidiInputsTable* m_midi_inputs;
	
	wxStaticBoxSizer* m_midi_output_frame;
	MidiOutputsTable* m_midi_outputs;
};

/// @brief simply creates a frame containing the EchoMidiWIndow.
class EchoMidiApp : public wxApp
{
	wxFrame* ui_frame;
	EchoMidiWindow* main_window;

	wxSize size = { 800, 400 };

	EchoManager manager;
	
	virtual bool OnInit() override
	{
		EchoMIDI::EchoMIDIInit();

	#ifdef _DEBUG
		// allocate console for debugging output
		AllocConsole();
	#endif

		ui_frame = new wxFrame(NULL, wxID_ANY, "Echo MIDI", wxDefaultPosition, size);
		ui_frame->SetMinSize(size);

		main_window = new EchoMidiWindow(ui_frame, wxID_ANY);

		ui_frame->Show(true);

		return true;
	}

	virtual int OnExit() override
	{
		EchoMIDI::EchoMIDICleanup();

		return 0;
	}
};

wxIMPLEMENT_APP(EchoMidiApp);
