/*

    Copyright (C) 2013  Hong Jen Yee (PCMan) <pcman.tw@gmail.com>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/


#ifndef PCMANFM_APPLICATION_H
#define PCMANFM_APPLICATION_H

#include <QApplication>
#include "settings.h"
#include "libfmqt.h"
#include "editbookmarksdialog.h"
#include <QVector>
#include <QPointer>
#include <QTranslator>
#include <gio/gio.h>


class QScreen;

class QFileSystemWatcher;

namespace Filer {

class DesktopWindow;
class PreferencesDialog;
class DesktopPreferencesDialog;

class Application : public QApplication {
  Q_OBJECT
  Q_PROPERTY(bool desktopManagerEnabled READ desktopManagerEnabled)

public:
  Application(int& argc, char** argv);
  virtual ~Application();

  void init();
  int exec();

  Settings& settings() {
    return settings_;
  }

  Fm::LibFmQt& libFm() {
    return libFm_;
  }

  // public interface exported via dbus
  void launchFiles(QString cwd, QStringList paths, bool inNewWindow);
  void setWallpaper(QString path, QString modeString);
  void preferences(QString page);
  void desktopPrefrences();
  void editBookmarks();
  void desktopManager(bool enabled);
  void findFiles(QStringList paths = QStringList());
  void ShowFolders(QStringList uriList, QString startupId);
  void ShowItems(QStringList uriList, QString startupId);
  void ShowItemProperties(QStringList uriList, QString startupId);

  bool desktopManagerEnabled() {
    return enableDesktopManager_;
  }

  void updateFromSettings();
  void updateDesktopsFromSettings();

  void openFolderInTerminal(FmPath* path);
  void openFolders(FmFileInfoList* files);

  QString profileName() {
    return profileName_;
  }

Q_SIGNALS:
  void openFolderAndSelectItems(QString folder, QStringList items);

protected Q_SLOTS:
  void onAboutToQuit();
  void onSigtermNotified();

  void onLastWindowClosed();
  void onSaveStateRequest(QSessionManager & manager);
  void onScreenResized(int num);
  void onScreenCountChanged(int newCount);
  void initVolumeManager();

  void onVirtualGeometryChanged(const QRect& rect);
  void onScreenDestroyed(QObject* screenObj);
  void onScreenAdded(QScreen* newScreen);
  void reloadDesktopsAsNeeded();

  void onFindFileAccepted();

protected:
  virtual bool eventFilter(QObject* watched, QEvent* event);
  bool parseCommandLineArgs();
  DesktopWindow* createDesktopWindow(int screenNum);
  bool autoMountVolume(GVolume* volume, bool interactive = true);

  static void onVolumeAdded(GVolumeMonitor* monitor, GVolume* volume, Application* pThis);

private Q_SLOTS:
  void onUserDirsChanged();

private:
  void initWatch();
  void installSigtermHandler();

  bool isPrimaryInstance;
  Fm::LibFmQt libFm_;
  Settings settings_;
  QString profileName_;
  bool daemonMode_;
  bool enableDesktopManager_;
  QVector<DesktopWindow*> desktopWindows_;
  QPointer<PreferencesDialog> preferencesDialog_;
  QPointer<DesktopPreferencesDialog> desktopPreferencesDialog_;
  QPointer<Fm::EditBookmarksDialog> editBookmarksialog_;
  QTranslator translator;
  QTranslator qtTranslator;
  GVolumeMonitor* volumeMonitor_;

  QFileSystemWatcher *userDirsWatcher_;
  QString userDirsFile_;
  QString userDesktopFolder_;
  bool lxqtRunning_;

  int argc_;
  char** argv_;
};

}

#endif // PCMANFM_APPLICATION_H
