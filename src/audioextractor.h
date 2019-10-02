#ifndef AUDIOEXTRACTOR_H
#define AUDIOEXTRACTOR_H

#include "pg_config.h"
#include "sndfile.h"

#include <cstdint>

//
// This class is the general audio extractor that wraps the library specific extractors.
// Its main purpose is to extract audio tracks from audio and video media and place them
// in either a .aiff or .wav temporary file depending on the platform and the library. 
// The audio library libsndfile will than read into memory the temporary file as sound samples.
//
// NOTE: If a path is already a .aiff or .wav file it is just read in directly by libsndfile.
//
class AudioExtractor {
public:
	AudioExtractor(const char *path, bool reverse = false);
	~AudioExtractor();

	bool IsValid() const;
	real Duration() const;

	real GetAmplitude(real startTime, real duration) const;
	real GetRMSAmplitude(real startTime, real duration) const;
	real GetMaxAmplitude(real startTime, real duration) const;

	std::uint32_t NumSamples() const;
	std::int32_t SampleRate() const;
	real *Buffer() const;
	std::uint32_t TimeToSample(real time, bool clamped) const;

	SF_INFO		fSndInfo;
	std::uint32_t		fNumSamples;
	sf_count_t	fNumFrames;
	float		*fSamples;
	
private:
    const char * fSourcePath;
	
	// This function uses the cross platform libsndfile to read .aiff and .wav files
	bool ReadSoundFile(const char *soundFilePath);
};

#endif
