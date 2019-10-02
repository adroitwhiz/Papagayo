#ifndef WAVEFORMVIEW_H
#define WAVEFORMVIEW_H

#include <QWidget>

#include "lipsyncdoc.h"

#include <cstdint>

class QScrollArea;

class WaveformView : public QWidget
{
	Q_OBJECT
public:
    explicit WaveformView(QWidget *parent = nullptr);
	~WaveformView();
	QSize sizeHint() const;

	void SetScrollArea(QScrollArea *scrollArea);
	void SetDocument(LipsyncDoc *doc);

signals:
	void frameChanged(int);

public slots:
	void onZoomIn();
	void onZoomOut();
	void onAutoZoom();
	void positionChanged(qint64 milliseconds);

protected:
	void mousePressEvent(QMouseEvent *event);
	void mouseDoubleClickEvent(QMouseEvent *event);
	void mouseMoveEvent(QMouseEvent *event);
	void mouseReleaseEvent(QMouseEvent *event);
	void paintEvent(QPaintEvent *event);

private:
	QScrollArea	*fScrollArea;
	LipsyncDoc	*fDoc;
	std::int32_t		fNumSamples;
	real		*fAmp;
	bool		fDragging, fDoubleClick;
	std::int32_t		fDraggingEnd;
	std::int32_t		fCurFrame;
	std::int32_t		fOldFrame;
	std::int32_t		fScrubFrame;
	std::int32_t		fAudioStopFrame;
	std::int32_t		fSampleWidth;
	std::int32_t		fSamplesPerFrame;
	std::int32_t		fSamplesPerSec;
	std::int32_t		fFrameWidth;
	std::int32_t		fPhraseBottom;
	std::int32_t		fWordBottom;
	std::int32_t		fPhonemeTop;

	LipsyncPhrase	*fSelectedPhrase, *fParentPhrase;
	LipsyncWord		*fSelectedWord, *fParentWord;
	LipsyncPhoneme	*fSelectedPhoneme;
};

#endif // WAVEFORMVIEW_H
