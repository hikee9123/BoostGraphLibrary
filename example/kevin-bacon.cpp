//=======================================================================
// Copyright 2001 Jeremy G. Siek, Andrew Lumsdaine, Lie-Quan Lee, 
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================
#include <boost/config.hpp>
#include <iostream>
#include <fstream>
#include <string>
#include <boost/tokenizer.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/visitors.hpp>
#include <boost/graph/breadth_first_search.hpp>
#include "range_pair.hpp"
#include <map>

using namespace boost;

template <typename DistanceMap>
class bacon_number_recorder : public default_bfs_visitor
{
public:
  bacon_number_recorder(DistanceMap dist) : d(dist) { }

  template <typename Edge, typename Graph>
  void tree_edge(Edge e, const Graph& g) const
  {
    typename graph_traits<Graph>::vertex_descriptor
      u = source(e, g), v = target(e, g);
      d[v] = d[u] + 1;
  }
private:
    DistanceMap d;
};

// Convenience function
template <typename DistanceMap>
bacon_number_recorder<DistanceMap>
record_bacon_number(DistanceMap d)
{
  return bacon_number_recorder<DistanceMap> (d);
}


int
main()
{
  std::ifstream datafile("./kevin-bacon.dat");
  if (!datafile) {
    std::cerr << "No ./kevin-bacon.dat file" << std::endl;
    return EXIT_FAILURE;
  }

  using Graph = adjacency_list <vecS, vecS, undirectedS,
    property<vertex_name_t, std::string>,
    property<edge_name_t, std::string>>;
  Graph g;

  auto actor_name = get(vertex_name, g);
  auto connecting_movie = get(edge_name, g);

  using Vertex = graph_traits<Graph>::vertex_descriptor;
  using NameVertexMap = std::map<std::string, Vertex>;
  NameVertexMap actors;

  for (std::string line; std::getline(datafile, line);) {
    char_delimiters_separator<char> sep(false, "", ";");
    tokenizer <> line_toks(line, sep);
    tokenizer <>::iterator i = line_toks.begin();
    std::string actors_name = *i++;
    Vertex u, v;
    auto [pos, inserted] = actors.insert(std::make_pair(actors_name, Vertex()));
    if (inserted) {
      u = add_vertex(g);
      actor_name[u] = actors_name;
      pos->second = u;
    } else
      u = pos->second;

    std::string movie_name = *i++;

    std::tie(pos, inserted) = actors.insert(std::make_pair(*i, Vertex()));
    if (inserted) {
      v = add_vertex(g);
      actor_name[v] = *i;
      pos->second = v;
    } else
      v = pos->second;

    auto [e, insertede] = add_edge(u, v, g);
    if (insertede)
      connecting_movie[e] = movie_name;

  }

  std::vector<int>bacon_number(num_vertices(g));

  Vertex src = actors["Kevin Bacon"];
  bacon_number[src] = 0;

  breadth_first_search(g, src,
                       visitor(record_bacon_number(&bacon_number[0])));

  for (const auto& vertex : make_range_pair(vertices(g))) {
    std::cout << actor_name[vertex] << " has a Bacon number of "
      << bacon_number[vertex] << std::endl;
  }

  return 0;
}
