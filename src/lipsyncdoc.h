#ifndef LIPSYNCDOC_H
#define LIPSYNCDOC_H

#include <QMediaPlayer>

#include "audioextractor.h"

#include <cstdint>

class QFile;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class LipsyncPhoneme
{
public:
	LipsyncPhoneme();
	~LipsyncPhoneme();

	QString		fText;
	std::int32_t		fFrame;
	std::int32_t		fTop, fBottom;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class LipsyncWord
{
public:
	LipsyncWord();
	~LipsyncWord();

	void RunBreakdown(QString language);
	void RepositionPhoneme(LipsyncPhoneme *phoneme);

	QString					fText;
	std::int32_t					fStartFrame, fEndFrame;
	std::int32_t					fTop, fBottom;
	QList<LipsyncPhoneme *>	fPhonemes;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class LipsyncPhrase
{
public:
	LipsyncPhrase();
	~LipsyncPhrase();

	void RunBreakdown(QString language);
	void RepositionWord(LipsyncWord *word);

	QString					fText;
	std::int32_t					fStartFrame, fEndFrame;
	std::int32_t					fTop, fBottom;
	QList<LipsyncWord *>	fWords;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class LipsyncVoice
{
public:
	LipsyncVoice(const QString &name);
	~LipsyncVoice();

	void Open(QTextStream &in);
	void Save(QTextStream &out);
	void Export(QString path);
	void RunBreakdown(QString language, std::int32_t audioDuration);
	void RepositionPhrase(LipsyncPhrase *phrase, std::int32_t audioDuration);
	QString GetPhonemeAtFrame(std::int32_t frame);

	QString					fName;
	QString					fText;
	QList<LipsyncPhrase *>	fPhrases;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class LipsyncDoc : public QObject
{
	Q_OBJECT
public:
	LipsyncDoc();
	~LipsyncDoc();

	static void LoadDictionaries();

	void Open(const QString &path);
	void OpenAudio(const QString &path);
	void Save();
	void RebuildAudioSamples();

	std::int32_t Fps() { return fFps; }
	void SetFps(std::int32_t fps);
	QMediaPlayer *GetAudioPlayer();
	AudioExtractor *GetAudioExtractor();
	std::int32_t Duration() { return fAudioDuration; }
	QString GetVolumePhonemeAtFrame(std::int32_t frame);

private slots:

private:
	static void LoadDictionary(QFile *f);

	std::int32_t					fFps, fAudioDuration;
	QString					fAudioPath;
	QMediaPlayer			*fAudioPlayer;
	AudioExtractor			*fAudioExtractor;
	real					fMaxAmplitude;

public:
	QString					fPath;
	bool					fDirty;
	QList<LipsyncVoice *>	fVoices;
	LipsyncVoice			*fCurrentVoice;

	static QList<QString>				Phonemes;
	static QHash<QString, QString>		DictionaryToPhonemeMap;
	static QHash<QString, QStringList>	PhonemeDictionary;

	/*
	 * I would have preferred to use a QAudioDecoder object, but it doesn't seem to actually be implemented (at least on Mac).
	*/
};

#endif // LIPSYNCDOC_H
