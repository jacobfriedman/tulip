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

#include <fstream>
#include <unordered_map>

#include <tulip/TulipPluginHeaders.h>

#include "GMLParser.h"

#define NODE "node"
#define EDGE "edge"
#define SOURCE "source"
#define TARGET "target"
#define GRAPH "graph"
#define LABEL "label"
#define ID "id"
#define GRAPHICS "graphics"
#define POINT "point"
#define LINE "Line"
#define DEFAULTVALUE "default"

using namespace std;
using namespace tlp;

void nodeAttributeError() {
  tlp::warning() << "Error reading node attribute: The attributes of nodes must be defined after "
                    "the node id (data ignored)"
                 << endl;
}
void edgeAttributeError() {
  tlp::warning() << "Error reading edge attribute: The attributes of edges must be defined after "
                    "source and target (data ignored)"
                 << endl;
}

//=================================================================================
struct GMLGraphBuilder : public GMLTrue {
  Graph *_graph;
  unordered_map<int, node> nodeIndex;
  ~GMLGraphBuilder() override {}
  GMLGraphBuilder(Graph *graph) : _graph(graph) {}
  bool addNode(int id) {
    if (nodeIndex.find(id) == nodeIndex.end()) {
      nodeIndex[id] = _graph->addNode();
    }

    return true;
  }
  edge addEdge(int idSource, int idTarget) {
    // return and invalid edge if one of the two nodes does not exits
    if (_graph->isElement(nodeIndex[idSource]) && _graph->isElement(nodeIndex[idTarget]))
      return _graph->addEdge(nodeIndex[idSource], nodeIndex[idTarget]);

    return edge();
  }
  bool setNodeValue(int nodeId, const string &propertyName, const string &value) {
    if (_graph->isElement(nodeIndex[nodeId])) {
      _graph->getLocalProperty<StringProperty>(propertyName)
          ->setNodeValue(nodeIndex[nodeId], value);
      return true;
    }

    return false;
  }
  bool setNodeValue(int nodeId, const string &propertyName, const double &value) {
    if (_graph->isElement(nodeIndex[nodeId])) {
      _graph->getLocalProperty<DoubleProperty>(propertyName)
          ->setNodeValue(nodeIndex[nodeId], value);
      return true;
    }

    return false;
  }
  bool setNodeValue(int nodeId, const string &propertyName, int value) {
    if (_graph->isElement(nodeIndex[nodeId])) {
      _graph->getLocalProperty<IntegerProperty>(propertyName)
          ->setNodeValue(nodeIndex[nodeId], value);
      return true;
    }

    return false;
  }
  bool setNodeValue(int nodeId, const string &propertyName, bool value) {
    if (_graph->isElement(nodeIndex[nodeId])) {
      _graph->getLocalProperty<BooleanProperty>(propertyName)
          ->setNodeValue(nodeIndex[nodeId], value);
      return true;
    }

    return false;
  }
  bool setNodeCoordValue(int nodeId, const string &propertyName, const Coord &value) {
    if (_graph->isElement(nodeIndex[nodeId])) {
      _graph->getLocalProperty<LayoutProperty>(propertyName)
          ->setNodeValue(nodeIndex[nodeId], value);
      return true;
    }

    return false;
  }
  bool setNodeSizeValue(int nodeId, const string &propertyName, const Size &value) {
    if (_graph->isElement(nodeIndex[nodeId])) {
      _graph->getLocalProperty<SizeProperty>(propertyName)->setNodeValue(nodeIndex[nodeId], value);
      return true;
    }

    return false;
  }
  bool setNodeValue(int nodeId, const string &propertyName, const Color &value) {
    if (_graph->isElement(nodeIndex[nodeId])) {
      _graph->getLocalProperty<ColorProperty>(propertyName)->setNodeValue(nodeIndex[nodeId], value);
      return true;
    }

    return false;
  }

