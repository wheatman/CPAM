
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstring>
#include <fstream>
#include <iostream>
#include <limits>
#include <optional>
#include <string_view>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include <parlay/primitives.h>
#include <parlay/random.h>

#include "aspen/aspen.h"
#include <cpam/get_time.h>
#include <cpam/parse_command_line.h>

#include "../aspen/byte-pd-amortized.h"
#include "../aspen/graph_io.h"
#include "../aspen/immutable_graph.h"
#include "../aspen/traversable_graph.h"
#include "../aspen/utils.h"
#include "parlay/internal/group_by.h"
#include "parlay/parallel.h"
#include "parlay/sequence.h"

class bin_file_map {
private:
  uint32_t *begin_p;
  uint32_t *end_p;

public:
  using value_type = uint32_t;
  using reference = uint32_t &;
  using const_reference = const uint32_t &;
  using iterator = uint32_t *;
  using const_iterator = const uint32_t *;
  using pointer = uint32_t *;
  using const_pointer = const uint32_t *;
  using difference_type = std::ptrdiff_t;
  using size_type = size_t;

  uint32_t operator[](size_t i) const { return begin_p[i]; }
  const uint32_t *begin() const { return begin_p; }
  const uint32_t *end() const { return end_p; }

  size_t size() const { return end() - begin(); }

  explicit bin_file_map(const std::string &filename) {

    struct stat sb;
    int fd = open(filename.c_str(), O_RDONLY);
    assert(fd != -1);
    [[maybe_unused]] auto fstat_res = fstat(fd, &sb);
    assert(fstat_res != -1);
    assert(S_ISREG(sb.st_mode));

    uint32_t *p = static_cast<uint32_t *>(
        mmap(0, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0));
    assert(p != MAP_FAILED);
    [[maybe_unused]] int close_p = ::close(fd);
    assert(close_p != -1);

    begin_p = p;
    end_p = p + (sb.st_size / sizeof(uint32_t));
  }

  ~bin_file_map() { close(); }

  // moveable but not copyable
  bin_file_map(const bin_file_map &) = delete;
  bin_file_map &operator=(const bin_file_map &) = delete;

  bin_file_map(bin_file_map &&other) noexcept
      : begin_p(other.begin_p), end_p(other.end_p) {
    other.begin_p = nullptr;
    other.end_p = nullptr;
  }

  bin_file_map &operator=(bin_file_map &&other) noexcept {
    close();
    swap(other);
    return *this;
  }

  void close() {
    if (begin_p != nullptr) {
      munmap(begin_p, end_p - begin_p);
    }
    begin_p = nullptr;
    end_p = nullptr;
  }

  void swap(bin_file_map &other) {
    std::swap(other.begin_p, begin_p);
    std::swap(other.end_p, end_p);
  }

  bool empty() const { return begin_p == nullptr; }
};

inline auto get_edges_from_file_bin_ts_with_remove(
    const std::string &filename, uint64_t *edge_count, uint32_t *node_count,
    bool symmetrize = true, long max_edges = std::numeric_limits<long>::max()) {
  using edge_t = std::tuple<uint32_t, uint32_t, uint32_t, bool>;

  auto file = bin_file_map(filename);
  long num_edges = file.size() / 4;
  if (num_edges > max_edges) {
    num_edges = max_edges;
  }
  std::cout << "num rows in file is " << num_edges << "\n";
  parlay::sequence<edge_t> edges = parlay::tabulate(num_edges, [&](size_t i) {
    bool insert = file[4 * i] == 1;
    uint32_t src = file[4 * i + 1];
    uint32_t dest = file[4 * i + 2];
    uint32_t ts = file[4 * i + 3];
    if (dest < src) {
      std::swap(src, dest);
    }
    return edge_t(src, dest, ts, insert);
  });
  file.close();

  edge_t *max_element =
      parlay::max_element(edges, [](const edge_t &e1, const edge_t &e2) {
        return std::get<2>(e1) < std::get<2>(e2);
      });
  size_t max_node = std::get<2>(*max_element);

  std::cout << "finished reading in the file\n";
  if (symmetrize) {
    parlay::sort_inplace(edges);
    auto unique = parlay::unique(edges);
    edges = std::move(unique);
    auto sym_edges = parlay::tabulate(edges.size() * 2, [&](size_t i) {
      auto edge = edges[i / 2];
      if (i % 2) {
        return edge;
      } else {
        return std::make_tuple(std::get<1>(edge), std::get<0>(edge),
                               std::get<2>(edge), std::get<3>(edge));
      }
    });
    std::cout << "symerterized the edges\n";
    edges = std::move(sym_edges);
  }

  parlay::sort_inplace(edges, [](auto const &t1, auto const &t2) {
    return std::get<2>(t1) < std::get<2>(t2);
  });
  *edge_count = edges.size();
  *node_count = max_node + 1;
  return edges;
}

