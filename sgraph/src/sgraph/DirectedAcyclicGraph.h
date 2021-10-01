// -*- tab-width: 2; indent-tabs-mode: nil; coding: utf-8-with-signature -*-
//-----------------------------------------------------------------------------
// Copyright 2000-2021 CEA (www.cea.fr) IFPEN (www.ifpenergiesnouvelles.com)
// See the top-level COPYRIGHT file for details.
// SPDX-License-Identifier: Apache-2.0
//-----------------------------------------------------------------------------
/*---------------------------------------------------------------------------*/
/* DirectedAcyclicGraph                                     (C) 2000-2021    */
/*                                                                           */
/* Basic Implementation of a directed acyclic graph                          */
/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

#ifndef SGRAPH_DIRECTEDACYCLICGRAPH_H
#define SGRAPH_DIRECTEDACYCLICGRAPH_H

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

#include "GraphBase.h"

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

namespace SGraph
{

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
/*! Template class for DirectedGraph.
 *
 */
template <class VertexType, class EdgeType>
class DirectedAcyclicGraph
: public GraphBase<VertexType, EdgeType>
{

/*---------------------------------------------------------------------------*/

 public:
  template <class ContainerT>
  struct SortedElementSet
  {
    template <class T>
    struct ReverseOrderSet
    {
      T m_elements;
      ReverseOrderSet(const T& elements) : m_elements(elements){}

      virtual ~ReverseOrderSet() = default;
      using iterator = typename std::reverse_iterator<typename T::iterator>;

      using const_iterator = typename std::reverse_iterator<typename T::const_iterator>;
      iterator begin() { return iterator(m_elements.end()); }

      const_iterator begin() const {return iterator(m_elements.end());}
      iterator end() { return iterator(m_elements.begin()); }

      const_iterator end() const { return iterator(m_elements.begin()); }

      int size() { return m_elements.size(); }
    };

    SortedElementSet(const ContainerT& elements)
    : m_elements(elements)
    {}

    SortedElementSet(ContainerT&& elements)
    : m_elements(elements)
    {}

    SortedElementSet() = default;
    virtual ~SortedElementSet() = default;

    using iterator = typename ContainerT::iterator;
    using const_iterator = typename ContainerT::const_iterator;

    iterator begin() { return m_elements.begin(); }
    const_iterator begin() const { return m_elements.begin(); }

    iterator end() { return m_elements.end(); }
    const_iterator end() const { return m_elements.end(); }

    int size() { return m_elements.size(); }

    ReverseOrderSet<ContainerT> reverseOrder() { return ReverseOrderSet<ContainerT>{m_elements}; }

    ContainerT m_elements;
  };

/*---------------------------------------------------------------------------*/

  using VertexRef = VertexType&;
  using EdgeRef = EdgeType& ;
  using Base = GraphBase<VertexType, EdgeType>;
  using SortedEdgeSet = SortedElementSet<typename Base::EdgeTypeRefArray>;
  using SortedVertexSet = SortedElementSet<typename Base::VertexTypeRefArray>;
  using VertexLevelMap = std::map<typename Base::VertexTypeConstRef, int, typename Base::VertexComparator>;
  using EdgeLevelMap = std::map<typename Base::EdgeTypeConstRef, int, typename Base::EdgeComparator>;

 private:
  std::set<typename Base::VertexTypeConstRef, typename Base::VertexComparator> m_colored_vertices{Base::m_vertex_comparator};
  VertexLevelMap m_vertex_level_map{ Base::m_vertex_comparator };
  EdgeLevelMap m_edge_level_map{ Base::m_edge_comparator };
  bool m_compute_vertex_levels = true;

 public:

  virtual ~DirectedAcyclicGraph() = default;

/*---------------------------------------------------------------------------*/

  void addEdge(const VertexType& source_vertex, const VertexType& target_vertex, const EdgeType& source_to_target_edge)
  {
    Base::addEdge(source_vertex, target_vertex, source_to_target_edge);
    m_compute_vertex_levels = true;
  }

/*---------------------------------------------------------------------------*/

  void addEdge(VertexType&& source_vertex, VertexType&& target_vertex, EdgeType&& source_to_target_edge)
  {
    Base::addEdge(source_vertex, target_vertex, source_to_target_edge);
    m_compute_vertex_levels = true;
  }

/*---------------------------------------------------------------------------*/

  SortedVertexSet topologicalSort()
  {
    if (m_compute_vertex_levels)
      _computeVertexLevels();
    typename Base::VertexTypeRefArray sorted_vertices;
    for (auto& vertex : this->m_vertices) {
      sorted_vertices.push_back(std::ref(vertex));
    }
    std::sort(sorted_vertices.begin(), sorted_vertices.end(), [&](const VertexType& a, const VertexType& b) { return m_vertex_level_map[a] < m_vertex_level_map[b]; });
    return SortedVertexSet{std::move(sorted_vertices)};
  }

/*---------------------------------------------------------------------------*/

  SortedEdgeSet spanningTree()
  {
    if (m_compute_vertex_levels)
      _computeVertexLevels();
    typename Base::EdgeTypeRefArray spaning_tree_edges;
    for (auto& edge : this->m_edges) {
      int level = _computeEdgeLevel(edge);
      m_edge_level_map[edge] = level;
      if (level < 0)
        continue; // edge must not be accounted for in spanning tree
      spaning_tree_edges.push_back(std::ref(edge));
    }
    std::sort(spaning_tree_edges.begin(), spaning_tree_edges.end(), [&](const EdgeType& a, const EdgeType& b) { return m_edge_level_map[a] < m_edge_level_map[b]; });
    return SortedEdgeSet{spaning_tree_edges};
  }

/*---------------------------------------------------------------------------*/

  void print()
  {
    if (m_compute_vertex_levels)
      _computeVertexLevels();
    std::cout << "--- Directed Graph ---" << std::endl;
    for (auto vertex_entry : this->m_adjacency_list) {
      _printGraphEntry(vertex_entry);
    }

    //std::ostringstream oss;
    //std::cout << oss.str() << std::endl;

    for (auto vertex_level_set_entry : m_vertex_level_map) {
      std::cout << "-- Graph has vertex "
                << vertex_level_set_entry.first.get() << " with level "
                << vertex_level_set_entry.second
                << std::endl;
    }
  }

/*---------------------------------------------------------------------------*/

  /*! Cycle detection is done with a lazy pattern triggered when topologicalSort() or print() are called.
   *  If a cycle is detected, these methods fail (throw runtime exception)
   */
  bool hasCycle()
  {
    bool has_cycle = false;
    try {
      _computeVertexLevels();
    }
    catch (std::runtime_error e) {
      has_cycle = true;
    }
    return has_cycle;
  }

/*---------------------------------------------------------------------------*/

 private:
  void _computeVertexLevels()
  {
    // Current algo cannot update vertex level ; need to clear the map.
    m_vertex_level_map.clear();
    // compute vertex level
    for (auto vertex_entry : this->m_adjacency_list) {
      _computeVertexLevel(vertex_entry.first, 0);
    }
    m_compute_vertex_levels = false;
  }

/*---------------------------------------------------------------------------*/

  void _computeVertexLevel(const VertexType& vertex, int level)
  {
    // Check for cycles
    if (!m_colored_vertices.insert(std::cref(vertex)).second)
      throw std::runtime_error("Cycle in graph. Exiting");

    // Try to insert vertex at the given level
    bool update_children = true;
    auto vertex_level_set_entry = m_vertex_level_map.insert(std::make_pair(std::cref(vertex), level)); // use emplace when available (gcc >= 4.8.0)
    if (!vertex_level_set_entry.second) // vertex already present
    {
      if (vertex_level_set_entry.first->second < level)
        vertex_level_set_entry.first->second = level;
      else
        update_children = false;
    }
    if (update_children) {
      auto vertex_adjacency_list = Base::m_adjacency_list.find(vertex);
      if (vertex_adjacency_list != Base::m_adjacency_list.end()) {
        ++level;
        for (auto child_vertex : vertex_adjacency_list->second.first) {
          _computeVertexLevel(child_vertex, level);
        }
      }
    }
    // Remove vertex from cycle detection
    m_colored_vertices.erase(vertex);
  }

/*---------------------------------------------------------------------------*/

  int _computeEdgeLevel(const EdgeType& edge)
  {
    auto edge_vertices_iterator = this->m_edge_to_vertex_map.find(edge);
    if (edge_vertices_iterator == this->m_edge_to_vertex_map.end())
      throw std::runtime_error("Not existing edge.");
    typename Base::VertexPair edge_vertices = edge_vertices_iterator->second;
    // edge level is set to source vertex level
    int edge_level = m_vertex_level_map[std::cref(edge_vertices.first)];
    // if the edge crosses graph levels (ie vertices level difference > 1), we put edge_level =-1, since we don't want to keep it in the spanning tree
    if (std::abs(edge_level - m_vertex_level_map[std::cref(edge_vertices.second)]) > 1)
      edge_level = -1;
    return edge_level;
  }

/*---------------------------------------------------------------------------*/

  void _printGraphEntry(const typename Base::AdjacencyListType::value_type& vertex_entry)
  {
    std::cout << "-- Vertex " << vertex_entry.first.get() << " depends on " << std::endl;
    for (auto connected_vertex : vertex_entry.second.first) {
      std::cout << "  - " << connected_vertex.get() << std::endl;
    }
  }
};

} // namespace SGraph

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

#endif //SGRAPH_DIRECTEDACYCLICGRAPH_H

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
