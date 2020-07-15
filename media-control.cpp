#include "media-control.hpp"
#include <QToolTip>
#include <QMainWindow>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QToolTip>
#include "../../item-widget-helpers.hpp"


MediaControl::MediaControl(OBSSource source_, bool showMs_)
	: source(std::move(source_)),
	  showMs(showMs_)
{

	timer = new QTimer(this);
	connect(timer, SIGNAL(timeout()), this, SLOT(SetSliderPosition()));

	QString sourceName = obs_source_get_name(source);
	setObjectName(sourceName);

	QVBoxLayout *mainLayout = new QVBoxLayout;
	mainLayout->setContentsMargins(4, 4, 4, 4);
	mainLayout->setSpacing(2);

	QHBoxLayout *sliderLayout = new QHBoxLayout;
	sliderLayout->setAlignment(Qt::AlignCenter);
	sliderLayout->setContentsMargins(0, 0, 0, 0);
	sliderLayout->setSpacing(2);
	timeLabel = new QLabel();
	sliderLayout->addWidget(timeLabel);
	slider = new MediaSlider();
	slider->setOrientation(Qt::Horizontal);
	slider->setTracking(false);
	slider->setMinimum(0);
	slider->setMaximum(4096);
	sliderLayout->addWidget(slider);
	durationLabel = new QLabel();
	sliderLayout->addWidget(durationLabel);

	QHBoxLayout *nameLayout = new QHBoxLayout;
	nameLayout->setAlignment(Qt::AlignLeft);
	nameLayout->setContentsMargins(0, 0, 0, 0);
	nameLayout->setSpacing(4);

	previousButton = new QPushButton();
	previousButton->setMinimumSize(35, 24);
	previousButton->setMaximumSize(35, 24);
	previousButton->setText(QString::fromUtf8("▐◄◄"));
	previousButton->setStyleSheet("padding: 0px 0px 0px 0px;");
	nameLayout->addWidget(previousButton);

	restartButton = new QPushButton();
	restartButton->setMinimumSize(35, 24);
	restartButton->setMaximumSize(35, 24);
	restartButton->setText(QString::fromUtf8("⭮"));//↩ ↻ ⭮
	restartButton->setStyleSheet("padding: 0px 0px 0px 0px;");
	nameLayout->addWidget(restartButton);

	playPauseButton = new QPushButton();
	playPauseButton->setMinimumSize(35, 24);
	playPauseButton->setMaximumSize(35, 24);
	playPauseButton->setText(QString::fromUtf8("►▌▌"));//⏯ ▶ 🢒 ᐅ
	playPauseButton->setStyleSheet("padding: 0px 0px 0px 0px;");
	nameLayout->addWidget(playPauseButton);

	stopButton = new QPushButton();
	stopButton->setMinimumSize(35, 24);
	stopButton->setMaximumSize(35, 24);
	stopButton->setText(QString::fromUtf8("⬛"));//◼⬛	▐▌
	stopButton->setStyleSheet("padding: 0px 0px 0px 0px;");
	nameLayout->addWidget(stopButton);

	nextButton = new QPushButton();
	nextButton->setMinimumSize(35, 24);
	nextButton->setMaximumSize(35, 24);
	nextButton->setText(QString::fromUtf8("►►▌"));
	nextButton->setStyleSheet("padding: 0px 0px 0px 0px;");
	nameLayout->addWidget(nextButton);

	nameLabel = new QLabel();
	nameLayout->addWidget(nameLabel);

	mainLayout->addItem(sliderLayout);
	mainLayout->addItem(nameLayout);

	setLayout(mainLayout);
	nameLabel->setText(sourceName);

	slider->setValue(0);
	float time = (float)obs_source_media_get_time(source) / 1000.0f;
	timeLabel->setText(FormatSeconds(time));
	float duration = (float)obs_source_media_get_duration(source) / 1000.0f;
	durationLabel->setText(FormatSeconds(duration));
	slider->setEnabled(false);

	connect(slider, SIGNAL(mediaSliderClicked()), this,
		SLOT(SliderClicked()));
	connect(slider, SIGNAL(mediaSliderHovered(int)), this,
		SLOT(SliderHovered(int)));
	connect(slider, SIGNAL(mediaSliderReleased(int)), this,
		SLOT(SliderReleased(int)));

	connect(restartButton, SIGNAL(clicked()), this,
		SLOT(on_restartButton_clicked()));
	connect(playPauseButton, SIGNAL(clicked()), this,
		SLOT(on_playPauseButton_clicked()));
	connect(stopButton, SIGNAL(clicked()), this,
		SLOT(on_stopButton_clicked()));
	connect(nextButton, SIGNAL(clicked()), this,
		SLOT(on_nextButton_clicked()));
	connect(previousButton, SIGNAL(clicked()), this,
		SLOT(on_previousButton_clicked()));

	signal_handler_t *sh = obs_source_get_signal_handler(source);
	signal_handler_connect(sh, "media_play", OBSMediaPlay, this);
	signal_handler_connect(sh, "media_pause", OBSMediaPause, this);
	signal_handler_connect(sh, "media_restart", OBSMediaPlay, this);
	signal_handler_connect(sh, "media_stopped", OBSMediaStopped, this);
	signal_handler_connect(sh, "media_started", OBSMediaStarted, this);
	signal_handler_connect(sh, "media_ended", OBSMediaStopped, this);

	RefreshControls();
}

