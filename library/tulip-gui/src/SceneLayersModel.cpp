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

#include <tulip/SceneLayersModel.h>

#include <QFont>
#include <QVector>
#include <QWidget>

#include <tulip/GlScene.h>
#include <tulip/GlGraphComposite.h>
#include <tulip/GlSceneObserver.h>
#include <tulip/GlComplexPolygon.h>
#include <tulip/GlConvexGraphHull.h>

using namespace tlp;

const quint32 NODES_ID = 1;
const quint32 EDGES_ID = 2;
const quint32 SELECTED_NODES_ID = 3;
const quint32 SELECTED_EDGES_ID = 4;
const quint32 META_NODES_ID = 5;
const quint32 SELECTED_META_NODES_ID = 6;
const quint32 META_NODE_LABELS_ID = 7;
const quint32 NODE_LABELS_ID = 8;
const quint32 EDGE_LABELS_ID = 9;
const QVector<quint32> GRAPH_COMPOSITE_IDS =
    QVector<quint32>() << NODES_ID << EDGES_ID << SELECTED_NODES_ID << SELECTED_EDGES_ID
                       << META_NODES_ID << SELECTED_META_NODES_ID << META_NODE_LABELS_ID
                       << NODE_LABELS_ID << EDGE_LABELS_ID;
const int NO_STENCIL = 0xFFFF;
const int FULL_STENCIL = 0x0002;

SceneLayersModel::SceneLayersModel(GlScene *scene, QObject *parent)
    : TulipModel(parent), _scene(scene) {
  _scene->addListener(this);
}

QModelIndex SceneLayersModel::index(int row, int column, const QModelIndex &parent) const {
  if (!hasIndex(row, column, parent))
    return QModelIndex();

  if (!parent.isValid()) { // Top level: layers
    GlLayer *layer = _scene->getLayersList()[row].second;
    assert(layer != nullptr);
    return createIndex(row, column, layer);
  }

  GlComposite *composite = nullptr;

  if (!parent.parent().isValid()) { // 1st sublevel, parent is a layer
    GlLayer *layer = static_cast<GlLayer *>(parent.internalPointer());
    composite = layer->getComposite();
  } else { // Deeper sublevel, the parent is a composite
    composite = static_cast<GlComposite *>(parent.internalPointer());
  }

  if (_scene->getGlGraphComposite() == composite)
    return createIndex(row, column, GRAPH_COMPOSITE_IDS[row]);

  int i = 0;

  for (auto &it : composite->getGlEntities()) {
    if (i++ == row)
      return createIndex(row, column, it.second);
  }

  return QModelIndex();
}

QModelIndex SceneLayersModel::graphCompositeIndex() const {
  for (const auto &itl : _scene->getLayersList()) {
    GlComposite *composite = itl.second->getComposite();
    int row = 0;

    for (auto &it : composite->getGlEntities()) {
      if (it.second == _scene->getGlGraphComposite())
        return createIndex(row, 0, _scene->getGlGraphComposite());

      ++row;
    }
  }

  return QModelIndex();
}

QModelIndex SceneLayersModel::parent(const QModelIndex &child) const {
  if (!child.isValid())
    return QModelIndex();

  if (GRAPH_COMPOSITE_IDS.contains(child.internalId()))
    return graphCompositeIndex();

  const auto &layers = _scene->getLayersList();

  for (const auto &itl : layers) {
    if (itl.second == child.internalPointer())
      return QModelIndex(); // Item was a layer, aka. a top level item.
  }

  GlSimpleEntity *entity = static_cast<GlSimpleEntity *>(child.internalPointer());
  GlComposite *parent = entity->getParent();

  if (parent == nullptr)
    return QModelIndex();

  GlComposite *ancestor = parent->getParent();

  if (ancestor == nullptr) { // Parent is a layer composite
    int row = 0;

    for (const auto &itl : layers) {
      if (itl.second->getComposite() == parent)
        return createIndex(row, 0, itl.second); // Item was a layer, aka. a top level item.

      row++;
    }
  }

  int row = 0;
  for (auto &it : ancestor->getGlEntities()) {
    if (it.second == parent)
      return createIndex(row, 0, parent);

    ++row;
  }

  return QModelIndex();
}

int SceneLayersModel::rowCount(const QModelIndex &parent) const {
  if (!parent.isValid()) { // Top level, layers count
    return _scene->getLayersList().size();
  }

  if (!parent.parent().isValid()) { // First sublevel: parent is a GlLayer
    GlLayer *layer = static_cast<GlLayer *>(parent.internalPointer());
    return layer->getComposite()->getGlEntities().size();
  }

  if (GRAPH_COMPOSITE_IDS.contains(parent.internalId()))
    return 0;

  GlSimpleEntity *entity = static_cast<GlSimpleEntity *>(parent.internalPointer());

  if (_scene->getGlGraphComposite() == entity)
    return GRAPH_COMPOSITE_IDS.size();

  GlComposite *composite = dynamic_cast<GlComposite *>(entity);
  if (composite != nullptr)
    return composite->getGlEntities().size();

  return 0;
}

