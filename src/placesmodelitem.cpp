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


#include "placesmodelitem.h"
#include "icontheme.h"
#include <gio/gio.h>
#include <QDebug>
#include <QProcess>

namespace Fm {

PlacesModelItem::PlacesModelItem():
  QStandardItem(),
  path_(NULL),
  icon_(NULL),
  fileInfo_(NULL) {
}

PlacesModelItem::PlacesModelItem(const char* iconName, QString title, FmPath* path):
  QStandardItem(title),
  path_(path ? fm_path_ref(path) : NULL),
  icon_(fm_icon_from_name(iconName)),
  fileInfo_(NULL) {
  if(icon_)
    QStandardItem::setIcon(IconTheme::icon(icon_));
  setEditable(false);
}

PlacesModelItem::PlacesModelItem(FmIcon* icon, QString title, FmPath* path):
  QStandardItem(title),
  path_(path ? fm_path_ref(path) : NULL),
  icon_(icon ? fm_icon_ref(icon) : NULL),
  fileInfo_(NULL) {
  if(icon_)
    QStandardItem::setIcon(IconTheme::icon(icon));
  setEditable(false);
}

PlacesModelItem::PlacesModelItem(QIcon icon, QString title, FmPath* path):
  QStandardItem(icon, title),
  icon_(NULL),
  path_(path ? fm_path_ref(path) : NULL),
  fileInfo_(NULL) {
  setEditable(false);
}

PlacesModelItem::~PlacesModelItem() {
  if(path_)
    fm_path_unref(path_);
  if(fileInfo_)
    g_object_unref(fileInfo_);
  if(icon_)
    fm_icon_unref(icon_);
}

void PlacesModelItem::setPath(FmPath* path) {
  if(path_)
    fm_path_unref(path_);
  path_ = path ? fm_path_ref(path) : NULL;
}

void PlacesModelItem::setIcon(FmIcon* icon) {
  if(icon_)
    fm_icon_unref(icon_);
  if(icon) {
    icon_ = fm_icon_ref(icon);
    QStandardItem::setIcon(IconTheme::icon(icon_));
  }
  else {
    icon_ = NULL;
    QStandardItem::setIcon(QIcon());
  }
}

void PlacesModelItem::setIcon(GIcon* gicon) {
  FmIcon* icon = gicon ? fm_icon_from_gicon(gicon) : NULL;
  setIcon(icon);
  fm_icon_unref(icon);
}

void PlacesModelItem::updateIcon() {
  if(icon_)
    QStandardItem::setIcon(IconTheme::icon(icon_));
}

QVariant PlacesModelItem::data(int role) const {
  // we use a QPixmap from FmIcon cache rather than QIcon object for decoration role.
  return QStandardItem::data(role);
}

void PlacesModelItem::setFileInfo(FmFileInfo* fileInfo) {
  // FIXME: how can we correctly update icon?
  if(fileInfo_)
    fm_file_info_unref(fileInfo_);

  if(fileInfo) {
    fileInfo_ = fm_file_info_ref(fileInfo);
  }
  else
    fileInfo_ = NULL;
}

PlacesModelBookmarkItem::PlacesModelBookmarkItem(FmBookmarkItem* bm_item):
  PlacesModelItem(QIcon::fromTheme("folder"), QString::fromUtf8(bm_item->name), bm_item->path),
  bookmarkItem_(fm_bookmark_item_ref(bm_item)) {
  setEditable(true);
}

PlacesModelVolumeItem::PlacesModelVolumeItem(GVolume* volume):
  PlacesModelItem(),
  volume_(reinterpret_cast<GVolume*>(g_object_ref(volume))) {
  update();
  setEditable(false);
}

void PlacesModelVolumeItem::update() {
  // set title
  char* volumeName = g_volume_get_name(volume_);
  qDebug() << "probono: PlacesModelVolumeItem::update";
  setText(QString::fromUtf8(volumeName));
  qDebug() << "probono: volumeName", QString::fromUtf8(volumeName);
  g_free(volumeName);

  // set icon
  GIcon* gicon = g_volume_get_icon(volume_);
  setIcon(gicon);
  g_object_unref(gicon);

  // set dir path
  GMount* mount = g_volume_get_mount(volume_);
  qDebug() << "probono: // set dir path";
  if(mount) {
    GFile* mount_root = g_mount_get_root(mount);
    FmPath* mount_path = fm_path_new_for_gfile(mount_root);
    setPath(mount_path);
    qDebug() << "probono: mount_path:" << mount_path;
    fm_path_unref(mount_path);
    g_object_unref(mount_root);
    g_object_unref(mount);
  }
  else {
    setPath(NULL);
  }
}


bool PlacesModelVolumeItem::isMounted() {
  GMount* mount = g_volume_get_mount(volume_);
  if(mount)
    g_object_unref(mount);
  return mount != NULL ? true : false;
}


PlacesModelMountItem::PlacesModelMountItem(GMount* mount):
  PlacesModelItem(),
  mount_(reinterpret_cast<GMount*>(mount)) {
  update();
  setEditable(false);
}

void PlacesModelMountItem::update() {
  // set title
  QString mount_name = QString::fromUtf8(g_mount_get_name(mount_));
  setText(mount_name);
  qDebug() << "probono: Get the 'Volume label' for the volume";
  QString displayName;
#ifdef __FreeBSD__
  qDebug() << "probono: Using 'fstyp -l /dev/" + mount_name + "' on FreeBSD";
  // NOTE: foldermodelitem.cpp has similar code for what gets shown in computer:///
  // NOTE: Alternatively, we could just use mountpoints that have the volume label as their name
  QProcess p;
  QString program = "fstyp";
  QStringList arguments;
  arguments << "-l" << "/dev/" + mount_name;
  p.start(program, arguments);
  p.waitForFinished();
  QString result(p.readAllStandardOutput());
  result.replace("\n", "");
  result = result.trimmed();
  qDebug() << "probono: result:" << result;
  if (result.split(" ").length() == 1) {
      if (result.split(" ")[0] != "" && result.split(" ")[0] != nullptr) {
          // We got a filesystem but no volume label back, so use the filesystem
          displayName = result.split(" ")[0];
          setText(displayName);
      }
  } else if (result.split(" ").length() == 2) {
      if (result.split(" ")[1] != "" && result.split(" ")[1] != nullptr) {
          // We got a filesystem and a volume label back, so use the volume label
          displayName = result.split(" ")[1];
          setText(displayName);
      }
  }
#else
  qDebug() << "probono: TODO: To be implemented for this OS";
#endif

  // set path
  GFile* mount_root = g_mount_get_root(mount_);
  FmPath* mount_path = fm_path_new_for_gfile(mount_root);
  setPath(mount_path);
  fm_path_unref(mount_path);
  g_object_unref(mount_root);

  // set icon
  GIcon* gicon = g_mount_get_icon(mount_);
  setIcon(gicon);
  g_object_unref(gicon);
}

}
