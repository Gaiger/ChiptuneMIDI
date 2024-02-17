/*
 *
 * Copyright 2003-2012 by David G. Slomin
 * Copyright 2012-2016 Augustin Cavalier <waddlesplash>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <QtGlobal>
#if defined( Q_OS_WIN )
#include <windows.h>
#endif

#include <stdio.h>
#include <QThread>
#include <QElapsedTimer>
#include <QCoreApplication>
#include <QDebug>

#include <QMidiOut.h>
#include <QMidiFile.h>
#include "MidiPlayer.h"

#include "TuneManager.h"
#include "AudioPlayer.h"

static void ListAvailableMidiDevices(void)
{
	QMap<QString, QString> vals = QMidiOut::devices();
	QList<QString> key_list = vals.keys();
	for(QString & key : key_list) {
		QString value = vals.value(key);
		fputs(key.toUtf8().constData(), stderr);
		fputs("\t", stderr);
		fputs(value.toUtf8().constData(), stderr);
		fputs("\n", stderr);
	}
}

/**********************************************************************************/

struct wav_header_t
{
	char chunk_id[4];							// RIFF string
	unsigned int chunk_size;					// overall size of file in bytes
	char format[4];								// WAVE string
	char subchunk1_id[4];						// fmt string with trailing null char
	unsigned int subchunk1_size;				// length of the format data as listed above
	unsigned short audio_format;					// format type. 1-PCM, 3- IEEE float, 6 - 8bit A law, 7 - 8bit mu law
	unsigned short number_of_channels;			// no.of channels
	unsigned int sampling_rate;					// sampling rate (blocks per second)
	unsigned int byte_rate;						// SampleRate * NumChannels * BitsPerSample/8
	unsigned short block_align;					// NumChannels * BitsPerSample/8
	unsigned short bits_per_sample;				// bits per sample, 8- 8bits, 16- 16 bits etc
	char subchunk2_id[4];						// DATA string or FLLR string
	unsigned int subchunk2_size;				// NumSamples * NumChannels * BitsPerSample/8 - size of the next chunk that will be read
};

static struct wav_header_t s_wave_header;

unsigned char const * const wav_file_header(unsigned int const number_of_channels, unsigned int const sampling_rate,
											unsigned int bits_per_sample, unsigned int wave_data_length,
											int * const p_this_header_size)
{
	s_wave_header.chunk_id[0] = 'R';
	s_wave_header.chunk_id[1] = 'I';
	s_wave_header.chunk_id[2] = 'F';
	s_wave_header.chunk_id[3] = 'F';

	s_wave_header.format[0] = 'W';
	s_wave_header.format[1] = 'A';
	s_wave_header.format[2] = 'V';
	s_wave_header.format[3] = 'E';

	s_wave_header.subchunk1_id[0] = 'f';
	s_wave_header.subchunk1_id[1] = 'm';
	s_wave_header.subchunk1_id[2] = 't';
	s_wave_header.subchunk1_id[3] = ' ';

	s_wave_header.subchunk2_id[0] = 'd';
	s_wave_header.subchunk2_id[1] = 'a';
	s_wave_header.subchunk2_id[2] = 't';
	s_wave_header.subchunk2_id[3] = 'a';

	s_wave_header.subchunk1_size = 16;
	s_wave_header.audio_format = 1;
	s_wave_header.number_of_channels = number_of_channels;
	s_wave_header.sampling_rate = sampling_rate;
	s_wave_header.byte_rate = number_of_channels * sampling_rate * bits_per_sample / 8;
	s_wave_header.block_align = number_of_channels * bits_per_sample / 8;
	s_wave_header.bits_per_sample = bits_per_sample;

	s_wave_header.subchunk2_size = wave_data_length;
	s_wave_header.chunk_size = wave_data_length + (sizeof(struct wav_header_t) - 8);

	*p_this_header_size = sizeof(struct wav_header_t);
	return (unsigned char const * const)&s_wave_header;
}

/**********************************************************************************/
#include <QElapsedTimer>
#include <QFile>

