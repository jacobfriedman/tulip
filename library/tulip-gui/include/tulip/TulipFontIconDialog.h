/*
 *
 * This file is part of Tulip (http://tulip.labri.fr)
 *
 * Authors: David Auber and the Tulip development Team
 * from LaBRI, University of Bordeaux
 *
 * Tulip is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation, either version 3
 * of the License, or (at your option) any later version.
 *
 * Tulip is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 */
///@cond DOXYGEN_HIDDEN
#ifndef TULIPFONTICONDIALOG_H
#define TULIPFONTICONDIALOG_H

#include <tulip/tulipconf.h>

#include <QDialog>
#include <QString>
#include <QIcon>

namespace Ui {
class TulipFontIconDialog;
}

namespace tlp {

class TLP_QT_SCOPE TulipFontIconDialog : public QDialog {

  Q_OBJECT

  Ui::TulipFontIconDialog *_ui;
  QString _selectedIconName;

public:
  TulipFontIconDialog(QWidget *parent = nullptr);

  QString getSelectedIconName() const;

  void setSelectedIconName(const QString &iconName);

  void accept() override;

  void showEvent(QShowEvent *) override;

protected slots:

  void updateIconList();

  void openUrlInBrowser(const QString &url);

protected:
  bool eventFilter(QObject *, QEvent *e) override;
};
} // namespace tlp

#endif
///@endcond