  /**
   * Set the values of the property 2st param on the edge 1st param, to the value 3rd param.
   */
  bool setEdgeValue(edge, const string &, string) {
    return true;
  }
  /**
   * Set the values of the property 2st param on the edge 1st param, to the value 3rd param.
   */
  bool setEdgeValue(edge, const string &, int) {
    return true;
  }
  /**
   * Set the values of the property 2st param on the edge 1st param, to the value 3rd param.
   */
  bool setEdgeValue(edge, const string &, bool) {
    return true;
  }
  /**
   * Set the values of the property 2st param on the edge 1st param, to the value 3rd param.
   */
  bool setEdgeValue(edge, const string &, double) {
    return true;
  }
  void setEdgeValue(edge e, const LineType::RealType &lCoord) {
    _graph->getLocalProperty<LayoutProperty>("viewLayout")->setEdgeValue(e, lCoord);
  }
  bool setAllNodeValue(const string, const string, string) {
    return true;
  }
  /**
   * Set all the edges values of the property 1st param, of type 2nd param to the value 3rd param.
   */
  bool setAllEdgeValue(const string &, const string &, string) {
    return true;
  }
  bool addStruct(const string &structName, GMLBuilder *&newBuilder) override;
};
//=================================================================================
struct GMLNodeBuilder : public GMLBuilder {
  GMLGraphBuilder *graphBuilder;
  int idSet;

  GMLNodeBuilder(GMLGraphBuilder *graphBuilder) : graphBuilder(graphBuilder), idSet(-1) {}

  bool addInt(const string &st, const int id) override {
    if (st == ID) {
      bool result = graphBuilder->addNode(id);

      if (result)
        idSet = id;
      else
        return false;
    } else {
      if (idSet != -1)
        graphBuilder->setNodeValue(idSet, st, id);
      else
        nodeAttributeError();
    }

    return true;
  }
  bool addDouble(const string &st, const double real) override {
    if (idSet != -1)
      graphBuilder->setNodeValue(idSet, st, real);
    else
      nodeAttributeError();

    return true;
  }
  bool addString(const string &st, const string &str) override {
    if (idSet != -1) {
      if (st == LABEL)
        graphBuilder->setNodeValue(idSet, "viewLabel", str);
      else
        graphBuilder->setNodeValue(idSet, st, str);
    } else
      nodeAttributeError();

    return true;
  }
  bool addBool(const string &st, const bool boolean) override {
    if (idSet != -1)
      graphBuilder->setNodeValue(idSet, st, boolean);
    else
      nodeAttributeError();

    return true;
  }
  void setColor(const Color &color) {
    graphBuilder->setNodeValue(idSet, "viewColor", color);
  }
  void setSize(const Size &size) {
    graphBuilder->setNodeSizeValue(idSet, "viewSize", size);
  }
  void setCoord(const Coord &coord) {
    graphBuilder->setNodeCoordValue(idSet, "viewLayout", coord);
  }
  bool addStruct(const string &structName, GMLBuilder *&newBuilder) override;
  bool close() override {
    return true;
  }
};

//=================================================================================
struct GMLNodeGraphicsBuilder : public GMLTrue {
  GMLNodeBuilder *nodeBuilder;
  Coord coord;
  Size size;
  Color color;

  GMLNodeGraphicsBuilder(GMLNodeBuilder *nodeBuilder)
      : nodeBuilder(nodeBuilder), coord(Coord(0, 0, 0)), size(Size(1, 1, 1)),
        color(Color(0, 0, 0, 255)) {}

  bool addInt(const string &st, const int integer) override {
    if (st == "x")
      coord.setX(integer);

    if (st == "y")
      coord.setY(integer);

    if (st == "z")
      coord.setZ(integer);

    if (st == "w")
      size.setW(integer);

    if (st == "h")
      size.setH(integer);

    if (st == "d")
      size.setD(integer);

    return true;
  }
  bool addDouble(const string &st, const double real) override {
    if (st == "x")
      coord.setX(real);

    if (st == "y")
      coord.setY(real);

    if (st == "z")
      coord.setZ(real);

    if (st == "w")
      size.setW(real);

    if (st == "h")
      size.setH(real);

    if (st == "d")
      size.setD(real);

    return true;
  }
  bool addString(const string &st, const string &str) override {
    if (st == "fill") {
      // parse color in format #rrggbb
      if (str[0] == '#' && str.length() == 7) {
        char *c_str = const_cast<char *>(str.c_str()) + 1;

        for (int i = 0; i < 3; i++, c_str++) {
          unsigned char value = 0;

          if (isdigit(*c_str))
            value += (*c_str - '0') * 16;
          else
            value += ((tolower(*c_str) - 'a') + 10) * 16;

          c_str++;

          if (isdigit(*c_str))
            value += *c_str - '0';
          else
            value += (tolower(*c_str) - 'a') + 10;

          switch (i) {
          case 0:
            color.setR(value);
            break;

          case 1:
            color.setG(value);
            break;

          case 2:
            color.setB(value);
          }
        }
      }
    }

    return true;
  }
  bool close() override {
    nodeBuilder->setCoord(coord);
    nodeBuilder->setColor(color);
    nodeBuilder->setSize(size);
    return true;
  }
};
//=================================================================================
bool GMLNodeBuilder::addStruct(const string &structName, GMLBuilder *&newBuilder) {
  if (idSet == -1) {
    newBuilder = new GMLTrue();
    nodeAttributeError();
    return true;
  }

  if (structName == GRAPHICS)
    newBuilder = new GMLNodeGraphicsBuilder(this);
  else
    newBuilder = new GMLTrue();

  return true;
}
//=================================================================================
struct GMLEdgeBuilder : public GMLTrue {
  GMLGraphBuilder *graphBuilder;
  int source, target;
  bool edgeOk;
  edge curEdge;

