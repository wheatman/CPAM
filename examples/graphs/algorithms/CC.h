#pragma once

#include "aspen/aspen.h"
#include "math.h"

namespace aspen {

#pragma once

template <class ET> inline bool CAS(ET *ptr, ET oldv, ET newv) {
  if (sizeof(ET) == 1) {
    return __sync_bool_compare_and_swap((bool *)ptr, *((bool *)&oldv),
                                        *((bool *)&newv));
  } else if (sizeof(ET) == 4) {
    return __sync_bool_compare_and_swap((int *)ptr, *((int *)&oldv),
                                        *((int *)&newv));
  } else if (sizeof(ET) == 8) {
    return __sync_bool_compare_and_swap((long *)ptr, *((long *)&oldv),
                                        *((long *)&newv));
  } else {
    std::cout << "CAS bad length : " << sizeof(ET) << std::endl;
    abort();
  }
}

template <class ET> inline bool writeMin(ET *a, ET b) {
  ET c;
  bool r = 0;
  do
    c = *a;
  while (c > b && !(r = CAS(a, c, b)));
  return r;
}

// function used by vertex map to sync prevIDs with IDs
struct CC_Vertex_F {
  parlay::sequence<uintE> &IDs, &prevIDs;
  CC_Vertex_F(parlay::sequence<uintE> &_IDs, parlay::sequence<uintE> &_prevIDs)
      : IDs(_IDs), prevIDs(_prevIDs) {}
  inline bool operator()(uintE i) {
    prevIDs[i] = IDs[i];
    return 1;
  }
};

template <class W> struct CC_F {
  parlay::sequence<uintE> &IDs, &prevIDs;
  CC_F(parlay::sequence<uintE> &_IDs, parlay::sequence<uintE> &_prevIDs)
      : IDs(_IDs), prevIDs(_prevIDs) {}
  inline bool update(uintE s, uintE d,
                     const W &w) { // Update function writes min ID
    uintE origID = IDs[d];
    if (IDs[s] < origID) {
      IDs[d] = min(origID, IDs[s]);
      if (origID == prevIDs[d])
        return 1;
    }
    return 0;
  }
  inline bool updateAtomic(uintE s, uintE d, const W &w) { // atomic Update
    uintE origID = IDs[d];
    return (writeMin(&IDs[d], IDs[s]) && origID == prevIDs[d]);
  }
  inline bool cond(uintE d) { return true; } // does nothing
};

template <class Graph>
inline parlay::sequence<uintE> CC(Graph &G, bool flatsnap = false) {
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

  auto IDs = parlay::sequence<uintE>(n, UINT_E_MAX);
  auto prevIDs = parlay::sequence<uintE>(n, UINT_E_MAX);
  parlay::parallel_for(0, n, [&](size_t i) { IDs[i] = i; });

  auto active = parlay::sequence<bool>(n, true);
  vertexSubset Active(n, std::move(active));
  while (!Active.isEmpty()) {
    std::cout << Active.size() << "\n";
    vertexMap(Active, CC_Vertex_F(IDs, prevIDs));
    timer emt;
    emt.start();
    vertexSubset output;
    if (!flatsnap) {
      output = G.edgeMap(Active, CC_F<W>(IDs, prevIDs), -1,
                         sparse_blocked | dense_parallel);
    } else {
      output = G.edgeMap(Active, CC_F<W>(IDs, prevIDs), fs, -1,
                         sparse_blocked | dense_parallel);
    }
    emt.stop();
    emt.reportTotal("edge map time");
    Active = std::move(output);
  }

  return IDs;
}

} // namespace aspen
