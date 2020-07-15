#pragma once

#include <QTimer>
#include <obs.hpp>
#include <QDockWidget>

#include "../UI/qt-wrappers.hpp"
#include <../UI/obs-frontend-api/obs-frontend-api.h>


#include "media-control.hpp"
#include "ui_MediaControls.h"

class MediaControls : public QDockWidget {
	Q_OBJECT

private:
	std::vector<MediaControl *> mediaControls;

	static void OBSSourceDestroy(void *data, calldata_t *calldata);
	static void OBSSourceActivate(void *data, calldata_t *calldata);
	static void OBSSourceDeactivate(void *data, calldata_t *calldata);
	static void OBSSourceRename(void *data, calldata_t *calldata);
	static void OBSSourceShow(void *data, calldata_t *calldata);
	static void OBSSourceHide(void *data, calldata_t *calldata);

	std::unique_ptr<Ui::MediaControls> ui;

private slots:
	void ActivateSource(OBSSource source);
	void DeactivateSource(OBSSource source);
	void RenameSource(OBSSource source, QString prev_name,
			  QString new_name);

public:
	MediaControls(QWidget *parent = nullptr);
	~MediaControls();
};
