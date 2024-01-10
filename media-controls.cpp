#include "media-controls.hpp"
#include <QToolTip>

#include <obs-module.h>
#include <QMainWindow>
#include <QMenuBar>
#include <QVBoxLayout>
#include "ui_MediaControls.h"

#include "media-control.hpp"
#include "version.h"
#include "util/platform.h"

OBS_DECLARE_MODULE()
OBS_MODULE_AUTHOR("Exeldro");
OBS_MODULE_USE_DEFAULT_LOCALE("media-controls", "en-US")

#define QT_UTF8(str) QString::fromUtf8(str)
#define QT_TO_UTF8(str) str.toUtf8().constData()

bool obs_module_load()
{
	blog(LOG_INFO, "[Media Controls] loaded version %s", PROJECT_VERSION);
	const auto main_window =
		static_cast<QMainWindow *>(obs_frontend_get_main_window());
	obs_frontend_push_ui_translation(obs_module_get_string);
	auto *tmp = new MediaControls(main_window);

	const QString title = QString::fromUtf8(obs_module_text("MediaControls"));
	const auto name = "MediaControls";
#if LIBOBS_API_VER >= MAKE_SEMANTIC_VERSION(30, 0, 0)
	obs_frontend_add_dock_by_id(name, title.toUtf8().constData(),
				    tmp);
#else
	auto dock = new QDockWidget(main_window);
	dock->setObjectName(QString::fromUtf8(name));
	dock->setWindowTitle(title);
	dock->setWidget(tmp);
	dock->setFeatures(QDockWidget::DockWidgetMovable |
			  QDockWidget::DockWidgetFloatable);
	dock->setFloating(true);
	dock->hide();
	obs_frontend_add_dock(dock);
#endif
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

void MediaControls::OBSSignal(void *data, const char *signal,
			      calldata_t *call_data)
{
	UNUSED_PARAMETER(signal);
	obs_source_t *source =
		static_cast<obs_source_t *>(calldata_ptr(call_data, "source"));
	if (!source)
		return;
	uint32_t flags = obs_source_get_output_flags(source);
	if ((flags & OBS_SOURCE_CONTROLLABLE_MEDIA) == 0)
		return;

	MediaControls *controls = static_cast<MediaControls *>(data);
	QMetaObject::invokeMethod(controls, "SignalMediaSource",
				  Qt::QueuedConnection);
}

void MediaControls::OBSFrontendEvent(enum obs_frontend_event event, void *ptr)
{
	MediaControls *controls = reinterpret_cast<MediaControls *>(ptr);

	switch (event) {
	case OBS_FRONTEND_EVENT_SCENE_CHANGED:
	case OBS_FRONTEND_EVENT_PREVIEW_SCENE_CHANGED:
		controls->RefreshMediaControls();
		break;
	default:
		break;
	}
}

void MediaControls::AddActiveSource(obs_source_t *parent, obs_source_t *child,
				    void *param)
{
	UNUSED_PARAMETER(parent);
	if (obs_source_removed(child))
		return;
	auto *controls = static_cast<MediaControls *>(param);
	const uint32_t flags = obs_source_get_output_flags(child);
	if ((flags & OBS_SOURCE_CONTROLLABLE_MEDIA) == 0) {
		obs_source_enum_active_sources(child, AddActiveSource, param);
		return;
	}

	obs_data_t *priv_settings = obs_source_get_private_settings(child);
	bool hidden = obs_data_get_bool(priv_settings, "media_hidden");
	obs_data_release(priv_settings);
	if (hidden)
		return;

	QString source_name = QT_UTF8(obs_source_get_name(child));
	const auto count = controls->ui->verticalLayout->count();
	for (int i = count - 1; i >= 0; i--) {
		QLayoutItem *item = controls->ui->verticalLayout->itemAt(i);
		if (!item)
			continue;
		auto *w = dynamic_cast<MediaControl *>(item->widget());
		if (w->objectName() == source_name ||
		    w->GetSource() == OBSGetWeakRef(child)) {
			return;
		}
	}

	for (int i = count - 1; i >= 0; i--) {
		QLayoutItem *item = controls->ui->verticalLayout->itemAt(i);
		if (item) {
			QWidget *w = item->widget();
			if (source_name.localeAwareCompare(w->objectName()) >=
			    0) {
				controls->addMediaControl(child, i + 1);
				return;
			}
		}
	}
	controls->addMediaControl(child, 0);
}

void MediaControls::addMediaControl(obs_source_t *source, int column)
{
	MediaControl *c = new MediaControl(OBSGetWeakRef(source),
					   showTimeDecimals, showTimeRemaining);
	ui->verticalLayout->insertWidget(column, c);
}

bool MediaControls::AddSource(void *param, obs_source_t *source)
{
	if (obs_source_removed(source))
		return true;
	MediaControls *controls = static_cast<MediaControls *>(param);
	uint32_t flags = obs_source_get_output_flags(source);
	if ((flags & OBS_SOURCE_CONTROLLABLE_MEDIA) == 0)
		return true;

	obs_data_t *priv_settings = obs_source_get_private_settings(source);
	bool hidden = obs_data_get_bool(priv_settings, "media_hidden");
	obs_data_release(priv_settings);
	if (hidden)
		return true;

	QString source_name = QT_UTF8(obs_source_get_name(source));
	const auto count = controls->ui->verticalLayout->count();
	for (int i = count - 1; i >= 0; i--) {
		QLayoutItem *item = controls->ui->verticalLayout->itemAt(i);
		if (!item)
			continue;
		auto *w = dynamic_cast<MediaControl *>(item->widget());
		if (w->objectName() == source_name ||
		    w->GetSource() == OBSGetWeakRef(source)) {
			return true;
		}
	}
	for (int i = count - 1; i >= 0; i--) {
		QLayoutItem *item = controls->ui->verticalLayout->itemAt(i);
		if (item) {
			QWidget *w = item->widget();
			if (source_name.localeAwareCompare(w->objectName()) >=
			    0) {
				controls->addMediaControl(source, i + 1);
				return true;
			}
		}
	}
	controls->addMediaControl(source, 0);
	return true;
}

MediaControls::MediaControls(QWidget *parent)
	: QWidget(parent),
	  ui(new Ui::MediaControls)
{
	ui->setupUi(this);

	char *file = obs_module_config_path("config.json");
	obs_data_t *data = nullptr;
	if (file) {
		data = obs_data_create_from_json_file(file);
		bfree(file);
	}
	if (data) {
		showTimeDecimals = obs_data_get_bool(data, "showTimeDecimals");
		showTimeRemaining =
			obs_data_get_bool(data, "showTimeRemaining");
		allSources = obs_data_get_bool(data, "showAllSources");
		obs_data_release(data);
	}

	connect(this, &QWidget::customContextMenuRequested,
		this, &MediaControls::ControlContextMenu);

	signal_handler_connect_global(obs_get_signal_handler(), OBSSignal,
				      this);
	obs_frontend_add_event_callback(OBSFrontendEvent, this);

	hide();
}

MediaControls::~MediaControls()
{
	signal_handler_disconnect_global(obs_get_signal_handler(), OBSSignal,
					 this);
	char *file = obs_module_config_path("config.json");
	if (!file) {
		deleteLater();
		return;
	}
	obs_data_t *data = obs_data_create_from_json_file(file);
	if (!data)
		data = obs_data_create();
	obs_data_set_bool(data, "showTimeDecimals", showTimeDecimals);
	obs_data_set_bool(data, "showTimeRemaining", showTimeRemaining);
	obs_data_set_bool(data, "showAllSources", allSources);
	if (!obs_data_save_json(data, file)) {
		char *path = obs_module_config_path("");
		if (path) {
			os_mkdirs(path);
			bfree(path);
		}
		obs_data_save_json(data, file);
	}
	obs_data_release(data);
	bfree(file);
	deleteLater();
}

void MediaControls::SignalMediaSource()
{
	RefreshMediaControls();
}

void MediaControls::ControlContextMenu()
{

	QAction showTimeDecimalsAction(obs_module_text("ShowTimeDecimals"),
				       this);
	showTimeDecimalsAction.setCheckable(true);
	showTimeDecimalsAction.setChecked(showTimeDecimals);

	QAction showTimeRemainingAction(obs_module_text("ShowTimeRemaining"),
					this);
	showTimeRemainingAction.setCheckable(true);
	showTimeRemainingAction.setChecked(showTimeRemaining);

	QAction allSourcesAction(obs_module_text("AllSources"), this);
	allSourcesAction.setCheckable(true);
	allSourcesAction.setChecked(allSources);

	connect(&showTimeDecimalsAction, &QAction::toggled, this,
		&MediaControls::ToggleShowTimeDecimals, Qt::DirectConnection);

	connect(&showTimeRemainingAction, &QAction::toggled, this,
		&MediaControls::ToggleShowTimeRemaining, Qt::DirectConnection);

	connect(&allSourcesAction, &QAction::toggled, this,
		&MediaControls::ToggleAllSources, Qt::DirectConnection);

	QMenu popup;
	popup.addAction(&showTimeDecimalsAction);
	popup.addAction(&showTimeRemainingAction);
	popup.addSeparator();
	popup.addAction(&allSourcesAction);
	popup.addSeparator();
	auto t = popup.addMenu(obs_module_text("Hide"));
	connect(t, &QMenu::aboutToShow, [this, t] { HideMenu(t); });

	popup.exec(QCursor::pos());
}

void MediaControls::ToggleShowTimeDecimals()
{
	showTimeDecimals = !showTimeDecimals;
	RefreshMediaControls();
}

void MediaControls::ToggleShowTimeRemaining()
{
	showTimeRemaining = !showTimeRemaining;
	RefreshMediaControls();
}

void MediaControls::ToggleAllSources()
{
	allSources = !allSources;
	RefreshMediaControls();
}

void MediaControls::RefreshMediaControls()
{
	const auto count = ui->verticalLayout->count();
	for (int i = count - 1; i >= 0; i--) {
		QLayoutItem *item = ui->verticalLayout->itemAt(i);
		if (!item)
			continue;
		QWidget *w = item->widget();
		obs_source_t *source =
			obs_get_source_by_name(QT_TO_UTF8(w->objectName()));
		if (!source) {
			ui->verticalLayout->removeItem(item);
			MediaControl::OBSRemove(w, nullptr);
			w->deleteLater();
		} else {
			if (obs_source_removed(source) ||
			    (!allSources && !obs_source_active(source))) {
				ui->verticalLayout->removeItem(item);
				MediaControl::OBSRemove(w, nullptr);
				w->deleteLater();
			} else {
				obs_data_t *priv_settings =
					obs_source_get_private_settings(source);
				bool hidden = obs_data_get_bool(priv_settings,
								"media_hidden");
				obs_data_release(priv_settings);
				if (hidden) {
					ui->verticalLayout->removeItem(item);
					MediaControl::OBSRemove(w, nullptr);
					w->deleteLater();
				} else {
					MediaControl *mc = (MediaControl *)w;
					mc->showTimeDecimals = showTimeDecimals;
					mc->showTimeRemaining =
						showTimeRemaining;
				}
			}
			obs_source_release(source);
		}
	}

	if (allSources) {
		obs_enum_sources(AddSource, this);
	} else {
		obs_source_t *scene = obs_frontend_get_current_scene();
		if (scene) {
			obs_source_enum_active_sources(scene, AddActiveSource,
						       this);
			obs_source_release(scene);
		}
		scene = obs_frontend_get_current_preview_scene();
		if (scene) {
			obs_source_enum_active_sources(scene, AddActiveSource,
						       this);
			obs_source_release(scene);
		}
	}
}

void MediaControls::HideMenu(QMenu *menu)
{
	menu->clear();
	std::pair<MediaControls *, QMenu *> data(this, menu);
	obs_enum_sources(
		[](void *data, obs_source_t *source) {
			auto t = (std::pair<MediaControls *, QMenu *> *)data;
			QMenu *menu = t->second;
			if (obs_source_removed(source))
				return true;
			uint32_t flags = obs_source_get_output_flags(source);
			if ((flags & OBS_SOURCE_CONTROLLABLE_MEDIA) == 0)
				return true;

			obs_data_t *priv_settings =
				obs_source_get_private_settings(source);
			bool hidden = obs_data_get_bool(priv_settings,
							"media_hidden");
			obs_data_release(priv_settings);

			QString source_name =
				QT_UTF8(obs_source_get_name(source));

			QList<QAction *> actions = menu->actions();
			QAction *before = nullptr;
			for (QAction *menuAction : actions) {
				if (menuAction->text().compare(source_name) >=
				    0)
					before = menuAction;
			}
			auto action = new QAction(source_name, menu);
			menu->insertAction(before, action);
			action->setCheckable(true);
			action->setChecked(hidden);
			auto mc = t->first;
			connect(action, &QAction::triggered, [mc, source] {
				obs_data_t *priv_settings =
					obs_source_get_private_settings(source);
				bool hidden = obs_data_get_bool(priv_settings,
								"media_hidden");
				obs_data_set_bool(priv_settings, "media_hidden",
						  !hidden);
				obs_data_release(priv_settings);
				mc->RefreshMediaControls();
			});
			return true;
		},
		&data);
}