auto filter_edges(
    parlay::sequence<std::pair<std::tuple<uint32_t, uint32_t>,
                               std::tuple<uint32_t, bool>>> &edges) {
  auto groups = parlay::group_by_key_ordered(edges);

  auto updates = parlay::map_maybe(
      groups,
      [](std::pair<std::tuple<uint32_t, uint32_t>,
                   parlay::sequence<std::tuple<uint32_t, bool>>> &group)
          -> std::optional<std::tuple<uint32_t, uint32_t, bool>> {
        auto [src, dest] = group.first;
        if (group.second.size() == 1) {
          auto [ts, insert] = group.second[0];
          return std::make_tuple(src, dest, insert);
        }
        auto sorted_ops = parlay::sort(group.second);
        bool first_op = std::get<1>(sorted_ops[0]);
        bool last_op = std::get<1>(sorted_ops.back());
        if (first_op) {
          // the first operations is an insert
          if (last_op) {
            // the last operation is an insert
            return std::make_tuple(src, dest, true);
          } else {
            // the last operation is a delete, and the first was an insert, so
            // this is a null opt
            return std::nullopt;
          }
        } else {
          // first operation was a delete
          if (last_op) {
            // last was an insert, so do nothing
            return std::nullopt;
          } else {
            return std::make_tuple(src, dest, false);
          }
        }
      });
  auto inserts =
      parlay::map_maybe(updates,
                        [](std::tuple<uint32_t, uint32_t, bool> &edge)
                            -> std::optional<std::tuple<uint32_t, uint32_t>> {
                          auto [s, d, i] = edge;
                          if (i) {
                            return std::make_tuple(s, d);
                          } else {
                            return std::nullopt;
                          }
                        });
  auto deletes =
      parlay::map_maybe(updates,
                        [](std::tuple<uint32_t, uint32_t, bool> &edge)
                            -> std::optional<std::tuple<uint32_t, uint32_t>> {
                          auto [s, d, i] = edge;
                          if (!i) {
                            return std::make_tuple(s, d);
                          } else {
                            return std::nullopt;
                          }
                        });
  return std::make_tuple(inserts, deletes, std::get<0>(edges[0].second));
}

auto group_edges_by_timestamp(
    const parlay::sequence<std::tuple<uint32_t, uint32_t, uint32_t, bool>>
        &edges,
    size_t time_steps_to_group) {
  parlay::sequence<parlay::sequence<
      std::pair<std::tuple<uint32_t, uint32_t>, std::tuple<uint32_t, bool>>>>
      sequences(1);
  size_t current_timestamp = 0;
  if (!edges.empty()) {
    auto [src, dest, ts, insert] = edges[0];
    current_timestamp = ts - (ts % time_steps_to_group);
  }
  for (const auto &edge : edges) {
    auto [src, dest, ts, insert] = edge;
    if (ts >= current_timestamp + time_steps_to_group) {
      sequences.emplace_back();
      current_timestamp = ts - (ts % time_steps_to_group);
    }
    sequences.back().emplace_back(std::make_tuple(src, dest),
                                  std::make_tuple(ts, insert));
  }
  return parlay::map(sequences, filter_edges);
}

inline auto
build_base_graph(uint32_t num_nodes, uint64_t num_edges,
                 parlay::sequence<std::tuple<uint32_t, uint32_t>> &edges) {
  using W = aspen::empty;
  using inner_graph = aspen::symmetric_graph<W>;
  using outer_graph = aspen::traversable_graph<inner_graph>;
  timer build_t;
  build_t.start();
  inner_graph ig;
  ig.reserve(num_nodes, num_edges);
  ig = ig.insert_edges_batch_2(edges.size(), edges.data());
  auto G = outer_graph(std::move(ig));
  build_t.stop();
  build_t.reportTotal("Aspen: build time");
  std::cout << G.num_vertices() << "\n";
  std::cout << G.num_edges() << "\n";
  return G;
}

