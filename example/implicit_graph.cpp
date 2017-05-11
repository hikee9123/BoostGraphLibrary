
//          Copyright W.P. McNeill 2010.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <boost/concept/assert.hpp>
#include <boost/graph/adjacency_iterator.hpp>
#include <boost/graph/dijkstra_shortest_paths.hpp>
#include <boost/graph/graph_concepts.hpp>
#include <boost/graph/properties.hpp>
#include <boost/iterator/counting_iterator.hpp>
#include <boost/iterator/iterator_facade.hpp>
#include <iostream>
#include <utility>
#include "range_pair.hpp"


/*
This file defines a simple example of a read-only implicit weighted graph
built using the Boost Graph Library. It can be used as a starting point for
developers creating their own implicit graphs.

The graph models the following concepts:
  Graph
  IncidenceGraph
  BidirectionalGraph
  AdjacencyGraph
  VertexListGraph
  EdgeListGraph
  AdjacencyMatrix
  ReadablePropertyGraph

The graph defined here is a ring graph, a graph whose vertices are arranged in
a ring so that each vertex has exactly two neighbors. For example, here is a
ring graph with five nodes.

                                    0
                                  /   \
                                4      1
                                |      |
                                3 ---- 2

The edges of this graph are undirected and each has a weight that is a
function of its position in the graph.

The vertices indexed are by integer and arranged sequentially so that each
vertex i is adjacent to i-1 for i>0 and i+1 for i<n-1.  Vertex 0 is also
adjacent to vertex n-1.  Edges are indexed by pairs of vertex indices.

Various aspects of the graph are modeled by the following classes:

  ring_graph
    The graph class instantiated by a client. This defines types for the
    concepts that this graph models and keeps track of the number of vertices
    in the graph.

  ring_incident_edge_iterator
    This is an iterator that ranges over edges incident on a given vertex. The
    behavior of this iterator defines the ring topology. Other iterators that
    make reference to the graph structure are defined in terms of this one.

  edge_weight_map
    This defines a property map between edges and weights. Here edges have a
    weight equal to the average of their endpoint vertex indices, i.e. edge
    (2,3) has weight 2.5, edge (0,4) has weight 2, etc.

  boost::property_map<graph, boost::edge_weight_t>
    This tells Boost to associate the edges of the ring graph with the edge
    weight map.

Along with these classes, the graph concepts are modeled by various valid
expression functions defined below.  This example also defines a
get(boost::vertex_index_t, const ring_graph&) function which isn't part of a
graph concept, but is used for Dijkstra search.

Apart from graph, client code should not instantiate the model classes
directly. Instead it should access them and their properties via
graph_traits<...> and property_traits<...> lookups. For convenience,
this example defines short names for all these properties that client code can
use.
*/

// Forward declarations
class ring_graph;
class ring_incident_edge_iterator;
class ring_adjacency_iterator;
class ring_edge_iterator;
struct edge_weight_map;

// ReadablePropertyGraph associated types
namespace boost {
  template<>
  struct property_map<ring_graph, edge_weight_t> {
    using type = edge_weight_map;
    using const_type = edge_weight_map;
  };

  template<>
  struct property_map<const ring_graph, edge_weight_t> {
    using type = edge_weight_map;
    using const_type = edge_weight_map;
  };
}

// Tag values that specify the traversal type in graph::traversal_category.
struct ring_traversal_catetory:
  virtual public boost::bidirectional_graph_tag,
  virtual public boost::adjacency_graph_tag,
  virtual public boost::vertex_list_graph_tag,
  virtual public boost::edge_list_graph_tag
  {};


/*
Undirected graph of vertices arranged in a ring shape.

Vertices are indexed by integer, and edges connect vertices with consecutive
indices.  Vertex 0 is also adjacent to the vertex n-1.
*/
class ring_graph {
public:
  // Graph associated types
  using vertex_descriptor = std::size_t;
  using directed_category = boost::undirected_tag;
  using edge_parallel_category = boost::disallow_parallel_edge_tag;
  using traversal_category = ring_traversal_catetory;

