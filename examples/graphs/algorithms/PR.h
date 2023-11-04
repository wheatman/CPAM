#pragma once

#include "aspen/aspen.h"
#include "math.h"

namespace aspen {

#pragma once

struct PR_Vertex {
  parlay::sequence<double> &p_curr;
  parlay::sequence<double> &p_next;
  parlay::sequence<uintE> &degrees;
  PR_Vertex(parlay::sequence<double> &_p_curr,
            parlay::sequence<double> &_p_next,
            parlay::sequence<uintE> &_degrees)
      : p_curr(_p_curr), p_next(_p_next), degrees(_degrees) {}
  inline bool operator()(uintE i) {
    p_next[i] = p_curr[i] / degrees[i]; // damping*p_next[i] + addedConstant;
    return 1;
  }
};

// resets p
struct PR_Vertex_Reset {
  parlay::sequence<double> &p_curr;
  PR_Vertex_Reset(parlay::sequence<double> &_p_curr) : p_curr(_p_curr) {}
  inline bool operator()(uintE i) {
    p_curr[i] = 0.0;
    return 1;
  }
};

template <class W, class Vtxs> struct PR_F_S {
  parlay::sequence<double> &p_curr, &p_next;

  Vtxs &vtxs;
  PR_F_S(parlay::sequence<double> &_p_curr, parlay::sequence<double> &_p_next,
         Vtxs &_vtxs)
      : p_curr(_p_curr), p_next(_p_next), vtxs(_vtxs) {}
  inline bool update(uintE s, uintE d,
                     W w) { // update function applies PageRank equation
    p_next[d] += p_curr[s];

    return 1;
  }
  inline bool updateAtomic(uintE s, uintE d, W w) { // atomic Update
    cpam::utils::fetch_and_add(&p_next[d], p_curr[s]);

    return 1;
  }
  inline bool cond(uintE d) { return 1; }
};

template <class Graph>
inline parlay::sequence<double> PR(Graph &G, bool flatsnap = false) {
  using W = typename Graph::weight_type;
  size_t n = G.num_vertices();
  using edge_node = typename Graph::edge_node;

  parlay::sequence<edge_node *> fs;
  if (flatsnap) {
    timer st;
    st.start();
    fs = G.fetch_all_vertices();
    st.next("Snapshot time");
  }

  auto degrees = parlay::sequence<uintE>::uninitialized(n);

  parlay::parallel_for(
      0, n,
      [&](size_t i) {
        auto vtx = G.get_vertex(i);
        degrees[i] = vtx.out_degree();
      },
      1);

  auto p_curr = parlay::sequence<double>(n, 1.0 / (double)n);
  auto p_next = parlay::sequence<double>(n, 0);

  auto active = parlay::sequence<bool>(n, true);
  vertexSubset Active(n, std::move(active));
  for (int i = 0; i < 10; i++) {

    vertexMap(Active, PR_Vertex(p_curr, p_next, degrees));
    vertexMap(Active, PR_Vertex_Reset(p_curr));
    swap(p_curr, p_next);
    timer emt;
    emt.start();
    if (!flatsnap) {
      G.edgeMap(Active,
                PR_F_S<W, parlay::sequence<edge_node *>>(p_curr, p_next, fs),
                -1, sparse_blocked | dense_parallel);
    } else {
      G.edgeMap(Active,
                PR_F_S<W, parlay::sequence<edge_node *>>(p_curr, p_next, fs),
                fs, -1, sparse_blocked | dense_parallel);
    }
    emt.stop();
    emt.reportTotal("edge map time");
    vertexMap(Active, PR_Vertex_Reset(p_curr));
    swap(p_curr, p_next);
  }

  return p_curr;
}

} // namespace aspen