template <class T> class Reducer_Vector {
  static constexpr std::size_t hardware_constructive_interference_size = 64;
  static constexpr std::size_t hardware_destructive_interference_size = 64;

  struct aligned_f {
    alignas(hardware_destructive_interference_size) std::vector<T> f;
  };
  std::vector<aligned_f> data;

  [[maybe_unused]] static int getWorkers() { return parlay::num_workers(); }

  [[maybe_unused]] static int getWorkerNum() { return parlay::worker_id(); }

public:
  Reducer_Vector() { data.resize(getWorkers()); }

  template <typename F> void push_back(F arg) {
    int worker_num = getWorkerNum();
    data[worker_num].f.emplace_back(arg);
  }
  void push_back(T arg) {
    int worker_num = getWorkerNum();
    data[worker_num].f.push_back(arg);
  }

  std::vector<T> get() const {
    if (data.size() == 0) {
      return {};
    }
    std::vector<size_t> lengths(data.size() + 1);
    for (size_t i = 1; i <= data.size(); i++) {
      lengths[i] += lengths[i - 1] + data[i - 1].f.size();
    }
    if (lengths[data.size()] == 0) {
      return {};
    }
    std::vector<T> output(lengths[data.size()]);
    if (output.size() > 0) {
      parlay::parallel_for(0, data.size(), [&](size_t i) {
        if (data[i].f.size() > 0) {
          std::copy(data[i].f.begin(), data[i].f.end(),
                    output.begin() + lengths[i]);
        }
      });
    }
    return output;
  }
};

size_t get_size_of_groups(
    const parlay::sequence<
        std::tuple<parlay::sequence<std::tuple<uint32_t, uint32_t>>,
                   parlay::sequence<std::tuple<uint32_t, uint32_t>>, uint32_t>>
        &groups) {
  return parlay::reduce(parlay::delayed_map(groups, [](auto &elem) {
    auto &[inserts, deletes, start_time] = elem;
    size_t insert_size =
        16 + (inserts.capacity() * sizeof(std::tuple<uint32_t, uint32_t>));
    size_t delete_size =
        16 + (deletes.capacity() * sizeof(std::tuple<uint32_t, uint32_t>));
    return insert_size + delete_size;
  }));
}

size_t get_count_of_groups(
    const parlay::sequence<
        std::tuple<parlay::sequence<std::tuple<uint32_t, uint32_t>>,
                   parlay::sequence<std::tuple<uint32_t, uint32_t>>, uint32_t>>
        &groups) {
  return parlay::reduce(parlay::delayed_map(groups, [](auto &elem) {
    auto &[inserts, deletes, start_time] = elem;
    return inserts.size() + deletes.size();
  }));
}

template <typename Graph> auto get_edges(Graph &g) {
  Reducer_Vector<std::tuple<uint32_t, uint32_t>> edges_reducer;

  g.map_vertices([&edges_reducer](auto &vertex) {
    auto neighbors = vertex.out_neighbors();
    auto map_f = [&edges_reducer](uint32_t src, uint32_t dest, auto weight) {
      edges_reducer.push_back({src, dest});
    };
    neighbors.map(map_f);
  });

  return parlay::sort(edges_reducer.get());
}

class simple_graph {
  parlay::sequence<std::unordered_set<uint32_t>> nodes;

public:
  simple_graph(uint32_t num_nodes) : nodes(num_nodes) {}
  void add_edge(uint32_t src, uint32_t dest) { nodes[src].insert(dest); }
  void remove_edge(uint32_t src, uint32_t dest) { nodes[src].erase(dest); }
  template <class T> void insert_edge_batch(const T &edges) {
    for (const auto &edge : edges) {
      add_edge(std::get<0>(edge), std::get<1>(edge));
    }
  }
  template <class T> void remove_edge_batch(const T &edges) {
    for (const auto &edge : edges) {
      remove_edge(std::get<0>(edge), std::get<1>(edge));
    }
  }
  auto get_edges() {
    Reducer_Vector<std::tuple<uint32_t, uint32_t>> edges_reducer;
    parlay::parallel_for(0, nodes.size(), [&](size_t i) {
      for (auto &dest : nodes[i]) {
        edges_reducer.push_back({i, dest});
      }
    });
    return parlay::sort(edges_reducer.get());
  }
};

uint32_t round_ts(uint32_t ts, uint32_t group_size) {
  return ts - (ts % group_size) + group_size;
}

template <class T> size_t count_unique_edges(const T &edges) {
  return parlay::unique(parlay::sort(parlay::delayed_map(
                            edges,
                            [](auto &edge) {
                              return std::make_tuple(std::get<0>(edge),
                                                     std::get<1>(edge));
                            })))
      .size();
}

