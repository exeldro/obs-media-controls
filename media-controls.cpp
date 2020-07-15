#include "media-controls.hpp"
#include <QToolTip>

#include <obs-module.h>
#include <QMainWindow>
#include <QVBoxLayout>
#include "ui_MediaControls.h"

#include "media-control.hpp"
#include "../../item-widget-helpers.hpp"

OBS_DECLARE_MODULE()
OBS_MODULE_AUTHOR("Exeldro");
OBS_MODULE_USE_DEFAULT_LOCALE("media-controls", "en-US")

bool obs_module_load()
{
	const auto main_window =
		static_cast<QMainWindow *>(obs_frontend_get_main_window());
	obs_frontend_push_ui_translation(obs_module_get_string);
	auto *tmp = new MediaControls(main_window);
	obs_frontend_add_dock(tmp);
	obs_frontend_pop_ui_translation();
	return true;
}

void obs_module_unload() {}

MODULE_EXPORT const char *obs_module_description(void)
{
	return obs_module_text("Description");
}

MODULE_EXPORT const char *obs_module_name(void)
{
	return obs_module_text("MediaControls");
}

void MediaControls::OBSSourceDestroy(void *data, calldata_t *calldata)
{
	UNUSED_PARAMETER(calldata);

	MediaControls *controls = static_cast<MediaControls *>(data);

}

void MediaControls::OBSSourceActivate(void *data, calldata_t *calldata)
{
	MediaControls *controls = static_cast<MediaControls *>(data);
	obs_source_t *source = static_cast<obs_source_t *>(calldata_ptr(calldata, "source"));
	uint32_t flags = obs_source_get_output_flags(source);
	if ((flags & OBS_SOURCE_CONTROLLABLE_MEDIA) == 0)
		return;

	QMetaObject::invokeMethod(controls, "ActivateSource",
				  Q_ARG(OBSSource, source));
}

void MediaControls::OBSSourceDeactivate(void *data, calldata_t *calldata)
{
	MediaControls *controls = static_cast<MediaControls *>(data);
	obs_source_t *source =
		static_cast<obs_source_t *>(calldata_ptr(calldata, "source"));
	uint32_t flags = obs_source_get_output_flags(source);
	if ((flags & OBS_SOURCE_CONTROLLABLE_MEDIA) == 0)
		return;

	QMetaObject::invokeMethod(controls, "DeactivateSource",
				  Q_ARG(OBSSource, source));
}

void MediaControls::OBSSourceRename(void *data, calldata_t *call_data)
{
	MediaControls *controls = static_cast<MediaControls *>(data);
	obs_source_t *source =
	static_cast<obs_source_t *>(calldata_ptr(call_data, "source"));
	const char *new_name = calldata_string(call_data, "new_name");
	const char *prev_name = calldata_string(call_data, "prev_name");

	QMetaObject::invokeMethod(controls, "RenameSource",
				  Q_ARG(OBSSource, source),
				  Q_ARG(QString, prev_name),
				  Q_ARG(QString, new_name));
	
}

MediaControls::MediaControls(QWidget *parent)
	: QDockWidget(parent),
	  ui(new Ui::MediaControls)
{
	ui->setupUi(this);


	auto sh = obs_get_signal_handler();
	signal_handler_connect(sh, "source_activate", OBSSourceActivate, this);
	signal_handler_connect(sh, "source_deactivate", OBSSourceDeactivate,
			       this);
	signal_handler_connect(sh, "source_rename", OBSSourceRename,
			       this);
	//signal_handler_connect(sh, "source_show");
	//signal_handler_connect(sh, "source_hide");
	//hide();
}

MediaControls::~MediaControls()
{
	deleteLater();
}

void MediaControls::ActivateSource(OBSSource source)
{
	MediaControl *c = new MediaControl(source, false);
	InsertQObjectByName(mediaControls, c);

	ui->verticalLayout->addWidget(c);
}

void MediaControls::DeactivateSource(OBSSource source)
{
	const char *source_name = obs_source_get_name(source);

	for (auto it = mediaControls.begin(); it != mediaControls.end(); ++it) {
		auto &mc = *it;

		if (mc->objectName() == source_name) {
			
			mediaControls.erase(it);
			ui->verticalLayout->removeWidget(mc);
			mc->deleteLater();
			break;
		}
	}
}

void MediaControls::RenameSource(OBSSource source, QString prev_name,
				 QString new_name)
{
	for (auto it = mediaControls.begin(); it != mediaControls.end(); ++it) {
		auto &mc = *it;

		if (mc->objectName() == prev_name) {
			mc->setObjectName(new_name);
			mediaControls.erase(it);
			InsertQObjectByName(mediaControls, mc);
			break;
		}
	}
}