MediaControl::~MediaControl()
{
	signal_handler_t *sh = obs_source_get_signal_handler(source);
	signal_handler_disconnect(sh, "media_play", OBSMediaPlay, this);
	signal_handler_disconnect(sh, "media_pause", OBSMediaPause, this);
	signal_handler_disconnect(sh, "media_restart", OBSMediaPlay, this);
	signal_handler_disconnect(sh, "media_stopped", OBSMediaStopped, this);
	signal_handler_disconnect(sh, "media_started", OBSMediaStarted, this);
	signal_handler_disconnect(sh, "media_ended", OBSMediaStopped, this);
}

void MediaControl::OBSMediaStopped(void *data, calldata_t *calldata)
{
	UNUSED_PARAMETER(calldata);

	MediaControl *media = static_cast<MediaControl *>(data);
	QMetaObject::invokeMethod(media, "SetRestartState");
}

void MediaControl::OBSMediaPlay(void *data, calldata_t *calldata)
{
	UNUSED_PARAMETER(calldata);

	MediaControl *media = static_cast<MediaControl *>(data);
	QMetaObject::invokeMethod(media, "SetPlayingState");
}

void MediaControl::OBSMediaPause(void *data, calldata_t *calldata)
{
	UNUSED_PARAMETER(calldata);

	MediaControl *media = static_cast<MediaControl *>(data);
	QMetaObject::invokeMethod(media, "SetPausedState");
}

void MediaControl::OBSMediaStarted(void *data, calldata_t *calldata)
{
	UNUSED_PARAMETER(calldata);

	MediaControl *media = static_cast<MediaControl *>(data);
	QMetaObject::invokeMethod(media, "SetPlayingState");
}

void MediaControl::SliderClicked()
{
	obs_media_state state = obs_source_media_get_state(source);

	switch (state) {
	case OBS_MEDIA_STATE_PAUSED:
		prevPaused = true;
		break;
	case OBS_MEDIA_STATE_PLAYING:
		prevPaused = false;
		obs_source_media_play_pause(source, true);
	default:
		break;
	}
}

void MediaControl::SliderReleased(int val)
{
	slider->setValue(val);

	float percent = (float)val / float(slider->maximum());
	float seekTo = percent * obs_source_media_get_duration(source);
	obs_source_media_set_time(source, seekTo);

	timeLabel->setText(FormatSeconds(seekTo / 1000.0f));

	if (!prevPaused)
		obs_source_media_play_pause(source, false);
}

void MediaControl::SliderHovered(int val)
{
	float percent = (float)val / float(slider->maximum());

	float seconds =
		percent * (obs_source_media_get_duration(source) / 1000.0f);

	QToolTip::showText(QCursor::pos(), FormatSeconds(seconds), this);
}