  GMLEdgeBuilder(GMLGraphBuilder *graphBuilder)
      : graphBuilder(graphBuilder), source(-1), target(-1), edgeOk(false) {}
  bool addInt(const string &st, const int id) override {
    bool result = true;

    if (st == SOURCE)
      source = id;

    if (st == TARGET)
      target = id;

    if ((!edgeOk) && (source != -1) && (target != -1)) {
      edgeOk = true;
      curEdge = graphBuilder->addEdge(source, target);
    }

    if ((st != SOURCE) && (st != TARGET)) {
      if (edgeOk && curEdge.isValid())
        result = graphBuilder->setEdgeValue(curEdge, st, id);
      else
        edgeAttributeError();
    }

    return result;
  }
  bool addDouble(const string &st, const double real) override {
    if (edgeOk)
      graphBuilder->setEdgeValue(curEdge, st, real);
    else
      edgeAttributeError();

    return true;
  }
  bool addString(const string &st, const string &str) override {
    if (edgeOk)
      graphBuilder->setEdgeValue(curEdge, st, str);
    else
      edgeAttributeError();

    return true;
  }
  bool addBool(const string &st, const bool boolean) override {
    if (edgeOk)
      graphBuilder->setEdgeValue(curEdge, st, boolean);
    else
      edgeAttributeError();

    return true;
  }
  void setEdgeValue(const LineType::RealType &lCoord) {
    graphBuilder->setEdgeValue(curEdge, lCoord);
  }
  bool addStruct(const string &structName, GMLBuilder *&newBuilder) override;
  bool close() override {
    return true;
  }
};
//=================================================================================
struct GMLEdgeGraphicsBuilder : public GMLTrue {
  GMLEdgeBuilder *edgeBuilder;
  Size size;
  Color color;