void test_building_in_groups(long max_edges, long group_size, std::string fname,
                             bool verify) {
  uint64_t edge_count;
  uint32_t node_count;
  auto edges = get_edges_from_file_bin_ts_with_remove(
      fname, &edge_count, &node_count, true, max_edges);

  size_t unique_edges = count_unique_edges(edges);

  std::cout << unique_edges << " unique edges\n";
  auto groups = group_edges_by_timestamp(edges, group_size);
  size_t size_of_groups = get_size_of_groups(groups);
  std::cout << "size of groups is " << size_of_groups << " count of groups is "
            << get_count_of_groups(groups) << "\n";
  edges.clear();

  auto G =
      build_base_graph(node_count, edge_count / 10, std::get<0>(groups[0]));

  size_t total_updates = std::get<0>(groups[0]).size();
  if (std::get<1>(groups[0]).size()) {
    G.delete_edges_batch_inplace(std::get<1>(groups[0]).size(),
                                 std::get<1>(groups[0]).data());
    total_updates += std::get<1>(groups[0]).size();
  }
  {
    auto [used, unused] = parlay::internal::memory_usage();
    std::cout << group_size << ", "
              << round_ts(std::get<2>(groups[0]), group_size) << ", "
              << total_updates << ", " << G.num_edges() << ", "
              << used - get_size_of_groups(groups) << "\n";
  }
  std::vector<aspen::traversable_graph<aspen::symmetric_graph<aspen::empty>>>
      graph_stack;
  graph_stack.push_back(G);

  size_t print_frequency = 1;
  if (groups.size() > 500) {
    print_frequency = groups.size() / 500;
  }
  timer insert_timer;
  insert_timer.start();
  for (size_t gi = 1; gi < groups.size(); gi += 1) {
    total_updates +=
        std::get<0>(groups[gi]).size() + std::get<1>(groups[gi]).size();

    auto g2 = graph_stack.back().insert_edges_batch(
        std::get<0>(groups[gi]).size(), std::get<0>(groups[gi]).data());
    g2.delete_edges_batch_inplace(std::get<1>(groups[gi]).size(),
                                  std::get<1>(groups[gi]).data());
    graph_stack.push_back(g2);
    if (gi % print_frequency == 0 || gi == groups.size() - 1) {
      auto [used, unused] = parlay::internal::memory_usage();
      std::cout << group_size << ", "
                << round_ts(std::get<2>(groups[gi]), group_size) << ", "
                << total_updates << ", " << g2.num_edges() << ", "
                << used - get_size_of_groups(groups) << ", "
                << total_updates / insert_timer.get_total() << "\n";
    }
  }

  std::cout << "sanity check, printing out the tree stats for the last tree\n";
  graph_stack.back().get_tree_sizes("", "");

  if (verify) {
    simple_graph verify_graph(node_count);
    for (size_t gi = 0; gi < groups.size(); gi += 1) {
      verify_graph.insert_edge_batch(std::get<0>(groups[gi]));
      verify_graph.remove_edge_batch(std::get<1>(groups[gi]));
      if (verify_graph.get_edges() != get_edges(graph_stack[gi])) {
        std::cerr << "edges do not match after group " << gi << "\n";
        return;
      }
      if (gi % print_frequency == 0) {
        std::cout << "verified " << gi << " graphs\n";
      }
    }
  }
}

void run_test(cpam::commandLine P) {
  std::string test_to_run = P.getOptionValue("-t", "nothing");
  //   size_t rounds = P.getOptionLongValue("-rounds", 3);
  if (test_to_run == "nothing") {
    return;
  }
  if (test_to_run == "groups") {
    uint64_t max_edges =
        P.getOptionLongValue("-m", std::numeric_limits<long>::max());
    uint64_t group_size =
        P.getOptionLongValue("-g", std::numeric_limits<long>::max());
    bool verify = P.getOption("-v");
    char *iFile = P.getArgument(0);
    std::string fname = std::string(iFile);
    test_building_in_groups(max_edges, group_size, fname, verify);
  }
}

int main(int argc, char *argv[]) {
  std::cout << "In main" << std::endl;
  cpam::commandLine P(argc, argv, " <inFile>");

  run_test(P);

  //   for (auto &group : groups) {
  //     std::cout << "### Group ###\n";
  //     for (auto &edge : group) {
  //       std::cout << std::get<0>(edge) << ", " << std::get<1>(edge) << ", "
  //                 << std::get<2>(edge) << "\n";
  //     }
  //   }
  //   timer rt;
  //   rt.start();

  //   auto G = aspen::parse_unweighted_symmetric_graph(iFile, mmap);
  //   rt.next("Graph read time");
  //   auto AG = aspen::symmetric_graph_from_static_graph(G);
  //   run_test(P, AG, rounds);
}
