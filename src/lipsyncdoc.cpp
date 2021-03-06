#include <QFile>
#include <QFileInfo>
#include <QDir>

#include "lipsyncdoc.h"

LipsyncPhoneme::LipsyncPhoneme()
{
	fText = "";
	fFrame = 0;
	fTop = fBottom = 0;
}

LipsyncPhoneme::~LipsyncPhoneme()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

LipsyncWord::LipsyncWord()
{
	fText = "";
	fStartFrame = 0;
	fEndFrame = 0;
	fTop = fBottom = 0;
}

LipsyncWord::~LipsyncWord()
{
	while (!fPhonemes.isEmpty())
		delete fPhonemes.takeFirst();
}

void LipsyncWord::RunBreakdown(QString language)
{
	while (!fPhonemes.isEmpty())
		delete fPhonemes.takeFirst();

	QString		text = fText;
	text.remove(QRegExp("[.,!?;-/()¿]"));
	QStringList	pronunciation;
	if (language == "EN")
	{
		pronunciation = LipsyncDoc::PhonemeDictionary.value(text.toUpper());
		if (pronunciation.size() > 1)
		{
			for (std::int32_t i = 1; i < pronunciation.size(); i++)
			{
				QString p = pronunciation.at(i);
				if (p.length() == 0)
					continue;
				LipsyncPhoneme *phoneme = new LipsyncPhoneme;
				phoneme->fText = LipsyncDoc::DictionaryToPhonemeMap.value(p, "etc");
				fPhonemes << phoneme;
			}
		}
	}
}