int SaveAsWavFile(TuneManager *p_tune_manager, QString filename)
{
	if(false == filename.endsWith(".wav", Qt::CaseInsensitive)){
		filename.append(".wav");
	}
	qDebug() << "saving as wav file " << filename;

	QElapsedTimer elasped_timer;

	elasped_timer.start();
	p_tune_manager->InitializeTune();
	int data_buffer_size = p_tune_manager->GetNumberOfChannels()
			* p_tune_manager->GetSamplingRate() * p_tune_manager->GetSamplingSize()/8;
	QByteArray wave_data;
	while(1)
	{
		wave_data += p_tune_manager->FetchWave(data_buffer_size);
		if(true == p_tune_manager->IsTuneEnding()){
			break;
		}
	}
	qDebug() << "Generate wave data elpased" << elasped_timer.elapsed() << "ms";
	int header_size = 0;
	char *p_wav_header = (char*)wav_file_header(
				p_tune_manager->GetNumberOfChannels(), p_tune_manager->GetSamplingRate(),
				p_tune_manager->GetSamplingSize(), wave_data.size(), &header_size);

	QFile file(filename);
	if(false == file.open(QIODevice::ReadWrite | QIODevice::Truncate)){
		qDebug() << "open file fail";
		return -2;
	}

	int file_size = file.write(QByteArray(p_wav_header, header_size) + wave_data);
	file.close();

	qDebug() << "file saved, size = " << file_size << " bytes";
	return 0;
}

void PilotRun(TuneManager *p_tune_manager)
{
	QElapsedTimer elasped_timer;

	elasped_timer.start();
	p_tune_manager->InitializeTune();
	int data_buffer_size = p_tune_manager->GetNumberOfChannels()
			* p_tune_manager->GetSamplingRate() * p_tune_manager->GetSamplingSize()/8;
	QByteArray wave_data;
	while(1)
	{
		wave_data = p_tune_manager->FetchWave(data_buffer_size);
		if(true == p_tune_manager->IsTuneEnding()){
			break;
		}
	}
	qDebug() <<Q_FUNC_INFO << " elpased" << elasped_timer.elapsed() << "ms";

}
/**********************************************************************************/

int main(int argc, char* argv[])
{
#if defined( Q_OS_WIN )
	if (AttachConsole(ATTACH_PARENT_PROCESS) || AllocConsole()){
		freopen("CONOUT$", "w", stdout);
		freopen("CONOUT$", "w", stderr);
		freopen("CONIN$", "r", stdin);
	}
	setvbuf(stdout, NULL, _IONBF, 0);
#endif

	QCoreApplication a(argc, argv);

	//QString filename = "8bit(bpm185)v0727T1.mid";
	QString filename = "totoro.mid";
	//QString filename = "evil_eye.mid";
	//QString filename = "black_star.mid";
	//QString filename = "crying.mid";
	//QString filename = "triligy.mid";
	//QString filename = "6Oclock.mid";
	//QString filename = "requiem.mid";
	//QString filename = "Laputa.mid";
	//QString filename = "Ironforge.mid";
	//QString filename = "23401.mid";
	//QString filename = "never_enough.mid";

	//QString filename = "67573.mid";
	//QString filename = "79538.mid";

	//QString filename = "2012111316917146.mid";
	//QString filename = "25626oth34.mid";
	//QString filename = "duck.mid";
	//QString filename = "201211212129826.mid";
	//QString filename = "pray_for_buddha.mid";
	//QString filename = "102473.mid";
	//QString filename = "korobeiniki.mid";
	//QString filename = "nekrasov-korobeiniki-tetris-theme-20230417182739.mid";
#if(0)
	ListAvailableMidiDevices();
	QString midiOutName = "0";
	QMidiFile* midi_file = new QMidiFile();
	midi_file->load(filename);

	QMidiOut* midi_out = new QMidiOut();
	midi_out->connect(midiOutName);

	MidiPlayer* p_player = new MidiPlayer(midi_file, midi_out, &a);
	QObject::connect(p_player, &MidiPlayer::Finished, &a,  &QCoreApplication::quit);
	//p_player->moveToThread(a.thread());
	p_player->Play();
#endif

#if(1)
	TuneManager tune_manager(true, 16000, 8);
	QThread tune_manager_working_thread;
	tune_manager.moveToThread(&tune_manager_working_thread);
	tune_manager_working_thread.start(QThread::HighPriority);
	tune_manager.SetMidiFile(filename);
	//PilotRun(&tune_manager);
	//SaveAsWavFile(&tune_manager, "20240206ankokuButo.wav");
	//SaveAsWavFile(&tune_manager, "20240205Laputa.wav");
	//SaveAsWavFile(&tune_manager, "20240205Ironforge.wav");
	//SaveAsWavFile(&tune_manager, "202402156Oclock.wav");
	//SaveAsWavFile(&tune_manager, "20240215never_enough.wav");
	//SaveAsWavFile(&tune_manager, "202402016pray_for_buddha.wav");
	AudioPlayer audio_player(&tune_manager, &a);
	audio_player.Play();
#endif
	return a.exec();
}