int SceneLayersModel::columnCount(const QModelIndex &) const {
  // Name, Visible, Texture, Stencil
  return 4;
}

QVariant SceneLayersModel::data(const QModelIndex &index, int role) const {
  GlComposite *parent = nullptr;
  GlSimpleEntity *entity = nullptr;
  GlLayer *layer = nullptr;

  if (GRAPH_COMPOSITE_IDS.contains(index.internalId())) {
    quint32 id = index.internalId();
    GlGraphRenderingParameters *parameters =
        _scene->getGlGraphComposite()->getRenderingParametersPointer();
    QString display;
    int stencil = NO_STENCIL;
    bool visible = false;

    if (id == NODES_ID) {
      display = "Nodes";
      stencil = parameters->getNodesStencil();
      visible = parameters->isDisplayNodes();
    } else if (id == EDGES_ID) {
      display = "Edges";
      stencil = parameters->getEdgesStencil();
      visible = parameters->isDisplayEdges();
    } else if (id == SELECTED_NODES_ID) {
      display = "Selected nodes";
      stencil = parameters->getSelectedNodesStencil();
      visible = parameters->isDisplayNodes();
    } else if (id == SELECTED_EDGES_ID) {
      display = "Selected edges";
      stencil = parameters->getSelectedEdgesStencil();
      visible = parameters->isDisplayEdges();
    } else if (id == META_NODES_ID) {
      display = "Meta nodes content";
      stencil = parameters->getMetaNodesStencil();
      visible = parameters->isDisplayMetaNodes();
    } else if (id == SELECTED_META_NODES_ID) {
      display = "Selected meta nodes";
      stencil = parameters->getSelectedMetaNodesStencil();
      visible = parameters->isDisplayMetaNodes();
    } else if (id == META_NODE_LABELS_ID) {
      display = "Meta node content labels";
      stencil = parameters->getMetaNodesLabelStencil();
      visible = parameters->isViewMetaLabel();
    } else if (id == NODE_LABELS_ID) {
      display = "Node labels";
      stencil = parameters->getNodesLabelStencil();
      visible = parameters->isViewNodeLabel();
    } else if (id == EDGE_LABELS_ID) {
      display = "Edge labels";
      stencil = parameters->getEdgesLabelStencil();
      visible = parameters->isViewEdgeLabel();
    }

    if (role == Qt::DisplayRole && index.column() == 0)
      return display;

    if (role == Qt::CheckStateRole) {
      if (index.column() == 1)
        return (visible ? Qt::Checked : Qt::Unchecked);

      if (index.column() == 3)
        return (stencil == NO_STENCIL ? Qt::Unchecked : Qt::Checked);
    }

    return QVariant();
  }

  if (!index.parent().isValid()) {
    layer = static_cast<GlLayer *>(index.internalPointer());
    entity = layer->getComposite();
  } else {
    entity = static_cast<GlSimpleEntity *>(index.internalPointer());
    parent = entity->getParent();
  }

  if (role == Qt::DisplayRole && index.column() == 0) {
    if (layer != nullptr)
      return layer->getName().c_str();

    for (auto &it : parent->getGlEntities()) {
      if (it.second == entity)
        return it.first.c_str();
    }
  }

  if (role == Qt::FontRole && layer != nullptr) {
    QFont f;
    QWidget *p = dynamic_cast<QWidget *>(QAbstractItemModel::parent());
    if (p)
      f = p->font();

    f.setBold(true);
    return f;
  }

  if (role == Qt::CheckStateRole) {
    if (index.column() == 1)
      return (entity->isVisible() ? Qt::Checked : Qt::Unchecked);

    if (index.column() == 2) {
      GlComplexPolygon *pl = dynamic_cast<GlComplexPolygon *>(entity);
      if (pl)
        return (pl->textureActivation() ? Qt::Checked : Qt::Unchecked);
      else {
        GlConvexGraphHullsComposite *composite =
            dynamic_cast<GlConvexGraphHullsComposite *>(entity);
        if (composite)
          return (composite->hullsTextureActivation() ? Qt::Checked : Qt::Unchecked);
      }
    }

    if (index.column() == 3)
      return (entity->getStencil() == NO_STENCIL ? Qt::Unchecked : Qt::Checked);
  }

  if (role == Qt::TextAlignmentRole && index.column() != 0)
    return Qt::AlignCenter;

  return QVariant();
}

