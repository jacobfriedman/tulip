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
#include <ogdf/tree/TreeLayout.h>

#include <tulip2ogdf/OGDFLayoutPluginBase.h>

#include <tulip/StringCollection.h>

#define ORIENTATIONLIST "top to bottom;bottom to top;left to right;right to left"
#define TOPTOBOTTOM 0
#define BOTTOMTOTOP 1
#define LEFTTORIGHT 2
#define RIGHTTOLEFT 3

#define ROOTSELECTIONLIST "source;sink;by coord"
#define ROOTSOURCE 0
#define ROOTSINK 1
#define ROOTCOORD 2

using namespace tlp;
using namespace ogdf;

static const char *paramHelp[] = {
    // siblings distance
    "The minimal required horizontal distance between siblings.",

    // subtrees distance
    "The minimal required horizontal distance between subtrees.",

    // levels distance
    "The minimal required vertical distance between levels.",

    // trees distance
    "The minimal required horizontal distance between trees in the forest.",

    // orthogonal layout
    "Indicates whether orthogonal edge routing style is used or not.",

    // Orientation
    "This parameter indicates the orientation of the layout.",

    // Root selection
    "This parameter indicates how the root is selected."};

static const char *orientationValuesDescription =
    "top to Bottom <i>(edges are oriented from top to bottom)</i><br>"
    "bottom to top <i>(edges are oriented from bottom to top)</i><br>"
    "left to right <i>(edges are oriented from left to right)</i><br>"
    "right to left <i>(edges are oriented from right to left)</i>";

static const char *rootSelectionValuesDescription =
    "source <i>(select a source in the graph)</i><br>"
    "sink <i>(select a sink in the graph)</i><br>"
    "by coord <i>(use the coordinates, e.g., select the topmost node if orientation is "
    "top to bottom)</i>";

class OGDFTree : public OGDFLayoutPluginBase {

public:
  PLUGININFORMATION("Improved Walker (OGDF)", "Christoph Buchheim", "12/11/2007",
                    "Implements a linear-time tree layout algorithm with straight-line or "
                    "orthogonal edge routing.",
                    "1.6", "Tree")
  OGDFTree(const tlp::PluginContext *context)
      : OGDFLayoutPluginBase(context, context ? new ogdf::TreeLayout() : nullptr) {
    addInParameter<double>("siblings distance", paramHelp[0], "20");
    addInParameter<double>("subtrees distance", paramHelp[1], "20");
    addInParameter<double>("levels distance", paramHelp[2], "50");
    addInParameter<double>("trees distance", paramHelp[3], "50");
    addInParameter<bool>("orthogonal layout", paramHelp[4], "false");
    addInParameter<StringCollection>("orientation", paramHelp[5], ORIENTATIONLIST, true,
                                     orientationValuesDescription);
    addInParameter<StringCollection>("root selection", paramHelp[6], ROOTSELECTIONLIST, true,
                                     rootSelectionValuesDescription);
  }

  ~OGDFTree() override {}

  bool check(string &error) override {
    if (!tlp::TreeTest::isTree(graph)) {
      error += "graph is not a directed tree";
      return false;
    }

    return true;
  }

  void beforeCall() override {
    ogdf::TreeLayout *tree = static_cast<ogdf::TreeLayout *>(ogdfLayoutAlgo);

    if (dataSet != nullptr) {
      double dval = 0;
      bool bval = false;
      StringCollection sc;

      if (dataSet->get("siblings distance", dval))
        tree->siblingDistance(dval);

      if (dataSet->get("subtrees distance", dval))
        tree->subtreeDistance(dval);

      if (dataSet->get("levels distance", dval))
        tree->levelDistance(dval);

      if (dataSet->get("trees distance", dval))
        tree->treeDistance(dval);

      if (dataSet->get("orthogonal layout", bval))
        tree->orthogonalLayout(bval);

      if (dataSet->getDeprecated("orientation", "Orientation", sc)) {
        switch (sc.getCurrent()) {
        case TOPTOBOTTOM:
          // because of an ununderstanding fix
          // in thirdparty/OGDF/src/ogdf/tree/TreeLayout.cpp
          tree->orientation(Orientation::bottomToTop);
          break;
        case BOTTOMTOTOP:
          // same as above
          tree->orientation(Orientation::topToBottom);
          break;
        case LEFTTORIGHT:
          tree->orientation(Orientation::leftToRight);
          break;
        default:
          tree->orientation(Orientation::rightToLeft);
        }
      }

      if (dataSet->getDeprecated("root selection", "Root selection", sc)) {
        switch (sc.getCurrent()) {
        case ROOTSOURCE:
          tree->rootSelection(TreeLayout::RootSelectionType::Source);
          break;
        case ROOTSINK:
          tree->rootSelection(TreeLayout::RootSelectionType::Sink);
          break;
        default:
          tree->rootSelection(TreeLayout::RootSelectionType::ByCoord);
        }
      }
    }
  }
};

PLUGIN(OGDFTree)
