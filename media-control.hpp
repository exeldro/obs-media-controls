#pragma once

#include <QTimer>
#include <obs.hpp>
#include <qicon.h>

#include "media-slider.hpp"
#include "../UI/qt-wrappers.hpp"

class MediaControl : public QWidget {
	Q_OBJECT

private:
	OBSSource source;
	QLabel *nameLabel;
	MediaSlider *slider;
	QPushButton *restartButton;
	QPushButton *playPauseButton;
	QPushButton *previousButton;
	QPushButton *stopButton;
	QPushButton *nextButton;
	QLabel *timeLabel;
	QLabel *durationLabel;

	QIcon playIcon;
	QIcon pauseIcon;

	QTimer *timer;
	bool prevPaused = false;
	bool showMs = false;

	QString FormatSeconds(float totalSeconds);
	void StartTimer();
	void StopTimer();
	void RefreshControls();

	static void OBSMediaStopped(void *data, calldata_t *calldata);
	static void OBSMediaPlay(void *data, calldata_t *calldata);
	static void OBSMediaPause(void *data, calldata_t *calldata);
	static void OBSMediaStarted(void *data, calldata_t *calldata);

private slots:
	void on_restartButton_clicked();
	void on_playPauseButton_clicked();
	void on_stopButton_clicked();
	void on_nextButton_clicked();
	void on_previousButton_clicked();
	void SliderClicked();
	void SliderReleased(int val);
	void SliderHovered(int val);
	void SetSliderPosition();
	void SetPlayingState();
	void SetPausedState();
	void SetRestartState();

public:
	explicit MediaControl(OBSSource source, bool showMs);
	~MediaControl();
	OBSSource GetSource();
};