  GMLEdgeGraphicsBuilder(GMLEdgeBuilder *edgeBuilder)
      : edgeBuilder(edgeBuilder), size(Size(0, 0, 0)), color(Color(0, 0, 0, 0)) {}
  bool addString(const string &, const string &) override {
    return true;
  }
  void setLine(const LineType::RealType &lCoord) {
    edgeBuilder->setEdgeValue(lCoord);
  }
  bool addStruct(const string &structName, GMLBuilder *&newBuilder) override;
  bool close() override {
    return true;
  }
};
//=================================================================================
struct GMLEdgeGraphicsLineBuilder : public GMLTrue {
  GMLEdgeGraphicsBuilder *edgeGraphicsBuilder;
  LineType::RealType lCoord;
  GMLEdgeGraphicsLineBuilder(GMLEdgeGraphicsBuilder *edgeGraphicsBuilder)
      : edgeGraphicsBuilder(edgeGraphicsBuilder) {}
  ~GMLEdgeGraphicsLineBuilder() override {}
  bool addStruct(const string &structName, GMLBuilder *&newBuilder) override;
  void addPoint(const Coord &coord) {
    lCoord.push_back(coord);
  }
  bool close() override {
    edgeGraphicsBuilder->setLine(lCoord);
    return true;
  }
};
//=================================================================================
struct GMLEdgeGraphicsLinePointBuilder : public GMLTrue {
  GMLEdgeGraphicsLineBuilder *edgeGraphicsLineBuilder;
  Coord coord;
  GMLEdgeGraphicsLinePointBuilder(GMLEdgeGraphicsLineBuilder *edgeGraphicsLineBuilder)
      : edgeGraphicsLineBuilder(edgeGraphicsLineBuilder), coord(0, 0, 0) {}
  bool addInt(const string &st, const int integer) override {
    if (st == "x")
      coord.setX(integer);

    if (st == "y")
      coord.setY(integer);

    if (st == "z")
      coord.setZ(integer);

    return true;
  }
  bool addDouble(const string &st, const double real) override {
    if (st == "x")
      coord.setX(real);

    if (st == "y")
      coord.setY(real);

    if (st == "z")
      coord.setZ(real);

    return true;
  }
  bool close() override {
    edgeGraphicsLineBuilder->addPoint(coord);
    return true;
  }
};
//=================================================================================
bool GMLEdgeGraphicsLineBuilder::addStruct(const string &structName, GMLBuilder *&newBuilder) {
  if (structName == POINT)
    newBuilder = new GMLEdgeGraphicsLinePointBuilder(this);
  else
    newBuilder = new GMLTrue();

  return true;
}
//=================================================================================
bool GMLEdgeGraphicsBuilder::addStruct(const string &structName, GMLBuilder *&newBuilder) {
  if (structName == LINE)
    newBuilder = new GMLEdgeGraphicsLineBuilder(this);
  else
    newBuilder = new GMLTrue();

  return true;
}
//=================================================================================
bool GMLEdgeBuilder::addStruct(const string &structName, GMLBuilder *&newBuilder) {
  if (!edgeOk) {
    newBuilder = new GMLTrue();
    edgeAttributeError();
    return true;
  }

  if (structName == GRAPHICS)
    newBuilder = new GMLEdgeGraphicsBuilder(this);
  else
    newBuilder = new GMLTrue();

  return true;
}
//=================================================================================
bool GMLGraphBuilder::addStruct(const string &structName, GMLBuilder *&newBuilder) {
  if (structName == GRAPH) {
    newBuilder = new GMLGraphBuilder(_graph);
  } else if (structName == NODE) {
    newBuilder = new GMLNodeBuilder(this);
  } else if (structName == EDGE) {
    newBuilder = new GMLEdgeBuilder(this);
  } else
    newBuilder = new GMLTrue();

  return true;
}
//=================================================================================

static const char *paramHelp[] = {
    // filename
    "The pathname of the GML file to import."};

/** \addtogroup import */

/// Import plugin for GML format.
/**
 * This plugin imports a graph structure recorded using the GML File format.
 * This format is the file format used by Graphlet.
 * See www.infosun.fmi.uni-passau.de/Graphlet/GML/ for details.
 */
class GMLImport : public ImportModule {
public:
  PLUGININFORMATION("GML", "Auber", "04/07/2001",
                    "<p>Supported extension: gml</p><p>Imports a new graph from a file (.gml) in "
                    "the GML input format (used by Graphlet).<p/>See "
                    "<a "
                    "href=\"http://www.infosun.fim.uni-passau.de/Graphlet/GML/gml-tr.html\">http://"
                    "www.infosun.fmi.uni-passau.de/Graphlet/GML/gml-tr.html</a> for details.</p>",
                    "1.1", "File")
  std::list<std::string> fileExtensions() const override {
    std::list<std::string> l;
    l.push_back("gml");
    return l;
  }
  GMLImport(PluginContext *context) : ImportModule(context) {
    addInParameter<string>("file::filename", paramHelp[0], "");
  }
  ~GMLImport() override {}
  bool importGraph() override {
    string filename;

    if (!dataSet->get<string>("file::filename", filename))
      return false;

    bool result = false;
    istream *is = tlp::getInputFileStream(filename);
    // check for open stream failure
    if (is->fail()) {
      std::stringstream ess;
      ess << "Unable to open " << filename << ": " << tlp::getStrError();
      pluginProgress->setError(ess.str());
    } else {
      GMLParser<true> myParser(*is, new GMLGraphBuilder(graph));
      myParser.parse();
      result = true;
    }
    delete is;
    return result;
  }
};

PLUGIN(GMLImport)