  // IncidenceGraph associated types
  using edge_descriptor = std::pair<vertex_descriptor, vertex_descriptor>;
  using out_edge_iterator = ring_incident_edge_iterator;
  using degree_size_type = std::size_t;

  // BidirectionalGraph associated types
  // Note that undirected graphs make no distinction between in- and out-
  // edges.
  using in_edge_iterator = ring_incident_edge_iterator;

  // AdjacencyGraph associated types
  using adjacency_iterator = ring_adjacency_iterator;

  // VertexListGraph associated types
  using vertex_iterator = boost::counting_iterator<vertex_descriptor>;
  using vertices_size_type = std::size_t;

  // EdgeListGraph associated types
  using edge_iterator = ring_edge_iterator;
  using edges_size_type = std::size_t;

  // This type is not part of a graph concept, but is used to return the
  // default vertex index map used by the Dijkstra search algorithm.
  using vertex_property_type = vertex_descriptor;

  ring_graph(std::size_t n):m_n(n) {};
  std::size_t n() const {return m_n;}
private:
  // The number of vertices in the graph.
  std::size_t m_n;
};

// Use these graph_traits parameterizations to refer to the associated
// graph types.
using vertex_descriptor = boost::graph_traits<ring_graph>::vertex_descriptor;
using edge_descriptor = boost::graph_traits<ring_graph>::edge_descriptor;
using out_edge_iterator = boost::graph_traits<ring_graph>::out_edge_iterator;
using in_edge_iterator = boost::graph_traits<ring_graph>::in_edge_iterator;
using adjacency_iterator = boost::graph_traits<ring_graph>::adjacency_iterator;
using degree_size_type = boost::graph_traits<ring_graph>::degree_size_type;
using vertex_iterator = boost::graph_traits<ring_graph>::vertex_iterator;
using vertices_size_type = boost::graph_traits<ring_graph>::vertices_size_type;
using edge_iterator = boost::graph_traits<ring_graph>::edge_iterator;
using edges_size_type = boost::graph_traits<ring_graph>::edges_size_type;


// Tag values passed to an iterator constructor to specify whether it should
// be created at the start or the end of its range.
struct iterator_position {};
struct iterator_start:virtual public iterator_position {};
struct iterator_end:virtual public iterator_position {};

/*
Iterator over edges incident on a vertex in a ring graph.

For vertex i, this returns edge (i, i+1) and then edge (i, i-1), wrapping
around the end of the ring as needed.

It is implemented with the boost::iterator_adaptor class, adapting an
offset into the dereference::ring_offset array.
*/
class ring_incident_edge_iterator:public boost::iterator_adaptor <
    ring_incident_edge_iterator,
    boost::counting_iterator<std::size_t>,
    edge_descriptor,
    boost::use_default,
    edge_descriptor > {
public:
  ring_incident_edge_iterator():
    ring_incident_edge_iterator::iterator_adaptor_(0),m_n(0),m_u(0) {};
  explicit ring_incident_edge_iterator(const ring_graph& g,
                                       vertex_descriptor u,
                                       iterator_start):
    ring_incident_edge_iterator::iterator_adaptor_(0),
    m_n(g.n()),m_u(u) {};
  explicit ring_incident_edge_iterator(const ring_graph& g,
                                       vertex_descriptor u,
                                       iterator_end):
    // A graph with one vertex only has a single self-loop.  A graph with
    // two vertices has a single edge between them.  All other graphs have
    // two edges per vertex.
    ring_incident_edge_iterator::iterator_adaptor_(g.n() > 2 ? 2:1),
    m_n(g.n()),m_u(u) {};

private:
  friend class boost::iterator_core_access;

  edge_descriptor dereference() const {
    static const int ring_offset[] = {1, -1};
    vertex_descriptor v;

    auto p = *this->base_reference();
    if (m_u == 0 && p == 1)
      v = m_n-1; // Vertex n-1 precedes vertex 0.
    else
      v = (m_u+ring_offset[p]) % m_n;
    return edge_descriptor(m_u, v);
  }

  std::size_t m_n; // Size of the graph
  vertex_descriptor m_u; // Vertex whose out edges are iterated
};


