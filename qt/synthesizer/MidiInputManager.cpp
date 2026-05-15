/* Written by Codex. */
#include <vector>

#include <QDebug>

#include "RtMidi.h"

#include "MidiInputManager.h"

/**********************************************************************************/
static uint32_t MakeShortMidiMessage(std::vector<unsigned char> const &message)
{
	uint32_t midi_message = 0;
	if(message.size() > 0){
		midi_message |= (uint32_t)message[0];
	}
	if(message.size() > 1){
		midi_message |= ((uint32_t)message[1] << 8);
	}
	if(message.size() > 2){
		midi_message |= ((uint32_t)message[2] << 16);
	}
	return midi_message;
}

/**********************************************************************************/
MidiInputManager::MidiInputManager(QObject *parent)
	: QObject(parent)
	, m_p_midi_in(nullptr)
{
	try {
		m_p_midi_in = new RtMidiIn();
		m_p_midi_in->ignoreTypes(true, false, true);
	}
	catch(RtMidiError &error){
		qWarning() << "RtMidiIn initialization failed:" << error.what();
		m_p_midi_in = nullptr;
	}
}

/**********************************************************************************/
MidiInputManager::~MidiInputManager(void)
{
	ClosePort();
	delete m_p_midi_in;
	m_p_midi_in = nullptr;
}

/**********************************************************************************/
QStringList MidiInputManager::GetPortNameList(void) const
{
	QStringList port_name_list;
	if(nullptr == m_p_midi_in){
		return port_name_list;
	}

	unsigned int const port_count = m_p_midi_in->getPortCount();
	for(unsigned int i = 0; i < port_count; ++i){
		try {
			port_name_list << QString::fromStdString(m_p_midi_in->getPortName(i));
		}
		catch(RtMidiError &error){
			port_name_list << QString("RtMidi port %1 name error: %2").arg(i).arg(error.what());
		}
	}
	return port_name_list;
}

/**********************************************************************************/
bool MidiInputManager::OpenPort(unsigned int const port_index)
{
	if(nullptr == m_p_midi_in){
		return false;
	}

	try {
		if(true == m_p_midi_in->isPortOpen()){
			m_p_midi_in->closePort();
		}
		m_p_midi_in->openPort(port_index);
		m_p_midi_in->setCallback(&MidiInputManager::HandleRtMidiMessage, this);
		qInfo() << "Opened MIDI input port" << port_index
				<< QString::fromStdString(m_p_midi_in->getPortName(port_index));
		return true;
	}
	catch(RtMidiError &error){
		qWarning() << "Opening MIDI input port failed:" << error.what();
		return false;
	}
}

/**********************************************************************************/
#ifndef Q_OS_WIN
bool MidiInputManager::OpenVirtualPort(QString const &port_name)
{
	if(nullptr == m_p_midi_in){
		return false;
	}

	try {
		if(true == m_p_midi_in->isPortOpen()){
			m_p_midi_in->closePort();
		}
		m_p_midi_in->openVirtualPort(port_name.toStdString());
		m_p_midi_in->setCallback(&MidiInputManager::HandleRtMidiMessage, this);
		qInfo() << "Opened virtual MIDI input port" << port_name;
		return true;
	}
	catch(RtMidiError &error){
		qWarning() << "Opening virtual MIDI input port failed:" << error.what();
		return false;
	}
}
#endif

/**********************************************************************************/
void MidiInputManager::ClosePort(void)
{
	if(nullptr == m_p_midi_in){
		return;
	}

	try {
		m_p_midi_in->cancelCallback();
		if(true == m_p_midi_in->isPortOpen()){
			m_p_midi_in->closePort();
		}
	}
	catch(RtMidiError &error){
		qWarning() << "Closing MIDI input port failed:" << error.what();
	}
}

/**********************************************************************************/
void MidiInputManager::HandleRtMidiMessage(double, std::vector<unsigned char> *p_message, void *p_user_data)
{
	if((nullptr == p_message) || (nullptr == p_user_data)){
		return;
	}

	MidiInputManager * const p_midi_input_manager = static_cast<MidiInputManager*>(p_user_data);
	p_midi_input_manager->HandleMessage(*p_message);
}

/**********************************************************************************/
void MidiInputManager::HandleMessage(std::vector<unsigned char> const &message)
{
	if((message.size() < 1) || (message.size() > 3)){
		return;
	}

	emit MidiMessageReceived(MakeShortMidiMessage(message));
}