QString MediaControl::FormatSeconds(float totalSeconds)
{
	int totalWholeSeconds = (int)totalSeconds;
	int wholeSeconds = (int)totalWholeSeconds % 60;
	float seconds =
		(float)wholeSeconds + (totalSeconds - (float)totalWholeSeconds);
	int totalMinutes = totalWholeSeconds / 60;
	int minutes = totalMinutes % 60;
	int hours = totalMinutes / 60;

	QString text;
	if (hours) {
		text.sprintf("%02d:%02d:%02d", hours, minutes, wholeSeconds);
	} else if (showMs) {
		text.sprintf("%02d:%05.2f", minutes, seconds);
	} else {
		text.sprintf("%02d:%02d", minutes, wholeSeconds);
	}
	return text;
}

void MediaControl::StartTimer()
{
	if (!timer->isActive())
		timer->start(showMs ? 10 : 100);
}

void MediaControl::StopTimer()
{
	if (timer->isActive())
		timer->stop();
}

void MediaControl::SetPlayingState()
{
	slider->setEnabled(true);
	playPauseButton->setText(QString::fromUtf8("▐▐"));//⏸‖ ▐

	prevPaused = false;

	StartTimer();
}

void MediaControl::SetPausedState()
{
	playPauseButton->setText(QString::fromUtf8("►"));
	StopTimer();
}

void MediaControl::SetRestartState()
{
	playPauseButton->setText(QString::fromUtf8("►"));

	slider->setValue(0);
	timeLabel->setText(FormatSeconds(0));
	durationLabel->setText(FormatSeconds(0));
	slider->setEnabled(false);

	StopTimer();
}

void MediaControl::RefreshControls()
{
	if (!source) {
		SetRestartState();
		setEnabled(false);
		return;
	} else {
		setEnabled(true);
	}

	/*const char *id = obs_source_get_unversioned_id(source);

	if (id && *id && strcmp(id, "ffmpeg_source") == 0) {
		nextButton->hide();
		previousButton->hide();
	} else {
		nextButton->show();
		previousButton->show();
	}*/

	obs_media_state state = obs_source_media_get_state(source);

	switch (state) {
	case OBS_MEDIA_STATE_STOPPED:
	case OBS_MEDIA_STATE_ENDED:
		SetRestartState();
		break;
	case OBS_MEDIA_STATE_PLAYING:
		SetPlayingState();
		break;
	case OBS_MEDIA_STATE_PAUSED:
		SetPausedState();
		break;
	default:
		break;
	}
	SetSliderPosition();
}

void MediaControl::SetSliderPosition()
{
	float time = (float)obs_source_media_get_time(source);
	float duration = (float)obs_source_media_get_duration(source);

	float sliderPosition =
		duration == 0.0f ? 0.0f
				 : (time / duration) * (float)slider->maximum();

	slider->setValue((int)sliderPosition);

	timeLabel->setText(FormatSeconds(time / 1000.0f));
	durationLabel->setText(FormatSeconds(duration / 1000.0f));
}

OBSSource MediaControl::GetSource()
{
	return source;
}
void MediaControl::on_restartButton_clicked()
{
	obs_source_media_restart(source);
}

void MediaControl::on_playPauseButton_clicked()
{
	obs_media_state state = obs_source_media_get_state(source);

	switch (state) {
	case OBS_MEDIA_STATE_STOPPED:
	case OBS_MEDIA_STATE_ENDED:
		obs_source_media_restart(source);
		break;
	case OBS_MEDIA_STATE_PLAYING:
		obs_source_media_play_pause(source, true);
		break;
	case OBS_MEDIA_STATE_PAUSED:
		obs_source_media_play_pause(source, false);
		break;
	default:
		break;
	}
}

void MediaControl::on_stopButton_clicked()
{
	obs_source_media_stop(source);
}

void MediaControl::on_nextButton_clicked()
{
	obs_source_media_next(source);
}

void MediaControl::on_previousButton_clicked()
{
	obs_source_media_previous(source);
}