// IncidenceGraph valid expressions
vertex_descriptor source(edge_descriptor e, const ring_graph&) {
  // The first vertex in the edge pair is the source.
  return e.first;
}


vertex_descriptor target(edge_descriptor e, const ring_graph&) {
 // The second vertex in the edge pair is the target.
 return e.second;
}

std::pair<out_edge_iterator, out_edge_iterator>
out_edges(vertex_descriptor u, const ring_graph& g) {
  return std::pair<out_edge_iterator, out_edge_iterator>(
    out_edge_iterator(g, u, iterator_start()),
    out_edge_iterator(g, u, iterator_end()) );
}


degree_size_type out_degree(vertex_descriptor, const ring_graph&) {
  // All vertices in a ring graph have two neighbors.
  return 2;
}


// BidirectionalGraph valid expressions
std::pair<in_edge_iterator, in_edge_iterator>
in_edges(vertex_descriptor u, const ring_graph& g) {
  // The in-edges and out-edges are the same in an undirected graph.
  return out_edges(u, g);
}

degree_size_type in_degree(vertex_descriptor u, const ring_graph& g) {
  // The in-degree and out-degree are both equal to the number of incident
  // edges in an undirected graph.
  return out_degree(u, g);
}

degree_size_type degree(vertex_descriptor u, const ring_graph& g) {
  // The in-degree and out-degree are both equal to the number of incident
  // edges in an undirected graph.
  return out_degree(u, g);
}


/*
Iterator over vertices adjacent to a given vertex.

This iterates over the target vertices of all the incident edges.
*/
class ring_adjacency_iterator:public boost::adjacency_iterator_generator<
  ring_graph,
  vertex_descriptor,
  out_edge_iterator>::type {
  // The parent class is an iterator_adpator that turns an iterator over
  // out edges into an iterator over adjacent vertices.
  using parent_class = boost::adjacency_iterator_generator<
    ring_graph,
    vertex_descriptor,
    out_edge_iterator>::type;
public:
  ring_adjacency_iterator() {};
  ring_adjacency_iterator(vertex_descriptor u,
                          const ring_graph& g,
                          iterator_start):
    parent_class(out_edge_iterator(g, u, iterator_start()), &g) {};
  ring_adjacency_iterator(vertex_descriptor u,
                          const ring_graph& g,
                          iterator_end):
    parent_class(out_edge_iterator(g, u, iterator_end()), &g) {};
};


// AdjacencyGraph valid expressions
std::pair<adjacency_iterator, adjacency_iterator>
adjacent_vertices(vertex_descriptor u, const ring_graph& g) {
  return std::pair<adjacency_iterator, adjacency_iterator>(
    adjacency_iterator(u, g, iterator_start()),
    adjacency_iterator(u, g, iterator_end()));
}


// VertexListGraph valid expressions
vertices_size_type num_vertices(const ring_graph& g) {
  return g.n();
};

std::pair<vertex_iterator, vertex_iterator> vertices(const ring_graph& g) {
  return std::pair<vertex_iterator, vertex_iterator>(
    vertex_iterator(0),                 // The first iterator position
    vertex_iterator(num_vertices(g)) ); // The last iterator position
}


/*
Iterator over edges in a ring graph.

This object iterates over all the vertices in the graph, then for each
vertex returns its first outgoing edge.

It is implemented with the boost::iterator_adaptor class, because it is
essentially a vertex_iterator with a customized deference operation.
*/
class ring_edge_iterator:public boost::iterator_adaptor<
  ring_edge_iterator,
  vertex_iterator,
  edge_descriptor,
  boost::use_default,
  edge_descriptor > {
public:
  ring_edge_iterator():
    ring_edge_iterator::iterator_adaptor_(0),m_g(NULL) {};
  explicit ring_edge_iterator(const ring_graph& g, iterator_start):
    ring_edge_iterator::iterator_adaptor_(vertices(g).first),m_g(&g) {};
  explicit ring_edge_iterator(const ring_graph& g, iterator_end):
    ring_edge_iterator::iterator_adaptor_(
      // Size 2 graphs have a single edge connecting the two vertices.
      g.n() == 2 ? ++(vertices(g).first) : vertices(g).second ),
    m_g(&g) {};

private:
  friend class boost::iterator_core_access;

  edge_descriptor dereference() const {
    // The first element in the incident edge list of the current vertex.
    return *(out_edges(*this->base_reference(), *m_g).first);
  }

  // The graph being iterated over
  const ring_graph *m_g;
};

