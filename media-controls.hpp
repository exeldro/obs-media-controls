#pragma once

#include <QTimer>
#include <obs.hpp>
#include <QDockWidget>
#include <memory>

#include <obs-frontend-api.h>

#include "media-control.hpp"
#include "ui_MediaControls.h"

class MediaControls : public QDockWidget {
	Q_OBJECT

private:
	static void OBSSignal(void *data, const char *signal,
			      calldata_t *calldata);
	static void AddActiveSource(obs_source_t *parent, obs_source_t *child,
				    void *param);
	static bool AddSource(void *param, obs_source_t *source);
	static void OBSFrontendEvent(enum obs_frontend_event event, void *ptr);
	void ControlContextMenu();
	void ToggleShowTimeDecimals();
	void ToggleShowTimeRemaining();
	void ToggleAllSources();
	void RefreshMediaControls();

	void addMediaControl(obs_source_t *source, int column);

	bool showTimeDecimals = false;
	bool showTimeRemaining = false;
	bool allSources = false;
	std::unique_ptr<Ui::MediaControls> ui;

private slots:
	void SignalMediaSource();

public:
	MediaControls(QWidget *parent = nullptr);
	~MediaControls();
};
