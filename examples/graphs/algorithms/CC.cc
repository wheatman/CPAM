// Usage:
// numactl -i all ./CC -s -m -rounds 3 twitter_SJ
// flags:
//   required:
//     -src: the source to compute the BFS from
//   optional:
//     -rounds : the number of times to run the algorithm
//     -c : indicate that the graph is compressed
//     -m : indicate that the graph should be mmap'd
//     -s : indicate that the graph is symmetric

#include "CC.h"
#include "aspen/aspen.h"

#include <cpam/parse_command_line.h>

namespace aspen {

template <class Graph> double CC_runner(Graph &G, cpam::commandLine P) {
  bool flatsnap = P.getOptionValue("-flatsnap");
  std::cout << "### Application: CC" << std::endl;
  std::cout << "### Graph: " << P.getArgument(0) << std::endl;
  std::cout << "### Threads: " << parlay::num_workers() << std::endl;
  std::cout << "### n: " << G.num_vertices() << std::endl;
  std::cout << "### m: " << G.num_edges() << std::endl;
  std::cout << "### Params: flatsnap = " << flatsnap << std::endl;
  std::cout << "### ------------------------------------" << std::endl;
  std::cout << "### ------------------------------------" << std::endl;

  timer t;
  t.start();
  auto values5 = CC(G, flatsnap);
  double tt = t.stop();

  std::cout << "### Running Time: " << tt << std::endl;

  std::unordered_map<uint32_t, uint32_t> components;
  for (uint32_t i = 0; i < G.num_vertices(); i++) {
    components[values5[i]] += 1;
  }
  printf("there are %zu components\n", components.size());
  uint32_t curent_max = 0;
  uint32_t curent_max_key = 0;
  for (auto p : components) {
    if (p.second > curent_max) {
      curent_max = p.second;
      curent_max_key = p.first;
    }
  }
  printf("the element with the biggest component is %u, it has %u members "
         "to its component\n",
         curent_max_key, curent_max);

  return tt;
}

} // namespace aspen

generate_symmetric_aspen_main(aspen::CC_runner, false);
