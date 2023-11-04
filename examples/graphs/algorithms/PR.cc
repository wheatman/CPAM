// Usage:
// numactl -i all ./PR -s -m -rounds 3 twitter_SJ
// flags:
//   required:
//     -src: the source to compute the BFS from
//   optional:
//     -rounds : the number of times to run the algorithm
//     -c : indicate that the graph is compressed
//     -m : indicate that the graph should be mmap'd
//     -s : indicate that the graph is symmetric

#include "PR.h"
#include "aspen/aspen.h"

#include <cpam/parse_command_line.h>

namespace aspen {

template <class Graph> double PR_runner(Graph &G, cpam::commandLine P) {
  bool flatsnap = true;
  std::cout << "### Application: PR" << std::endl;
  std::cout << "### Graph: " << P.getArgument(0) << std::endl;
  std::cout << "### Threads: " << parlay::num_workers() << std::endl;
  std::cout << "### n: " << G.num_vertices() << std::endl;
  std::cout << "### m: " << G.num_edges() << std::endl;
  std::cout << "### Params: flatsnap = " << flatsnap << std::endl;
  std::cout << "### ------------------------------------" << std::endl;
  std::cout << "### ------------------------------------" << std::endl;

  timer t;
  t.start();
  auto values5 = PR(G, flatsnap);
  double tt = t.stop();

  std::cout << "### Running Time: " << tt << std::endl;

  return tt;
}

} // namespace aspen

generate_symmetric_aspen_main(aspen::PR_runner, false);