void LipsyncWord::RepositionPhoneme(LipsyncPhoneme *phoneme)
{
	int id = fPhonemes.indexOf(phoneme);

	if ((id > 0) && (phoneme->fFrame < fPhonemes[id - 1]->fFrame + 1))
		phoneme->fFrame = fPhonemes[id - 1]->fFrame + 1;
	if ((id < fPhonemes.size() - 1) && (phoneme->fFrame > fPhonemes[id + 1]->fFrame - 1))
		phoneme->fFrame = fPhonemes[id + 1]->fFrame - 1;
	if (phoneme->fFrame < fStartFrame)
		phoneme->fFrame = fStartFrame;
	if (phoneme->fFrame > fEndFrame)
		phoneme->fFrame = fEndFrame;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

LipsyncPhrase::LipsyncPhrase()
{
	fText = "";
	fStartFrame = 0;
	fEndFrame = 0;
	fTop = fBottom = 0;
}

LipsyncPhrase::~LipsyncPhrase()
{
	while (!fWords.isEmpty())
		delete fWords.takeFirst();
}

void LipsyncPhrase::RunBreakdown(QString language)
{
	// break phrase into words
	while (!fWords.isEmpty())
		delete fWords.takeFirst();
	QStringList strList = fText.split(' ', QString::SkipEmptyParts);
	for (std::int32_t i = 0; i < strList.size(); i++)
	{
		if (strList.at(i).length() == 0)
			continue;
		LipsyncWord *word = new LipsyncWord;
		word->fText = strList.at(i);
		fWords << word;
	}

	// now break down the words
	for (std::int32_t i = 0; i < fWords.size(); i++)
		fWords[i]->RunBreakdown(language);
}

void LipsyncPhrase::RepositionWord(LipsyncWord *word)
{
	int id = fWords.indexOf(word);

	if ((id > 0) && (word->fStartFrame < fWords[id - 1]->fEndFrame + 1))
	{
		word->fStartFrame = fWords[id - 1]->fEndFrame + 1;
		if (word->fEndFrame < word->fStartFrame + 1)
			word->fEndFrame = word->fStartFrame + 1;
	}
	if ((id < fWords.size() - 1) && (word->fEndFrame > fWords[id + 1]->fStartFrame - 1))
	{
		word->fEndFrame = fWords[id + 1]->fStartFrame - 1;
		if (word->fStartFrame > word->fEndFrame - 1)
			word->fStartFrame = word->fEndFrame - 1;
	}
	if (word->fStartFrame < fStartFrame)
		word->fStartFrame = fStartFrame;
	if (word->fEndFrame > fEndFrame)
		word->fEndFrame = fEndFrame;
	if (word->fEndFrame < word->fStartFrame)
		word->fEndFrame = word->fStartFrame;

	// now divide up the total time by phonemes
	std::int32_t frameDuration = word->fEndFrame - word->fStartFrame + 1;
	std::int32_t phonemeCount = word->fPhonemes.size();
	float framesPerPhoneme = 1.0f;
	if (frameDuration > 0 && phonemeCount > 0)
	{
        framesPerPhoneme = static_cast<float>(frameDuration) / static_cast<float>(phonemeCount);
		if (framesPerPhoneme < 1.0f)
			framesPerPhoneme = 1.0f;
	}

	// finally, assign frames based on phoneme durations
	float curFrame = word->fStartFrame;
	for (std::int32_t i = 0; i < word->fPhonemes.size(); i++)
	{
        word->fPhonemes[i]->fFrame = pg_math::round(curFrame);
		curFrame = curFrame + framesPerPhoneme;
	}
	for (std::int32_t i = 0; i < word->fPhonemes.size(); i++)
	{
		word->RepositionPhoneme(word->fPhonemes[i]);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

LipsyncVoice::LipsyncVoice(const QString &name)
{
	fName = name;
}

LipsyncVoice::~LipsyncVoice()
{
	while (!fPhrases.isEmpty())
		delete fPhrases.takeFirst();
}

void LipsyncVoice::Open(QTextStream &in)
{
	std::int32_t		numPhrases, numWords, numPhonemes;
	QString		str;

	fName = in.readLine().trimmed();
	fText = in.readLine().trimmed();
	fText = fText.split('|').join('\n');

	numPhrases = in.readLine().toInt();
	for (int p = 0; p < numPhrases; p++)
	{
		LipsyncPhrase *phrase = new LipsyncPhrase;
		phrase->fText = in.readLine().trimmed();
		phrase->fStartFrame = in.readLine().toInt();
		phrase->fEndFrame = in.readLine().toInt();
		numWords = in.readLine().toInt();
		for (int w = 0; w < numWords; w++)
		{
			LipsyncWord *word = new LipsyncWord;
			str = in.readLine().trimmed();
			QStringList strList = str.split(' ', QString::SkipEmptyParts);
			if (strList.size() >= 4)
			{
				word->fText = strList.at(0);
				word->fStartFrame = strList.at(1).toInt();
				word->fEndFrame = strList.at(2).toInt();
				numPhonemes = strList.at(3).toInt();
			}
			for (int ph = 0; ph < numPhonemes; ph++)
			{
				LipsyncPhoneme *phoneme = new LipsyncPhoneme;
				str = in.readLine().trimmed();
				QStringList strList = str.split(' ', QString::SkipEmptyParts);
				if (strList.size() >= 2)
				{
					phoneme->fFrame = strList.at(0).toInt();
					phoneme->fText = strList.at(1);
				}
				word->fPhonemes << phoneme;
			} // for ph
			phrase->fWords << word;
		} // for w
		fPhrases << phrase;
	} // for p
}

void LipsyncVoice::Save(QTextStream &out)
{
	out << '\t' << fName << endl;
	out << '\t' << fText.split('\n').join('|') << endl;
	out << '\t' << fPhrases.size() << endl;
	for (int p = 0; p < fPhrases.size(); p++)
	{
		LipsyncPhrase *phrase = fPhrases[p];
		out << "\t\t" << phrase->fText << endl;
		out << "\t\t" << phrase->fStartFrame << endl;
		out << "\t\t" << phrase->fEndFrame << endl;
		out << "\t\t" << phrase->fWords.size() << endl;
		for (int w = 0; w < phrase->fWords.size(); w++)
		{
			LipsyncWord *word = phrase->fWords[w];
			out << "\t\t\t" << word->fText
				<< ' ' << word->fStartFrame
				<< ' ' << word->fEndFrame
				<< ' ' << word->fPhonemes.size()
				<< endl;
			for (int ph = 0; ph < word->fPhonemes.size(); ph++)
			{
				LipsyncPhoneme *phoneme = word->fPhonemes[ph];
				out << "\t\t\t\t" << phoneme->fFrame << ' ' << phoneme->fText << endl;
			} // for ph
		} // for w
	} // for p
}

void LipsyncVoice::Export(QString path)
{
	QFile	f(path);

	if (!f.open(QIODevice::WriteOnly | QIODevice::Text))
		return;

	QTextStream out(&f);

	out << "MohoSwitch1" << endl;

	int		startFrame = 0;
	int		endFrame = 1;
	QString	phoneme, nextPhoneme;

	if (fPhrases.size() > 0)
	{
		startFrame = fPhrases[0]->fStartFrame;
		endFrame = fPhrases.last()->fEndFrame;
	}

	if (startFrame > 1)
	{
		phoneme = "rest";
		out << 1 << ' ' << "rest" << endl;
	}

	for (int frame = startFrame; frame <= endFrame; frame++)
	{
		nextPhoneme = GetPhonemeAtFrame(frame);
		if (nextPhoneme != phoneme)
		{
			if (phoneme == "rest")
			{ // export an extra "rest" phoneme at the end of a pause between words or phrases
				out << frame << ' ' << phoneme << endl;
			}
			phoneme = nextPhoneme;
			out << frame - 1 << ' ' << phoneme << endl;
		}
	}
	out << endFrame + 2 << ' ' << "rest" << endl;
}

void LipsyncVoice::RunBreakdown(QString language, std::int32_t audioDuration)
{
	// make sure there is a space after all punctuation marks
	QString punctuation = ".,!?;";
	bool repeatLoop = true;
	while (repeatLoop)
	{
		repeatLoop = false;
		std::int32_t n = fText.length();
		for (std::int32_t i = 0; i < n - 1; i++)
		{
			if (punctuation.contains(fText[i]) && !fText[i + 1].isSpace())
			{
				fText.insert(i + 1,	' ');
				repeatLoop = true;
				break;
			}
		}
	}

	// break text into phrases
	while (!fPhrases.isEmpty())
		delete fPhrases.takeFirst();
	QStringList strList = fText.split('\n', QString::SkipEmptyParts);
	for (std::int32_t i = 0; i < strList.size(); i++)
	{
		if (strList.at(i).length() == 0)
			continue;
		LipsyncPhrase *phrase = new LipsyncPhrase;
		phrase->fText = strList.at(i);
		fPhrases << phrase;
	}

	// now break down the phrases
	for (std::int32_t i = 0; i < fPhrases.size(); i++)
		fPhrases[i]->RunBreakdown(language);

	// for first-guess frame alignment, count how many phonemes we have
	std::int32_t phonemeCount = 0;
	for (std::int32_t i = 0; i < fPhrases.size(); i++)
	{
		LipsyncPhrase *phrase = fPhrases[i];
		for (std::int32_t j = 0; j < phrase->fWords.size(); j++)
		{
			if (phrase->fWords[j]->fPhonemes.size() == 0) // deal with unknown words
				phonemeCount += 4;
			else
				phonemeCount += phrase->fWords[j]->fPhonemes.size();
		}
	}
	// now divide up the total time by phonemes
    std::uint32_t framesPerPhoneme = 1;
	if (audioDuration > 0 && phonemeCount > 0)
    {
        framesPerPhoneme = static_cast<std::uint32_t>((static_cast<float>(audioDuration) / static_cast<float>(phonemeCount)) + 0.5f);
		if (framesPerPhoneme < 1)
			framesPerPhoneme = 1;
	}

	// finally, assign frames based on phoneme durations
	std::int32_t curFrame = 0;
	for (std::int32_t i = 0; i < fPhrases.size(); i++)
	{
		LipsyncPhrase *phrase = fPhrases[i];
		for (std::int32_t j = 0; j < phrase->fWords.size(); j++)
		{
			LipsyncWord *word = phrase->fWords[j];
			for (std::int32_t k = 0; k < word->fPhonemes.size(); k++)
			{
				LipsyncPhoneme *phoneme = word->fPhonemes[k];
				phoneme->fFrame = curFrame;
				curFrame += framesPerPhoneme;
			} // for k
			if (word->fPhonemes.size() == 0)
			{ // deal with unknown words
				word->fStartFrame = curFrame;
				word->fEndFrame = curFrame + 3;
				curFrame += 4;
			}
			else
			{
				word->fStartFrame = word->fPhonemes.at(0)->fFrame;
                word->fEndFrame = word->fPhonemes.last()->fFrame + static_cast<std::int32_t>(framesPerPhoneme) - 1;
			}
		} // for j
		phrase->fStartFrame = phrase->fWords[0]->fStartFrame;
		phrase->fEndFrame = phrase->fWords.last()->fEndFrame;
	} // for i
}

void LipsyncVoice::RepositionPhrase(LipsyncPhrase *phrase, std::int32_t audioDuration)
{
	int id = fPhrases.indexOf(phrase);

	if ((id > 0) && (phrase->fStartFrame < fPhrases[id - 1]->fEndFrame + 1))
	{
		phrase->fStartFrame = fPhrases[id - 1]->fEndFrame + 1;
		if (phrase->fEndFrame < phrase->fStartFrame + 1)
			phrase->fEndFrame = phrase->fStartFrame + 1;
	}
	if ((id < fPhrases.size() - 1) && (phrase->fEndFrame > fPhrases[id + 1]->fStartFrame - 1))
	{
		phrase->fEndFrame = fPhrases[id + 1]->fStartFrame - 1;
		if (phrase->fStartFrame > phrase->fEndFrame - 1)
			phrase->fStartFrame = phrase->fEndFrame - 1;
	}
	if (phrase->fStartFrame < 0)
		phrase->fStartFrame = 0;
	if (phrase->fEndFrame > audioDuration)
		phrase->fEndFrame = audioDuration;
	if (phrase->fStartFrame > phrase->fEndFrame - 1)
		phrase->fStartFrame = phrase->fEndFrame - 1;

	// for first-guess frame alignment, count how many phonemes we have
	std::int32_t frameDuration = phrase->fEndFrame - phrase->fStartFrame + 1;
	std::int32_t phonemeCount = 0;
	for (std::int32_t i = 0; i < phrase->fWords.size(); i++)
	{
		LipsyncWord *word = phrase->fWords[i];
		if (word->fPhonemes.size() == 0) // deal with unknown words
			phonemeCount += 4;
		else
			phonemeCount += word->fPhonemes.size();
	}

	// now divide up the total time by phonemes
	float framesPerPhoneme = 1.0f;
	if (frameDuration > 0 && phonemeCount > 0)
	{
        framesPerPhoneme = static_cast<float>(frameDuration) / static_cast<float>(phonemeCount);
		if (framesPerPhoneme < 1.0f)
			framesPerPhoneme = 1.0f;
	}

	// finally, assign frames based on phoneme durations
	float curFrame = phrase->fStartFrame;
	for (std::int32_t i = 0; i < phrase->fWords.size(); i++)
	{
		LipsyncWord *word = phrase->fWords[i];
		for (std::int32_t j = 0; j < word->fPhonemes.size(); j++)
		{
            word->fPhonemes[j]->fFrame = pg_math::round(curFrame);
			curFrame += framesPerPhoneme;
		}
		if (word->fPhonemes.size() == 0) // deal with unknown words
		{
            word->fStartFrame = pg_math::round(curFrame);
			word->fEndFrame = word->fStartFrame + 3;
			curFrame += 4.0f;
		}
		else
		{
			word->fStartFrame = word->fPhonemes[0]->fFrame;
            word->fEndFrame = word->fPhonemes.last()->fFrame + pg_math::round(framesPerPhoneme) - 1;
		}
		phrase->RepositionWord(word);
	}
}

QString LipsyncVoice::GetPhonemeAtFrame(std::int32_t frame)
{
	for (std::int32_t i = 0; i < fPhrases.size(); i++)
	{
		LipsyncPhrase *phrase = fPhrases[i];
		if (frame >= phrase->fStartFrame && frame <= phrase->fEndFrame)
		{ // we found the phrase that contains this frame
			for (std::int32_t j = 0; j < phrase->fWords.size(); j++)
			{
				LipsyncWord *word = phrase->fWords[j];
				if (frame >= word->fStartFrame && frame <= word->fEndFrame)
				{ // we found the word that contains this frame
					if (word->fPhonemes.size() > 0)
					{
						for (std::int32_t k = word->fPhonemes.size() - 1; k >= 0; k--)
						{
							if (frame >= word->fPhonemes[k]->fFrame)
							{
								return word->fPhonemes[k]->fText;
							}
						}
					}
					else
					{ // volume-based breakdown
						return "";
					}
				}
			}
		}
	}

	return "rest";
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

QList<QString>				LipsyncDoc::Phonemes;
QHash<QString, QString>		LipsyncDoc::DictionaryToPhonemeMap;
QHash<QString, QStringList>	LipsyncDoc::PhonemeDictionary;

LipsyncDoc::LipsyncDoc()
{
	fDirty = false;
	fFps = 24;
	fAudioDuration = 0;
	fAudioPlayer = nullptr;
	fAudioExtractor = nullptr;
	fMaxAmplitude = 1.0f;
	fCurrentVoice = nullptr;
}

LipsyncDoc::~LipsyncDoc()
{
	if (fAudioPlayer)
	{
		fAudioPlayer->stop();
		delete fAudioPlayer;
		fAudioPlayer = nullptr;
	}
	if (fAudioExtractor)
	{
		delete fAudioExtractor;
		fAudioExtractor = nullptr;
	}
	while (!fVoices.isEmpty())
		delete fVoices.takeFirst();
}

void LipsyncDoc::LoadDictionaries()
{
	if (PhonemeDictionary.size() > 0)
		return;

	QFile	*f;

	f = new QFile(":/dictionaries/dictionaries/standard_dictionary");
	if (f->open(QIODevice::ReadOnly | QIODevice::Text))
	{
		LoadDictionary(f);
		f->close();
	}
	delete f;

	f = new QFile(":/dictionaries/dictionaries/extended_dictionary");
	if (f->open(QIODevice::ReadOnly | QIODevice::Text))
	{
		LoadDictionary(f);
		f->close();
	}
	delete f;

	f = new QFile(":/dictionaries/dictionaries/user_dictionary");
	if (f->open(QIODevice::ReadOnly | QIODevice::Text))
	{
		LoadDictionary(f);
		f->close();
	}
	delete f;

	f = new QFile(":/dictionaries/dictionaries/phoneme_mapping");
	if (f->open(QIODevice::ReadOnly | QIODevice::Text))
	{
		while (!f->atEnd())
		{
			QString line = f->readLine();
			line = line.trimmed();
			if (line.left(1) == "#" || line.length() == 0)
				continue; // skip comments

			QStringList strList = line.split(' ', QString::SkipEmptyParts);
			if (strList.size() > 1)
			{
				if (strList[0] == ".")
					Phonemes << strList.at(1);
				else
					DictionaryToPhonemeMap.insert(strList.at(0), strList.at(1));
			}
		}
		f->close();
	}
	delete f;
}

void LipsyncDoc::LoadDictionary(QFile *f)
{
	while (!f->atEnd())
	{
		QString line = f->readLine();
		line = line.trimmed();
		if (line.left(1) == "#" || line.length() == 0)
			continue; // skip comments

		QStringList strList = line.split(' ', QString::SkipEmptyParts);
		if (strList.size() > 1)
		{
			if (!PhonemeDictionary.contains(strList.at(0)))
				PhonemeDictionary.insert(strList.at(0), strList);
		}
	}
}

void LipsyncDoc::Open(const QString &path)
{
	QFile			*f;
	QString			str;
	QString			tempPath;
	std::int32_t			numVoices;

	f = new QFile(path);
	if (!f->open(QIODevice::ReadOnly | QIODevice::Text))
	{
		f->close();
		delete f;
		return;
	}
	QTextStream in(f);

	if (fAudioPlayer)
	{
		fAudioPlayer->stop();
		delete fAudioPlayer;
		fAudioPlayer = nullptr;
	}
	if (fAudioExtractor)
	{
		delete fAudioExtractor;
		fAudioExtractor = nullptr;
	}
	while (!fVoices.isEmpty())
		delete fVoices.takeFirst();
	fCurrentVoice = nullptr;

	fPath = path;
	str = in.readLine(); // discard the header
	tempPath = in.readLine().trimmed();
	QFileInfo audioFileInfo(tempPath);
	if (!audioFileInfo.isAbsolute())
	{
		QFileInfo fileInfo(path);
		QDir dir = fileInfo.absoluteDir();
		tempPath = dir.absoluteFilePath(tempPath);
	}
	fAudioPath = tempPath;

	fFps = in.readLine().toInt();
	fFps = PG_CLAMP(fFps, 1, 120);
	fAudioDuration = in.readLine().toInt();

	numVoices = in.readLine().toInt();
	for (int i = 0; i < numVoices; i++)
	{
		LipsyncVoice	*voice = new LipsyncVoice("");
		voice->Open(in);
		fVoices << voice;
	}

	f->close();
	delete f;
	OpenAudio(fAudioPath);
	if (fVoices.size() > 0)
		fCurrentVoice = fVoices[0];

	fDirty = false;
}

void LipsyncDoc::OpenAudio(const QString &path)
{
	fDirty = true;
	fMaxAmplitude = 1.0f;

	if (fAudioPlayer)
	{
		fAudioPlayer->stop();
		delete fAudioPlayer;
		fAudioPlayer = nullptr;
	}
	if (fAudioExtractor)
	{
		delete fAudioExtractor;
		fAudioExtractor = nullptr;
	}

	fAudioPath = path;
	fAudioPlayer = new QMediaPlayer;
	//connect(player, SIGNAL(positionChanged(qint64)), this, SLOT(positionChanged(qint64)));
	fAudioPlayer->setMedia(QUrl::fromLocalFile(fAudioPath));
	if (fAudioPlayer->error())
	{
		delete fAudioPlayer;
		fAudioPlayer = nullptr;
	}
	else
	{
		fFps = 24;
		fAudioExtractor = new AudioExtractor(path.toUtf8().data());
		if (fAudioExtractor->IsValid())
		{
            real f = fAudioExtractor->Duration() * fFps;
            fAudioDuration = pg_math::round(f);
			fMaxAmplitude = 0.001f;
			real time = 0.0f, sampleDur = 1.0f / 24.0f;
			while (time < fAudioExtractor->Duration())
			{
				real amp = fAudioExtractor->GetRMSAmplitude(time, sampleDur);
				if (amp > fMaxAmplitude)
					fMaxAmplitude = amp;
				time += sampleDur;
			}
		}
		else
		{
			delete fAudioExtractor;
			fAudioExtractor = nullptr;
		}
	}

	if (fVoices.size() == 0)
	{
		fCurrentVoice = new LipsyncVoice(tr("Voice 1"));
		fVoices << fCurrentVoice;
	}
}

void LipsyncDoc::Save()
{
	if (fPath.isEmpty())
		return;

	QFile			*f;

	f = new QFile(fPath);
	if (!f->open(QIODevice::WriteOnly | QIODevice::Text))
	{
		f->close();
		delete f;
		return;
	}

	if (fAudioExtractor && fAudioExtractor->IsValid())
	{
		real f = fAudioExtractor->Duration() * fFps;
        fAudioDuration = pg_math::round(f);
	}

	QTextStream out(f);

	out << "lipsync version 1" << endl;

	QFileInfo docInfo(fPath);
	QFileInfo audioInfo(fAudioPath);
	QString audioPath = fAudioPath;
	if (audioInfo.absoluteDir() == docInfo.absoluteDir())
	{
		audioPath = audioInfo.fileName();
	}

	out << audioPath << endl;
	out << fFps << endl;
	out << fAudioDuration << endl;

	out << fVoices.size() << endl;
	for (int i = 0; i < fVoices.size(); i++)
	{
		fVoices[i]->Save(out);
	}

	f->close();
	delete f;
	fDirty = false;
}

void LipsyncDoc::SetFps(std::int32_t fps)
{
	fFps = fps;
	fDirty = true;

	if (fAudioExtractor && fAudioExtractor->IsValid())
	{
		real f = fAudioExtractor->Duration() * fFps;
        fAudioDuration = pg_math::round(f);
	}
}

QMediaPlayer *LipsyncDoc::GetAudioPlayer()
{
	return fAudioPlayer;
}

AudioExtractor *LipsyncDoc::GetAudioExtractor()
{
	return fAudioExtractor;
}

QString LipsyncDoc::GetVolumePhonemeAtFrame(std::int32_t frame)
{
	if (!fAudioExtractor)
		return "rest";

	real amp = fAudioExtractor->GetRMSAmplitude((real)frame / (real)fFps, 1.0f / (real)fFps);
	amp /= fMaxAmplitude;
	amp *= 4.0f;
    std::int32_t volID = pg_math::round(amp);
	volID = PG_CLAMP(volID, 0, 4);

	// new method - use a fixed set of phonemes for this method:
	// rest, etc, E, L, AI
	// presumably, these will vary from more closed to more open
	// the benefit of this is that the same mouths can be used for amplitude-based lipsync as well as proper lipsync
	switch (volID)
	{
		case 0:
			return "rest";
		case 1:
			return "etc";
		case 2:
			return "E";
		case 3:
			return "L";
		case 4:
			return "AI";
	}
	return "rest";
}
