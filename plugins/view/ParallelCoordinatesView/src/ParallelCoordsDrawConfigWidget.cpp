/**
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

#include <QFileDialog>

#include <tulip/TlpQtTools.h>

#include "ParallelTools.h"
#include "ParallelCoordsDrawConfigWidget.h"
#include "ui_ParallelCoordsDrawConfigWidget.h"

using namespace std;

namespace tlp {

ParallelCoordsDrawConfigWidget::ParallelCoordsDrawConfigWidget(QWidget *parent)
    : QWidget(parent), oldValuesInitialized(false),
      _ui(new Ui::ParallelCoordsDrawConfigWidgetData) {
  _ui->setupUi(this);
  setBackgroundColor(Color(255, 255, 255));
  connect(_ui->browseButton, SIGNAL(clicked()), this, SLOT(pressButtonBrowse()));
  connect(_ui->userTexture, SIGNAL(toggled(bool)), this, SLOT(userTextureRbToggled(bool)));
  connect(_ui->minAxisPointSize, SIGNAL(valueChanged(int)), this,
          SLOT(minAxisPointSizeValueChanged(int)));
  connect(_ui->maxAxisPointSize, SIGNAL(valueChanged(int)), this,
          SLOT(maxAxisPointSizeValueChanged(int)));
}

ParallelCoordsDrawConfigWidget::~ParallelCoordsDrawConfigWidget() {
  delete _ui;
}

void ParallelCoordsDrawConfigWidget::pressButtonBrowse() {
  QString fileName(QFileDialog::getOpenFileName(this, "Open Texture File", "./",
                                                "Image Files (*.png *.jpg *.bmp)"));
  _ui->userTextureFile->setText(fileName);
}

unsigned int ParallelCoordsDrawConfigWidget::getAxisHeight() const {
  return _ui->axisHeight->value();
}

void ParallelCoordsDrawConfigWidget::setAxisHeight(const unsigned int aHeight) {
  _ui->axisHeight->setValue(aHeight);
}

bool ParallelCoordsDrawConfigWidget::drawPointOnAxis() const {
  return _ui->cBoxAxisPoints->isChecked();
}

string ParallelCoordsDrawConfigWidget::getLinesTextureFilename() const {
  if (_ui->cBoxLineTexture->isChecked()) {
    if (_ui->defaultTexture->isChecked()) {
      return DEFAULT_TEXTURE_FILE;
    } else {
      return QStringToTlpString(_ui->userTextureFile->text());
    }
  } else {
    return "";
  }
}

void ParallelCoordsDrawConfigWidget::setLinesTextureFilename(
    const std::string &linesTextureFileName) {
  if (!linesTextureFileName.empty()) {
    _ui->cBoxLineTexture->setChecked(true);

    if (linesTextureFileName == DEFAULT_TEXTURE_FILE) {
      _ui->defaultTexture->setChecked(true);
    } else {
      _ui->userTexture->setChecked(true);
      _ui->userTextureFile->setText(tlpStringToQString(linesTextureFileName));
    }
  } else {
    _ui->cBoxLineTexture->setChecked(false);
  }
}

Size ParallelCoordsDrawConfigWidget::getAxisPointMinSize() const {
  float pointSize = _ui->minAxisPointSize->text().toFloat();
  return Size(pointSize, pointSize, pointSize);
}

Size ParallelCoordsDrawConfigWidget::getAxisPointMaxSize() const {
  float pointSize = _ui->maxAxisPointSize->text().toFloat();
  return Size(pointSize, pointSize, pointSize);
}

void ParallelCoordsDrawConfigWidget::setAxisPointMinSize(const unsigned int axisPointMinSize) {
  _ui->minAxisPointSize->setValue(axisPointMinSize);
}

void ParallelCoordsDrawConfigWidget::setAxisPointMaxSize(const unsigned int axisPointMaxSize) {
  _ui->maxAxisPointSize->setValue(axisPointMaxSize);
}

bool ParallelCoordsDrawConfigWidget::displayNodeLabels() const {
  return _ui->displayLabelsCB->isChecked();
}

void ParallelCoordsDrawConfigWidget::setDisplayNodeLabels(const bool set) {
  return _ui->displayLabelsCB->setChecked(set);
}

void ParallelCoordsDrawConfigWidget::userTextureRbToggled(const bool checked) {
  _ui->userTextureFile->setEnabled(checked);
  _ui->browseButton->setEnabled(checked);
}

void ParallelCoordsDrawConfigWidget::minAxisPointSizeValueChanged(const int newValue) {
  if (_ui->maxAxisPointSize->value() < newValue) {
    _ui->maxAxisPointSize->setValue(newValue + 1);
  }
}

void ParallelCoordsDrawConfigWidget::maxAxisPointSizeValueChanged(const int newValue) {
  if (_ui->minAxisPointSize->value() > newValue) {
    _ui->minAxisPointSize->setValue(newValue - 1);
  }
}

void ParallelCoordsDrawConfigWidget::setLinesColorAlphaValue(unsigned int value) {
  if (value > 255) {
    _ui->viewColorAlphaRb->setChecked(true);
    _ui->userAlphaRb->setChecked(false);
  } else {
    _ui->viewColorAlphaRb->setChecked(false);
    _ui->userAlphaRb->setChecked(true);
    _ui->viewColorAlphaValue->setValue(value);
  }
}

unsigned int ParallelCoordsDrawConfigWidget::getLinesColorAlphaValue() const {
  if (_ui->viewColorAlphaRb->isChecked()) {
    return 300;
  } else {
    return _ui->viewColorAlphaValue->value();
  }
}

void ParallelCoordsDrawConfigWidget::setBackgroundColor(const Color &color) {
  _ui->bgColorButton->setTulipColor(color);
}

Color ParallelCoordsDrawConfigWidget::getBackgroundColor() const {
  return _ui->bgColorButton->tulipColor();
}

void ParallelCoordsDrawConfigWidget::setDrawPointOnAxis(const bool drawPointOnAxis) {
  _ui->cBoxAxisPoints->setChecked(drawPointOnAxis);
}

unsigned int ParallelCoordsDrawConfigWidget::getUnhighlightedEltsColorsAlphaValue() const {
  return _ui->nonHighlightedEltsAlphaValue->value();
}

void ParallelCoordsDrawConfigWidget::setUnhighlightedEltsColorsAlphaValue(
    const unsigned int alphaValue) {
  _ui->nonHighlightedEltsAlphaValue->setValue(alphaValue);
}

bool ParallelCoordsDrawConfigWidget::configurationChanged() {
  bool confChanged = false;

  if (oldValuesInitialized) {
    if (oldAxisHeight != getAxisHeight() || oldDrawPointOnAxis != drawPointOnAxis() ||
        oldAxisPointMinSize != getAxisPointMinSize() ||
        oldAxisPointMaxSize != getAxisPointMaxSize() ||
        oldDisplayNodesLabels != displayNodeLabels() ||
        oldLinesColorAlphaValue != getLinesColorAlphaValue() ||
        oldBackgroundColor != getBackgroundColor() ||
        oldUnhighlightedEltsColorsAlphaValue != getUnhighlightedEltsColorsAlphaValue() ||
        oldLinesTextureFilename != getLinesTextureFilename()) {
      confChanged = true;
    }
  } else {
    confChanged = true;
    oldValuesInitialized = true;
  }

  if (confChanged) {
    oldAxisHeight = getAxisHeight();
    oldDrawPointOnAxis = drawPointOnAxis();
    oldAxisPointMinSize = getAxisPointMinSize();
    oldAxisPointMaxSize = getAxisPointMaxSize();
    oldDisplayNodesLabels = displayNodeLabels();
    oldLinesColorAlphaValue = getLinesColorAlphaValue();
    oldBackgroundColor = getBackgroundColor();
    oldUnhighlightedEltsColorsAlphaValue = getUnhighlightedEltsColorsAlphaValue();
    oldLinesTextureFilename = getLinesTextureFilename();
  }

  return confChanged;
}
} // namespace tlp