// EdgeListGraph valid expressions
std::pair<edge_iterator, edge_iterator> edges(const ring_graph& g) {
  return std::pair<edge_iterator, edge_iterator>(
    ring_edge_iterator(g, iterator_start()),
    ring_edge_iterator(g, iterator_end()) );
}

edges_size_type num_edges(const ring_graph& g) {
  // There are as many edges as there are vertices, except for size 2
  // graphs, which have a single edge connecting the two vertices.
  return g.n() == 2 ? 1:g.n();
}


// AdjacencyMatrix valid expressions
std::pair<edge_descriptor, bool>
edge(vertex_descriptor u, vertex_descriptor v, const ring_graph& g) {
  if ((u == v + 1 || v == u + 1) &&
      u > 0 && u < num_vertices(g) && v > 0 && v < num_vertices(g))
    return std::pair<edge_descriptor, bool>(edge_descriptor(u, v), true);
  else
    return std::pair<edge_descriptor, bool>(edge_descriptor(), false);
}


/*
Map from edges to weight values
*/
struct edge_weight_map {
  using value_type = double;
  using reference = value_type;
  using key_type = edge_descriptor;
  using category = boost::readable_property_map_tag;

  // Edges have a weight equal to the average of their endpoint indexes.
  reference operator[](key_type e) const {
    return (e.first + e.second)/2.0;
  }
};

// Use these propety_map and property_traits parameterizations to refer to
// the associated property map types.
using const_edge_weight_map = boost::property_map<ring_graph,
                            boost::edge_weight_t>::const_type;
using edge_weight_map_value_type = boost::property_traits<const_edge_weight_map>::reference;
using edge_weight_map_key = boost::property_traits<const_edge_weight_map>::key_type;

// PropertyMap valid expressions
edge_weight_map_value_type
get(const_edge_weight_map pmap, edge_weight_map_key e) {
  return pmap[e];
}


// ReadablePropertyGraph valid expressions
const_edge_weight_map get(boost::edge_weight_t, const ring_graph&) {
  return const_edge_weight_map();
}

edge_weight_map_value_type get(boost::edge_weight_t tag,
                               const ring_graph& g,
                               edge_weight_map_key e) {
  return get(tag, g)[e];
}


// This expression is not part of a graph concept, but is used to return the
// default vertex index map used by the Dijkstra search algorithm.
boost::identity_property_map get(boost::vertex_index_t, const ring_graph&) {
  // The vertex descriptors are already unsigned integer indices, so just
  // return an identity map.
  return boost::identity_property_map();
}

// Print edges as (x, y)
std::ostream& operator<<(std::ostream& output, const edge_descriptor& e) {
  return output << "(" << e.first << ", " << e.second << ")";
}

