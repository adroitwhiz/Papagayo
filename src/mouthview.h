#ifndef MOUTHVIEW_H
#define MOUTHVIEW_H

#include <QWidget>

#include "lipsyncdoc.h"

#include <cstdint>

class MouthView : public QWidget
{
	Q_OBJECT
public:
	explicit MouthView(QWidget *parent = 0);
	~MouthView();

	void SetDocument(LipsyncDoc *doc);
	void SetMouth(std::int32_t id);

signals:

public slots:
	void onMouthChanged(int id);
	void onFrameChanged(int frame);

protected:
	void paintEvent(QPaintEvent *event);

private:
	LipsyncDoc					*fDoc;
	std::int32_t						fMouthID;
	std::int32_t						fFrame;
	QHash<QString, QImage *>	fMouths[4];
};

#endif // MOUTHVIEW_H
