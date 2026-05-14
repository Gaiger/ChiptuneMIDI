/* Written by Codex. */
#ifndef _MIDIINPUTMANAGER_H_
#define _MIDIINPUTMANAGER_H_

#include <stdint.h>

#include <vector>

#include <QObject>
#include <QStringList>

class RtMidiIn;

class MidiInputManager : public QObject
{
	Q_OBJECT
public:
	explicit MidiInputManager(QObject *parent = nullptr);
	~MidiInputManager(void) Q_DECL_OVERRIDE;

	QStringList GetPortNameList(void) const;
	bool OpenPort(unsigned int const port_index);
	void ClosePort(void);

signals:
	void MidiMessageReceived(uint32_t const midi_message);

private:
	static void HandleRtMidiMessage(double time_stamp, std::vector<unsigned char> *p_message, void *p_user_data);
	void HandleMessage(std::vector<unsigned char> const &message);

private:
	RtMidiIn *m_p_midi_in;
};

#endif // _MIDIINPUTMANAGER_H_