bool SceneLayersModel::setData(const QModelIndex &index, const QVariant &value, int role) {
  if (index.column() == 0 || role != Qt::CheckStateRole)
    return false;

  if (GRAPH_COMPOSITE_IDS.contains(index.internalId())) {
    quint32 id = index.internalId();
    GlGraphRenderingParameters *p = _scene->getGlGraphComposite()->getRenderingParametersPointer();

    if (index.column() == 1) {
      bool visible = value.value<int>() == int(Qt::Checked);

      if (id == NODES_ID)
        p->setDisplayNodes(visible);
      else if (id == EDGES_ID)
        p->setDisplayEdges(visible);
      else if (id == META_NODES_ID)
        p->setDisplayMetaNodes(visible);
      else if (id == NODE_LABELS_ID)
        p->setViewNodeLabel(visible);
      else if (id == EDGE_LABELS_ID)
        p->setViewEdgeLabel(visible);
      else if (id == META_NODE_LABELS_ID)
        p->setViewMetaLabel(visible);
    } else if (index.column() == 2) {
      int stencil = (value.value<int>() == int(Qt::Checked) ? FULL_STENCIL : NO_STENCIL);

      if (id == NODES_ID)
        p->setNodesStencil(stencil);
      else if (id == EDGES_ID)
        p->setEdgesStencil(stencil);
      else if (id == SELECTED_NODES_ID)
        p->setSelectedNodesStencil(stencil);
      else if (id == SELECTED_EDGES_ID)
        p->setSelectedEdgesStencil(stencil);
      else if (id == META_NODES_ID)
        p->setMetaNodesStencil(stencil);
      else if (id == SELECTED_META_NODES_ID)
        p->setSelectedMetaNodesStencil(stencil);
      else if (id == META_NODE_LABELS_ID)
        p->setMetaNodesLabelStencil(stencil);
      else if (id == NODE_LABELS_ID)
        p->setNodesLabelStencil(stencil);
      else if (id == EDGE_LABELS_ID)
        p->setEdgesLabelStencil(stencil);
    }

    emit drawNeeded(_scene);
    return true;
  }

  GlSimpleEntity *entity = nullptr;
  GlLayer *layer = nullptr;

  if (!index.parent().isValid()) {
    layer = static_cast<GlLayer *>(index.internalPointer());
    entity = layer->getComposite();
  } else
    entity = static_cast<GlSimpleEntity *>(index.internalPointer());

  bool val = value.value<int>() == int(Qt::Checked);

  if (index.column() == 1) {
    if (layer)
      layer->setVisible(val);

    entity->setVisible(val);
  } else if (index.column() == 2) {
    GlComplexPolygon *pl = dynamic_cast<GlComplexPolygon *>(entity);
    if (pl)
      pl->setTextureActivation(val);
    else {
      GlConvexGraphHullsComposite *composite = dynamic_cast<GlConvexGraphHullsComposite *>(entity);
      if (composite)
        composite->setHullsTextureActivation(val);
    }
  } else
    entity->setStencil(val ? FULL_STENCIL : 0xFFFF);

  emit drawNeeded(_scene);
  return true;
}

QVariant SceneLayersModel::headerData(int section, Qt::Orientation orientation, int role) const {
  if (orientation == Qt::Horizontal) {
    if (role == Qt::DisplayRole) {
      switch (section) {
      case 0:
        return "Name";
      case 1:
        return "Visible";
      case 2:
        return "Texture";
      case 3:
        return "Stencil";
      }
    }

    else if (role == Qt::TextAlignmentRole)
      return Qt::AlignCenter;
  }

  return TulipModel::headerData(section, orientation, role);
}

Qt::ItemFlags SceneLayersModel::flags(const QModelIndex &index) const {
  Qt::ItemFlags result = QAbstractItemModel::flags(index);

  if (index.column() != 0)
    result |= Qt::ItemIsUserCheckable;

  return result;
}

void SceneLayersModel::treatEvent(const Event &e) {
  if (e.type() == Event::TLP_MODIFICATION) {
    const GlSceneEvent *glse = dynamic_cast<const GlSceneEvent *>(&e);

    if (glse) {
      emit layoutAboutToBeChanged();

      // prevent dangling pointers to remain in the model persistent indexes
      if (glse->getSceneEventType() == GlSceneEvent::TLP_DELENTITY) {
        QModelIndexList persistentIndexes = persistentIndexList();

        for (int i = 0; i < persistentIndexes.size(); ++i) {
          if (persistentIndexes.at(i).internalPointer() == glse->getGlSimpleEntity()) {
            changePersistentIndex(persistentIndexes.at(i), QModelIndex());
            break;
          }
        }
      }

      emit layoutChanged();
    }
  }
}
