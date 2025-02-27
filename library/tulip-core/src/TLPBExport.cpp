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
#include <algorithm>
#include <unordered_map>

#include <tulip/TLPBExportImport.h>
#include <tulip/TlpTools.h>
#include <tulip/PropertyTypes.h>
#include <tulip/GraphProperty.h>

PLUGIN(TLPBExport)

using namespace tlp;
using namespace std;

// used to renumerate the hierarychy to be saved
static unsigned int getSavedId(Graph *g = nullptr) {
  static unsigned int nextSavedId = 0;
  static unordered_map<unsigned int, unsigned int> ids;
  if (g == nullptr) {
    // reset
    nextSavedId = 0;
    ids.clear();
    return 0;
  }
  auto id = g->getId();
  auto it = ids.find(id);
  if (it != ids.end())
    return it->second;
  ids[id] = nextSavedId;
  return nextSavedId++;
}

//================================================================================
void TLPBExport::getSubGraphs(Graph *g, vector<Graph *> &vsg) {
  // get subgraphs in a vector
  for (Graph *sg : g->subGraphs()) {
    vsg.push_back(sg);
    getSubGraphs(sg, vsg);
  }
}
//================================================================================
void TLPBExport::writeAttributes(ostream &os, Graph *g) {
  const DataSet &attributes = g->getAttributes();

  if (!attributes.empty()) {
    // If nodes and edges are stored as graph attributes
    // we need to update their ids before serializing them
    // as nodes and edges have been reindexed

    for (const pair<string, DataType *> &attribute : attributes.getValues()) {
      if (attribute.second->getTypeName() == string(typeid(node).name())) {
        node *n = static_cast<node *>(attribute.second->value);
        n->id = getNode(*n).id;
      } else if (attribute.second->getTypeName() == string(typeid(edge).name())) {
        edge *e = static_cast<edge *>(attribute.second->value);
        e->id = getEdge(*e).id;
      } else if (attribute.second->getTypeName() == string(typeid(vector<node>).name())) {
        vector<node> *vn = static_cast<vector<node> *>(attribute.second->value);

        for (size_t i = 0; i < vn->size(); ++i) {
          (*vn)[i].id = getNode((*vn)[i]).id;
        }
      } else if (attribute.second->getTypeName() == string(typeid(vector<edge>).name())) {
        vector<edge> *ve = static_cast<vector<edge> *>(attribute.second->value);

        for (size_t i = 0; i < ve->size(); ++i) {
          (*ve)[i].id = getEdge((*ve)[i]).id;
        }
      }
    }
  }

  unsigned int id = getSavedId(g);
  // write graph id
  os.write(reinterpret_cast<const char *>(&id), sizeof(id));
  // write graph attributes
  DataSet::write(os, attributes);
  // do not forget to write the end marker
  os.put(')');
}
//================================================================================
bool TLPBExport::exportGraph(std::ostream &os) {

  // header
  TLPBHeader header(graph->numberOfNodes(), graph->numberOfEdges());
  // write header
  os.write(reinterpret_cast<const char *>(&header), sizeof(header));
  // loop to write edges
  {
    pluginProgress->setComment("writing edges...");
    // use a vector as buffer
    std::vector<std::pair<node, node>> vEdges(MAX_EDGES_TO_WRITE);
    unsigned int edgesToWrite = 0, nbWrittenEdges = 0;
    for (auto e : graph->edges()) {
      std::pair<node, node> ends = graph->ends(e);
      ends.first = getNode(ends.first);
      ends.second = getNode(ends.second);
      vEdges[edgesToWrite] = std::move(ends);

      if (++edgesToWrite == MAX_EDGES_TO_WRITE) {
        // write already buffered edges
        os.write(reinterpret_cast<const char *>(vEdges.data()),
                 MAX_EDGES_TO_WRITE * sizeof(vEdges[0]));
        nbWrittenEdges += edgesToWrite;

        if (pluginProgress->progress(nbWrittenEdges, header.numNodes) != TLP_CONTINUE)
          return pluginProgress->state() != TLP_CANCEL;

        edgesToWrite = 0;
      }
    }

    if (edgesToWrite) {
      // write last buffered edges
      os.write(reinterpret_cast<const char *>(vEdges.data()),
               edgesToWrite * sizeof(std::pair<node, node>));
    }
  }
  // write subgraphs
  std::vector<Graph *> vSubGraphs;
  // get subgraphs in a vector
  getSubGraphs(graph, vSubGraphs);
  unsigned int numSubGraphs = vSubGraphs.size();
  {
    pluginProgress->setComment("writing subgraphs...");
    // write nb subgraphs
    os.write(reinterpret_cast<const char *>(&numSubGraphs), sizeof(numSubGraphs));

    // initialize graph hierarchy ids
    getSavedId();
    for (unsigned int i = 0; i < numSubGraphs; ++i) {
      Graph *sg = vSubGraphs[i];
      unsigned int parentId = getSavedId(sg->getSuperGraph());

      std::pair<unsigned int, unsigned int> ids(getSavedId(sg), parentId);
      // write ids
      os.write(reinterpret_cast<const char *>(&ids), sizeof(ids));
      // loop to write sg nodes ranges
      {
        // first sort sg nodes
        const std::vector<node> &nodes = sg->nodes();
        unsigned int nbNodes = nodes.size();
        std::vector<node> sgNodes(nbNodes);

        for (unsigned int j = 0; j < nbNodes; ++j)
          sgNodes[j] = getNode(nodes[j]);

        std::sort(sgNodes.begin(), sgNodes.end());

        // use a vector as buffer
        std::vector<std::vector<std::pair<node, node>>> vRangesVec;
        vRangesVec.emplace_back(MAX_RANGES_TO_WRITE);
        auto &vRanges = vRangesVec.back();

        unsigned int rangesToWrite = 0;
        unsigned int numRanges = 0;

        bool pendingWrite = false;
        node beginNode, lastNode;

        for (unsigned int j = 0; j < nbNodes; ++j) {
          node current = sgNodes[j];
          pendingWrite = true;

          if (!beginNode.isValid())
            beginNode = lastNode = current;
          else {
            if (current.id == lastNode.id + 1)
              lastNode = current;
            else {
              vRanges[rangesToWrite++] = std::pair<node, node>(beginNode, lastNode);
              ++numRanges;
              beginNode = lastNode = current;

              if (rangesToWrite == MAX_RANGES_TO_WRITE) {
                vRangesVec.emplace_back(MAX_RANGES_TO_WRITE);
                vRanges = vRangesVec.back();
                rangesToWrite = 0;
                pendingWrite = false;
              }
            }
          }
        }

        if (pendingWrite) {
          // insert last range in buffer
          vRanges[rangesToWrite++] = std::pair<node, node>(beginNode, lastNode);
          ++numRanges;
        }

        // mark nb ranges
        os.write(reinterpret_cast<const char *>(&numRanges), sizeof(numRanges));
        // write already buffered ranges
        int numRangesV = numRanges / MAX_RANGES_TO_WRITE;

        for (int j = 0; j < numRangesV; ++j) {
          os.write(reinterpret_cast<const char *>(vRangesVec[j].data()),
                   MAX_RANGES_TO_WRITE * sizeof(vRangesVec[j][0]));
        }

        // write last buffered ranges
        os.write(reinterpret_cast<const char *>(vRanges.data()),
                 rangesToWrite * sizeof(vRanges[0]));
      }
      // loop to write sg edges ranges
      {
        // first sort sg edges
        const std::vector<edge> &edges = sg->edges();
        unsigned int nbEdges = edges.size();
        std::vector<edge> sgEdges(nbEdges);

        for (unsigned int j = 0; j < nbEdges; ++j)
          sgEdges[j] = getEdge(edges[j]);

        std::sort(sgEdges.begin(), sgEdges.end());

        // use a vector as buffer
        std::vector<std::vector<std::pair<edge, edge>>> vRangesVec;
        vRangesVec.emplace_back(MAX_RANGES_TO_WRITE);
        auto &vRanges = vRangesVec.back();

        unsigned int rangesToWrite = 0;
        unsigned int numRanges = 0;

        bool pendingWrite = false;
        edge beginEdge, lastEdge;

        for (unsigned int j = 0; j < nbEdges; ++j) {
          edge current = sgEdges[j];
          pendingWrite = true;

          if (!beginEdge.isValid())
            beginEdge = lastEdge = current;
          else {
            if (current.id == lastEdge.id + 1)
              lastEdge = current;
            else {
              vRanges[rangesToWrite++] = std::pair<edge, edge>(beginEdge, lastEdge);
              ++numRanges;
              beginEdge = lastEdge = current;

              if (rangesToWrite == MAX_RANGES_TO_WRITE) {
                vRangesVec.emplace_back(MAX_RANGES_TO_WRITE);
                vRanges = vRangesVec.back();
                rangesToWrite = 0;
                pendingWrite = false;
              }
            }
          }
        }

        if (pendingWrite) {
          // insert last range in buffer
          vRanges[rangesToWrite++] = std::pair<edge, edge>(beginEdge, lastEdge);
          ++numRanges;
        }

        // mark nb ranges
        os.write(reinterpret_cast<const char *>(&numRanges), sizeof(numRanges));
        // write already buffered ranges
        int numRangesV = numRanges / MAX_RANGES_TO_WRITE;

        for (int j = 0; j < numRangesV; ++j) {
          os.write(reinterpret_cast<const char *>(vRangesVec[j].data()),
                   MAX_RANGES_TO_WRITE * sizeof(vRangesVec[j][0]));
        }

        // write last buffered ranges
        os.write(reinterpret_cast<const char *>(vRanges.data()),
                 rangesToWrite * sizeof(vRanges[0]));
      }

      if (pluginProgress->progress(i, numSubGraphs) != TLP_CONTINUE)
        return pluginProgress->state() != TLP_CANCEL;
    }
  }
  // write properties
  {
    pluginProgress->setComment("writing properties...");
    unsigned int numGraphProperties = 0;
    unsigned int numProperties = 0;
    std::vector<PropertyInterface *> props;
    // get local properties in a vector
    for (PropertyInterface *prop : graph->getObjectProperties()) {
      props.push_back(prop);
      ++numProperties;
      ++numGraphProperties;
    }

    // get subgraphs local properties too
    for (unsigned int i = 0; i < numSubGraphs; ++i) {
      Graph *sg = vSubGraphs[i];
      for (PropertyInterface *prop : sg->getLocalObjectProperties()) {
        props.push_back(prop);
        ++numProperties;
      }
    }

    // write nb properties
    os.write(reinterpret_cast<const char *>(&numProperties), sizeof(numProperties));

    // loop on properties
    for (unsigned int i = 0; i < numProperties; ++i) {
      PropertyInterface *prop = props[i];
      std::string nameOrType = prop->getName();
      unsigned int size = nameOrType.size();
      // write property name
      os.write(reinterpret_cast<const char *>(&size), sizeof(size));
      os.write(reinterpret_cast<const char *>(nameOrType.data()), size);
      // write graph id
      unsigned int propGraphId = (i < numGraphProperties) ? 0 : getSavedId(prop->getGraph());

      os.write(reinterpret_cast<const char *>(&propGraphId), sizeof(propGraphId));
      // special treament for pathnames view properties
      bool pnViewProp = (nameOrType == string("viewFont") || nameOrType == string("viewTexture"));
      // write type
      nameOrType = prop->getTypename();
      size = nameOrType.size();
      os.write(reinterpret_cast<const char *>(&size), sizeof(size));
      os.write(reinterpret_cast<const char *>(nameOrType.data()), size);

      if (pnViewProp && !TulipBitmapDir.empty()) {
        string defVal = prop->getNodeDefaultStringValue();
        size_t pos = defVal.find(TulipBitmapDir);

        if (pos != string::npos)
          defVal.replace(pos, TulipBitmapDir.size(), "TulipBitmapDir/");

        StringType::writeb(os, defVal);

        defVal = prop->getEdgeDefaultStringValue();
        pos = defVal.find(TulipBitmapDir);

        if (pos != string::npos)
          defVal.replace(pos, TulipBitmapDir.size(), "TulipBitmapDir/");

        StringType::writeb(os, defVal);
      } else {
        // write node default value
        prop->writeNodeDefaultValue(os);
        // write edge default value
        prop->writeEdgeDefaultValue(os);
      }

      // write nodes values
      {
        // write nb of non default values
        size = prop->numberOfNonDefaultValuatedNodes(propGraphId ? nullptr : graph);
        os.write(reinterpret_cast<const char *>(&size), sizeof(size));
        // prepare output stream
        stringstream vs;

        // std::basic_streambuf::pubsetbuf is a no-op in libcxx (LLVM implementation of STL)
        // see https://github.com/llvm-mirror/libcxx/blob/master/include/streambuf#L150
        // and https://github.com/llvm-mirror/libcxx/blob/master/include/streambuf#L360
        // and also in STL implementation of Microsoft Visual C++
        // so fallback writing directly to the output stream in these cases
#if defined(_LIBCPP_VERSION) || defined(_MSC_VER)
        bool canUsePubSetBuf = false;
        std::ostream &s = os;
#else
        bool canUsePubSetBuf = true;
        std::ostream &s = vs;
#endif
        char *vBuf = nullptr;
        unsigned int valueSize = prop->nodeValueSize();

        if (valueSize && canUsePubSetBuf) {
          // allocate a special buffer for values
          // this will ease the write of a bunch of values
          vBuf =
              static_cast<char *>(malloc(MAX_VALUES_TO_WRITE * (sizeof(unsigned int) + valueSize)));
          vs.rdbuf()->pubsetbuf(vBuf, MAX_VALUES_TO_WRITE * (sizeof(unsigned int) + valueSize));
        }

        // loop on nodes
        unsigned int nbValues = 0;
        // when exporting the GraphProperty
        // we will have to check if the node pointed
        // subgraph is a descendant graph of this graph
        bool checkForMetaGraph = prop->getTypename() == GraphProperty::propertyTypename;
        for (auto n : prop->getNonDefaultValuatedNodes(propGraphId ? nullptr : graph)) {
          // write current node reindexed id
          size = getNode(n).id;
          s.write(reinterpret_cast<const char *>(&size), sizeof(size));

          if (pnViewProp && !TulipBitmapDir.empty()) { // viewFont || viewTexture
            string sVal = prop->getNodeStringValue(n);
            size_t pos = sVal.find(TulipBitmapDir);

            if (pos != string::npos)
              sVal.replace(pos, TulipBitmapDir.size(), "TulipBitmapDir/");

            StringType::writeb(s, sVal);
          } else if (checkForMetaGraph) {
            string tmp = prop->getNodeStringValue(n);
            unsigned int id = strtoul(tmp.c_str(), nullptr, 10);
            auto sg = graph->getDescendantGraph(id);
            // record a null value if the node pointed subgraph
            // is not a descendant of the currently exported graph
            UnsignedIntegerType::writeb(s, sg ? getSavedId(sg) : 0);
          } else
            prop->writeNodeValue(s, n);

          ++nbValues;

          if (nbValues == MAX_VALUES_TO_WRITE && canUsePubSetBuf) {
            // write already buffered values
            if (vBuf)
              os.write(vBuf, MAX_VALUES_TO_WRITE * (sizeof(unsigned int) + valueSize));
            else {
              std::string sbuf = vs.str();
              size = uint(vs.tellp());
              // write buffer
              os.write(sbuf.c_str(), size);
            }

            // reset for next write
            vs.seekp(0, ios_base::beg);
            nbValues = 0;
          }
        }

        if (nbValues && canUsePubSetBuf) {
          // write last buffered values
          if (vBuf) {
            os.write(vBuf, nbValues * (sizeof(unsigned int) + valueSize));
          } else {
            std::string sbuf = vs.str();
            size = uint(vs.tellp());
            // write buffer
            os.write(sbuf.c_str(), size);
          }
        }

        if (vBuf) {
          free(vBuf);
        }
      }
      // write edges values
      {
        // write nb of non default values
        size = prop->numberOfNonDefaultValuatedEdges(propGraphId ? nullptr : graph);
        os.write(reinterpret_cast<const char *>(&size), sizeof(size));
        // prepare output stream
        stringstream vs;

        // std::basic_streambuf::pubsetbuf is a no-op in libcxx (LLVM implementation of STL)
        // see https://github.com/llvm-mirror/libcxx/blob/master/include/streambuf#L150
        // and https://github.com/llvm-mirror/libcxx/blob/master/include/streambuf#L360
        // and also in STL implementation of Microsoft Visual C++
        // so fallback writing directly to the output stream in these cases
#if defined(_LIBCPP_VERSION) || defined(_MSC_VER)
        bool canUsePubSetBuf = false;
        ostream &s = os;
#else
        bool canUsePubSetBuf = true;
        ostream &s = vs;
#endif
        char *vBuf = nullptr;
        unsigned int valueSize = prop->edgeValueSize();
        bool isGraphProperty = false;

        if (valueSize && canUsePubSetBuf) {
          // allocate a special buffer for values
          // this will ease the write of a bunch of values
          vBuf =
              static_cast<char *>(malloc(MAX_VALUES_TO_WRITE * (sizeof(unsigned int) + valueSize)));
          vs.rdbuf()->pubsetbuf(vBuf, MAX_VALUES_TO_WRITE * (sizeof(unsigned int) + valueSize));
        } else {
          if (prop->getTypename() == GraphProperty::propertyTypename)
            isGraphProperty = true;
        }

        // loop on edges
        unsigned int nbValues = 0;
        for (auto e : prop->getNonDefaultValuatedEdges(propGraphId ? nullptr : graph)) {
          size = getEdge(e).id;
          s.write(reinterpret_cast<const char *>(&size), sizeof(size));

          if (isGraphProperty) {
            // reindex embedded edges
            const set<edge> &eEdges = static_cast<GraphProperty *>(prop)->getEdgeValue(e);
            set<edge> rEdges;

            for (auto eEdge : eEdges) {
              // reindex only embedded edges belonging to the exported graph
              if (!graph->isElement(eEdge))
                continue;
              edge rEdge = getEdge(eEdge);
              rEdges.insert(rEdge);
            }

            // finally save set
            EdgeSetType::writeb(s, rEdges);
          } else if (pnViewProp && !TulipBitmapDir.empty()) { // viewFont || viewTexture
            string sVal = prop->getEdgeStringValue(e);
            size_t pos = sVal.find(TulipBitmapDir);

            if (pos != string::npos)
              sVal.replace(pos, TulipBitmapDir.size(), "TulipBitmapDir/");

            StringType::writeb(s, sVal);
          } else
            prop->writeEdgeValue(s, e);

          ++nbValues;

          if (nbValues == MAX_VALUES_TO_WRITE && canUsePubSetBuf) {
            // write already buffered values
            if (vBuf)
              os.write(vBuf, MAX_VALUES_TO_WRITE * (sizeof(unsigned int) + valueSize));
            else {
              std::string sbuf = vs.str();
              size = uint(vs.tellp());
              // write buffer
              os.write(sbuf.c_str(), size);
            }

            // reset for next write
            vs.seekp(0, ios_base::beg);
            nbValues = 0;
          }
        }

        if (nbValues && canUsePubSetBuf) {
          // write last buffered values
          if (vBuf) {
            os.write(vBuf, nbValues * (sizeof(unsigned int) + valueSize));
          } else {
            std::string sbuf = vs.str();
            size = uint(vs.tellp());
            // write buffer
            os.write(sbuf.c_str(), size);
          }
        }

        if (vBuf) {
          free(vBuf);
        }
      }

      if (pluginProgress->progress(i, numProperties) != TLP_CONTINUE)
        return pluginProgress->state() != TLP_CANCEL;
    }
  }
  // write graph and sub graphs attributes
  writeAttributes(os, graph);

  for (unsigned int i = 0; i < numSubGraphs; ++i)
    writeAttributes(os, vSubGraphs[i]);

  return true;
}