int main (int argc, char const *argv[]) {
  using namespace boost;
  // Check the concepts that graph models.  This is included to demonstrate
  // how concept checking works, but is not required for a working program
  // since Boost algorithms do their own concept checking.
  BOOST_CONCEPT_ASSERT(( BidirectionalGraphConcept<ring_graph> ));
  BOOST_CONCEPT_ASSERT(( AdjacencyGraphConcept<ring_graph> ));
  BOOST_CONCEPT_ASSERT(( VertexListGraphConcept<ring_graph> ));
  BOOST_CONCEPT_ASSERT(( EdgeListGraphConcept<ring_graph> ));
  BOOST_CONCEPT_ASSERT(( AdjacencyMatrixConcept<ring_graph> ));
  BOOST_CONCEPT_ASSERT((
    ReadablePropertyMapConcept<const_edge_weight_map, edge_descriptor> ));
  BOOST_CONCEPT_ASSERT((
    ReadablePropertyGraphConcept<ring_graph, edge_descriptor, edge_weight_t> ));

  // Specify the size of the graph on the command line, or use a default size
  // of 5.
  std::size_t n = argc == 2 ? std::stoul(argv[1]) : 5;

  // Create a small ring graph.
  ring_graph g(n);

  // Print the outgoing edges of all the vertices.  For n=5 this will print:
  //
  // Vertices, outgoing edges, and adjacent vertices
  // Vertex 0: (0, 1)  (0, 4)   Adjacent vertices 1 4
  // Vertex 1: (1, 2)  (1, 0)   Adjacent vertices 2 0
  // Vertex 2: (2, 3)  (2, 1)   Adjacent vertices 3 1
  // Vertex 3: (3, 4)  (3, 2)   Adjacent vertices 4 2
  // Vertex 4: (4, 0)  (4, 3)   Adjacent vertices 0 3
  // 5 vertices
  std::cout << "Vertices, outgoing edges, and adjacent vertices" << std::endl;
  for (const auto& u : make_range_pair(vertices(g))) {
    std::cout << "Vertex " << u << ": ";
    // Adjacenct edges
    for (const auto& edge : make_range_pair(out_edges(u, g)))
      std::cout << edge << "  ";
    std::cout << " Adjacent vertices ";
    // Adjacent vertices
    // Here we want our adjacency_iterator and not boost::adjacency_iterator.
    for (const auto& vertex : make_range_pair(adjacent_vertices(u, g))) {
      std::cout << vertex << " ";
    }
    std::cout << std::endl;
  }
  std::cout << num_vertices(g) << " vertices" << std::endl << std::endl;

  // Print all the edges in the graph along with their weights.  For n=5 this
  // will print:
  //
  // Edges and weights
  // (0, 1) weight 0.5
  // (1, 2) weight 1.5
  // (2, 3) weight 2.5
  // (3, 4) weight 3.5
  // (4, 0) weight 2
  // 5 edges
  std::cout << "Edges and weights" << std::endl;
  for (const auto& e : make_range_pair(edges(g))) {
    std::cout << e << " weight " << get(edge_weight, g, e) << std::endl;
  }
  std::cout << num_edges(g) << " edges"  << std::endl;

  if (n>0) {
    std::cout << std::endl;
    // Do a Dijkstra search from vertex 0.  For n=5 this will print:
    //
    // Dijkstra search from vertex 0
    // Vertex 0: parent 0, distance 0
    // Vertex 1: parent 0, distance 0.5
    // Vertex 2: parent 1, distance 2
    // Vertex 3: parent 2, distance 4.5
    // Vertex 4: parent 0, distance 2
    vertex_descriptor source = 0;
    std::vector<vertex_descriptor> pred(num_vertices(g));
    std::vector<edge_weight_map_value_type> dist(num_vertices(g));
    iterator_property_map<std::vector<vertex_descriptor>::iterator,
                          property_map<ring_graph, vertex_index_t>::const_type>
      pred_pm(pred.begin(), get(vertex_index, g));
    iterator_property_map<std::vector<edge_weight_map_value_type>::iterator,
                          property_map<ring_graph, vertex_index_t>::const_type>
      dist_pm(dist.begin(), get(vertex_index, g));

    dijkstra_shortest_paths(g, source,
                            predecessor_map(pred_pm).
                            distance_map(dist_pm) );

    std::cout << "Dijkstra search from vertex " << source << std::endl;
    for (const auto& vertex : make_range_pair(vertices(g))) {
      std::cout << "Vertex " << vertex << ": "
                << "parent "<< pred[vertex] << ", "
                << "distance " << dist[vertex]
                << std::endl;
    }
  }

  return 0;
}
