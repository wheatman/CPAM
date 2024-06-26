#include <algorithm>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <iostream>
#include <map>
#include <random>
#include <set>
#include <vector>
#include "assert.h"

#include <pam/pam.h>
#include <pam/parse_command_line.h>
#include <cpam/cpam.h>

#include <parlay/primitives.h>
#include <parlay/random.h>
#include <unordered_map>

namespace cpam {

timer t;

using namespace std;

// using key_type = unsigned int;
using key_type = size_t;

bool do_check = false;

struct Add {
  using T = key_type;
  static T add(T a, T b) { return a + b; }
  static T identity() { return 0; }
};

struct Add_Pair {
  using T = std::pair<key_type, key_type>;
  Add_Pair() : identity({0, 0}) {}
  T identity;
  static T f(T a, T b) { return {a.first + b.first, a.second + b.second}; }
};

struct entry {
  using key_t = key_type;
  using val_t = key_type;
  using aug_t = key_type;
  static inline bool comp(key_t a, key_t b) { return a < b; }
  static aug_t get_empty() { return 0; }
  static aug_t from_entry(key_t k, val_t v) { return v;}
  static aug_t combine(aug_t a, aug_t b) { return std::max(a,b);}
  // following just used for treaps
  static size_t hash(pair<key_t, val_t> e) { return parlay::hash64(std::get<0>(e)); }
};

struct entry2 {
  using key_t = key_type;
  using val_t = bool;
  static inline bool comp(key_t a, key_t b) { return a < b; }
};
struct entry3 {
  using key_t = key_type;
  using val_t = char;
  static inline bool comp(key_t a, key_t b) { return a < b; }
};

using par = std::tuple<key_type, key_type>;
using par2 = std::tuple<key_type, bool>;
#ifdef USE_DIFF_ENCODING
#ifdef NO_AUG
using tmap = diff_encoded_map<entry, BLOCK_SIZE>;
#else
using tmap = diff_encoded_aug_map<entry, BLOCK_SIZE>;
#endif
#else
#ifdef NO_AUG
using tmap = pam_map<entry, BLOCK_SIZE>;
#else
using tmap = aug_map<entry, BLOCK_SIZE>;
#endif
#endif

#ifdef USE_DIFF_ENCODING
using tset = pam_set<entry, 256, diffencoded_entry_encoder>;
#else
using tset = pam_set<entry>;
#endif

// using tmap = pam_map<entry>;
// using tmap = aug_map<entry>;
// using tmap = pam_set<entry>;
// using tmap = diff_encoded_map<entry>;

struct mapped {
  key_type k, v;
  mapped(key_type _k, key_type _v) : k(_k), v(_v){};
  mapped(){};

  bool operator<(const mapped& m) const { return k < m.k; }

  bool operator>(const mapped& m) const { return k > m.k; }

  bool operator==(const mapped& m) const { return k == m.k; }
};

size_t str_to_int(char* str) { return strtol(str, NULL, 10); }

void check(bool test, string message) {
  if (!test) {
    cout << "Failed test: " << message << endl;
    exit(1);
  }
}

std::mt19937_64& get_rand_gen() {
  static thread_local std::random_device rd;
  static thread_local std::mt19937_64 generator(rd());
  // TODO: change back to use randomness (det for testing)
  //static thread_local std::mt19937_64 generator(42);
  return generator;
}

parlay::sequence<par> uniform_input(size_t n, size_t window,
                                  bool shuffle = false) {
  auto g = [&](size_t i) {
    uniform_int_distribution<> r_keys(1, window);
    key_type key = r_keys(get_rand_gen());
    key_type val = i;
    return make_pair(key, val);
  };
  auto Pairs = parlay::sequence<par>::from_function(n, g);
  auto addfirst = [](par a, par b) -> par {
    return par(std::get<0>(a) + std::get<0>(b), std::get<1>(b));
  };
  parlay::scan_inclusive_inplace(parlay::make_slice(Pairs), parlay::make_monoid(addfirst, par(0, 0)));
  if (shuffle) parlay::random_shuffle(parlay::make_slice(Pairs));
  return Pairs;
}

parlay::sequence<par> uniform_input_unsorted(size_t n, size_t window) {
  auto f = [&](size_t i) {
    uniform_int_distribution<size_t> r_keys(1, window);
    key_type k = r_keys(get_rand_gen());
    key_type c = r_keys(get_rand_gen());
    return make_pair(k, c);
  };
  auto v = parlay::sequence<par>::from_function(n, f);
  return v;
}

parlay::sequence<key_type> uniform_input_unsorted2(size_t n, size_t window) {
  auto f = [&](size_t i) {
    uniform_int_distribution<size_t> r_keys(1, window);
    key_type k = r_keys(get_rand_gen());
    return k;
  };
  auto v = parlay::sequence<key_type>::from_function(n, f);
  return v;
}

bool check_union(const tmap &m1, const tmap &m2, const tmap &m3) {
  std::cout << "m1.size = " << m1.size() << " m2.size = " << m2.size()
            << std::endl;
  vector<key_type> e1(m1.size());
  vector<key_type> e2(m2.size());
  vector<key_type> e3;
  tmap::keys_to_array(m1, &e1[0]);
  tmap::keys_to_array(m2, &e2[0]);

  set_union(e1.begin(), e1.end(), e2.begin(), e2.end(), back_inserter(e3));

  bool ret = (m3.size() == e3.size());
  for (auto it : e3) {
//    std::cout << "looking up e3 = " << it << std::endl;
    bool cont = m3.contains(it);
//    if (!cont) std::cout << "m3 does not contain " << it << std::endl;
    ret &= cont;
  }
  std::cout << "m3 size = " << m3.size() << " e3 size = " << e3.size() << std::endl;
  std::cout << "ret is: " << ret << std::endl;
  return ret;
}

bool check_multi_insert(const parlay::sequence<par>& m1,
                        const parlay::sequence<par>& m2, const tmap& m3) {
  size_t l1 = m1.size();
  size_t l2 = m2.size();
  vector<key_type> e1(m1.size());
  vector<key_type> e2(m2.size());
  vector<key_type> e3;
  auto f = [&](size_t i) { e1[i] = std::get<0>(m1[i]); };
  parlay::parallel_for(0, l1, f);
  auto f2 = [&](size_t i) { e2[i] = std::get<0>(m2[i]); };
  parlay::parallel_for(0, l2, f2);

  set_union(e1.begin(), e1.end(), e2.begin(), e2.end(), back_inserter(e3));

  bool ret = (m3.size() == e3.size());

  for (auto it : e3) ret &= m3.contains(it);

  return ret;
}

bool check_intersect(const tmap& m1, const tmap& m2, const tmap& m3) {
  vector<key_type> e1(m1.size());
  vector<key_type> e2(m2.size());
  vector<key_type> e3;
  tmap::keys_to_array(m1, &e1[0]);
  tmap::keys_to_array(m2, &e2[0]);

  set_intersection(e1.begin(), e1.end(), e2.begin(), e2.end(),
                   back_inserter(e3));

  bool ret = (m3.size() == e3.size());

  for (auto it : e3) ret &= m3.contains(it);

  return ret;
}

bool check_difference(const tmap& m1, const tmap& m2, const tmap& m3) {
  vector<key_type> e1(m1.size());
  vector<key_type> e2(m2.size());
  vector<key_type> e3;
  tmap::keys_to_array(m1, &e1[0]);
  tmap::keys_to_array(m2, &e2[0]);

  set_difference(e1.begin(), e1.end(), e2.begin(), e2.end(), back_inserter(e3));

  bool ret = (m3.size() == e3.size());

  for (auto it : e3) ret &= m3.contains(it);

  return true;
}

bool check_split(key_type key, const par* v, const pair<tmap, tmap>& m) {
  size_t n = m.first.size() + m.second.size() + 1;

  bool ret = 1;
  for (size_t i = 0; i < n; ++i) {
    if (std::get<0>(v[i]) < key) ret &= m.first.contains(std::get<0>(v[i]));
    if (std::get<0>(v[i]) > key) ret &= m.second.contains(std::get<0>(v[i]));
  }

  return ret;
}

template <class T>
bool check_filter(const tmap& m, const tmap& f, T cond) {
  vector <key_type> e1(m.size()), e2(f.size());
  tmap::keys_to_array(m, &e1[0]);
  tmap::keys_to_array(f, &e2[0]);

  vector<key_type> v;
  for (size_t i = 0; i < e1.size(); ++i)
      if (cond({e1[i], e1[i]})) v.push_back(e1[i]);

  bool ret = (e2.size() == v.size());
  for (size_t i = 0; i < v.size(); ++i)
      ret &= e2[i] == v[i];

  return ret;
  return true;
}

template <class T>
bool check_aug_filter(const tmap& m, const tmap& f, T cond) {
  // vector <par> e1, e2;
  // m.content(back_inserter(e1));
  // f.content(back_inserter(e2));

  // vector<par> v;
  // for (size_t i = 0; i < e1.size(); ++i)
  //     if (cond(e1[i].second)) v.push_back(e1[i]);

  // bool ret = (e2.size() == v.size());
  // for (size_t i = 0; i < v.size(); ++i)
  //     ret &= e2[i] == v[i];

  // return ret;
  return true;
}

bool contains(const tmap& m, const parlay::sequence<par> v) {
  bool ret = 1;
  for (size_t i = 0; i < m.size(); ++i) {
    ret &= m.contains(std::get<0>(v[i]));
  }
  return ret;
}

bool contains_kv(const tmap& m, const parlay::sequence<par>& v) {
  bool ret = 1;
  for (size_t i = 0; i < m.size(); ++i) {
    auto find_opt = m.find(std::get<0>(v[i]));
    ret &= find_opt.has_value();
    if (find_opt.has_value()) {
      ret &= (*(find_opt) == std::get<1>(v[i]));
    }
  }
  return ret;
}

auto union_inner(size_t n, size_t m) {
  parlay::sequence<par> v1 = uniform_input_unsorted(n, 4294967295);
  //parlay::sequence<par> v1 = uniform_input(n, 20);
  tmap m1(v1);
  size_t nx = m1.size();
  // std::cout << "Size of node is: " << sizeof(tmap::node) << std::endl;

  timer mt; mt.start();
  parlay::sequence<par> v2 = uniform_input_unsorted(m, 4294967295);
  //parlay::sequence<par> v2 = uniform_input(m, 20);
  tmap m2(v2);
  size_t mx = m2.size();
  double tm;
  mt.next("M2 build time");

//  vector<key_type> e1(m1.size());
//  tmap::keys_to_array(m1, &e1[0]);
//  for (size_t i=0; i<e1.size(); i++) {
//    std::cout << e1[i] << std::endl;
//  }

  timer t;
  t.start();
  std::cout << "Starting union" << std::endl;
  tmap m3 = tmap::map_union(m1, m2);
  tm = t.stop();

  if (do_check) {
    check(m1.size() == nx, "map size is wrong.");
    check(m2.size() == mx, "map size is wrong.");
    check(check_union(m1, m2, m3), "union is wrong");
  }
  return std::make_tuple(m3, tm);
}

double test_union(size_t n, size_t m) {
  auto [ret, tm] = union_inner(n, m);

  std::cout << "ret size = " << ret.size() << std::endl;
  std::cout << "With built tree, used:" << std::endl;
  // tmap::GC::alloc::print_stats();
  // parlay::default_allocator.print_stats();

#ifndef PARLAY_USE_STD_ALLOC
  parlay::internal::memory_usage();
  std::cout << "Per-bucket details: " << std::endl;
  parlay::internal::get_default_allocator().print_stats();
#endif
  tmap::GC::alloc::print_stats();
  tmap::GC::complex_alloc::print_stats();
  return tm;
}

double test_incremental_union_nonpersistent(size_t n, size_t m) {
  parlay::sequence<par> v1 = uniform_input(n, 20, true);
  tmap m1(v1);

  parlay::sequence<par> v2 = uniform_input(n, 20, true);

  // size_t round = n/m;
  timer t;
  t.start();
  for (size_t i = 0; i < n; i += m) {
    tmap m2(parlay::to_sequence(v2.cut(i, i + m)));
    m1 = tmap::map_union(move(m1), move(m2));
  }
  double tm = t.stop();

  cout << "m1 size: " << m1.size() << endl;

  return tm;
}

double test_intersect(size_t n, size_t m) {
  parlay::sequence<par> v1 = uniform_input_unsorted(n, 4294967295);
  tmap m1(v1);
  size_t nx = m1.size();

  // parlay::sequence<par> v2 = uniform_input(m, (n/m) * 2);
  parlay::sequence<par> v2 = uniform_input_unsorted(m, 4294967295);
  parlay::parallel_for(0, m / 2, [&](int i) { v2[i] = v1[i]; });
  tmap m2(v2);
  size_t mx = m2.size();

  timer t;
  t.start();
  tmap m3 = tmap::map_intersect(m1, m2);
  double tm = t.stop();
  std::cout << "Intersection size = " << m3.size() << std::endl;

  check(m1.size() == nx, "map size is wrong");
  check(m2.size() == mx, "map size is wrong");
  if (do_check) {
    check(check_intersect(m1, m2, m3), "intersect is wrong");
  }

  return tm;
}

double test_deletion(size_t n, size_t m) {
  parlay::sequence<par> v = uniform_input(n, 20);
  tmap m1(v);

  parlay::sequence<par> u = uniform_input(m, (n / m) * 20, true);

  timer t;
  t.start();
  for (size_t i = 0; i < m; ++i) m1 = tmap::remove(move(m1), std::get<0>(u[i]));

  double tm = t.stop();
  return tm;
}

double test_deletion_destroy(size_t n) {
  parlay::sequence<par> v = uniform_input(n, 20, true);
  tmap m1;
  for (size_t i = 0; i < n; ++i) m1.insert(v[i]);
  parlay::random_shuffle(v);

  timer t;
  t.start();
  for (size_t i = 0; i < n; ++i) m1 = tmap::remove(move(m1), std::get<0>(v[i]));
  double tm = t.stop();

  return tm;
}

double test_insertion_build(size_t n) {
  // parlay::sequence<par> v = uniform_input(n, 20, true);
  parlay::sequence<par> v = uniform_input_unsorted(n, 1000000000);
  tmap m1;

  timer t;
  t.start();
  for (size_t i = 0; i < n; ++i) {
    m1.insert(v[i]);
  }
  double tm = t.stop();
  return tm;
}

double test_insertion_build_persistent(size_t n) {
  parlay::sequence<par> v = uniform_input(n, 20, true);
  tmap m1;
  tmap* r = new tmap[n];
  tmap::reserve(30 * n);

  timer t;
  t.start();
  r[0].insert(v[0]);
  for (size_t i = 1; i < n; ++i) {
    r[i] = r[i - 1];
    r[i].insert(v[i]);
  }
  double tm = t.stop();

  delete[] r;
  return tm;
}

double test_insertion(size_t n, size_t m) {
  parlay::sequence<par> v = uniform_input(n, 20);
  tmap m1(v);

  parlay::sequence<par> u = uniform_input(m, (n / m) * 20, true);

  timer t;
  t.start();
  for (size_t i = 0; i < m; ++i) {
    m1.insert(u[i]);
  }

  double tm = t.stop();
  return tm;
}

double test_map(size_t n) {
  parlay::sequence<par> v = uniform_input(n, 20, true);
  tmap m1(v);

  timer t; t.start();

  auto map_f = [] (const par& kv) { return std::get<1>(kv); };
  auto m2 = tmap::map(m1, map_f);

  double tm = t.stop();
  std::cout << "m1 size = " << m1.size() << " m2 size = " << m2.size() << std::endl;

  return tm;
}

tmap build_slow(par *A, size_t n) {
  if (n <= 0)
    return tmap();
  if (n == 1)
    return tmap(A[0]);
  size_t mid = n / 2;

  tmap a, b;
  auto left = [&]() { a = build_slow(A, mid); };
  auto right = [&]() { b = build_slow(A + mid, n - mid); };

  parlay::par_do_if(n > 100, left, right);

  return tmap::map_union(std::move(a), std::move(b));
}

tset build_slow2(key_type *A, size_t n) {
  if (n <= 0)
    return tset();
  if (n == 1)
    return tset(A[0]);
  size_t mid = n / 2;

  tset a, b;
  auto left = [&]() { a = build_slow2(A, mid); };
  auto right = [&]() { b = build_slow2(A + mid, n - mid); };

  parlay::par_do_if(n > 100, left, right);

  return tset::map_union(std::move(a), std::move(b));
}

double test_reduce(size_t n) {
  parlay::sequence<par> v = uniform_input_unsorted(n, 1UL << 40);

  tmap m1 = build_slow(v.data(), n);

  timer t;
  t.start();

  auto f = [&](par e) { return std::get<0>(e); };
  auto sum = tmap::map_reduce(m1, f, Add());

  double tm = t.stop();

  auto noop = [&](const auto &et) { return 0; };
  size_t size_in_bytes = m1.size_in_bytes(noop);
  std::cout << "sum = " << sum << " size in bytes = " << size_in_bytes << "\n";

  return tm;
}
double test_reduce2(size_t n) {
  parlay::sequence<par> v = uniform_input_unsorted(n, 1UL << 40);
  tmap m1(v);

  timer t;
  t.start();

  auto f = [&](par e) { return std::get<1>(e); };
  auto sum = tmap::map_reduce(m1, f, Add());

  double tm = t.stop();

  auto test_sum = std::get<1>(v[0]);
  for (size_t i = 1; i < n; i++) {
    test_sum += std::get<1>(v[i]);
  }
  std::cout << "sum = " << sum << " correct_sum = " << test_sum << "\n";

  return tm;
}

double test_reduce3(size_t n) {
  parlay::sequence<par> v = uniform_input_unsorted(n, 1UL << 40);
  std::unordered_map<key_type, key_type> correct;
  key_type test_sum = 0;
  for (size_t i = 0; i < n; i++) {
    correct[std::get<0>(v[i])] = std::get<1>(v[i]);
  }
  for (auto p : correct) {
    test_sum += p.second;
  }
  tmap m1 = build_slow(v.data(), n);

  timer t;
  t.start();

  auto f = [&](par e) { return std::get<1>(e); };
  auto sum = tmap::map_reduce(m1, f, Add());

  double tm = t.stop();

  std::cout << "sum = " << sum << " correct_sum = " << test_sum << "\n";

  return tm;
}

double test_map_range(size_t n, size_t range_size) {
  parlay::sequence<key_type> v = uniform_input_unsorted2(n, 1UL << 40);

  parlay::sequence<key_type> range_start =
      uniform_input_unsorted2(100000, (1UL << 40) - range_size);
  tset m1 = build_slow2(v.data(), n);
  timer t;
  t.start();
  if (range_size > 0) {
    auto res = parlay::reduce(
        parlay::map(range_start,
                    [&](key_type &p) {
                      uint64_t counter = 0;
                      uint64_t total = 0;
                      tset m2 = tset::range(m1, p, p + range_size);
                      tset::map_void(
                          m2,
                          [&counter, &total](auto elem) {
                            counter += 1;
                            total += elem;
                          },
                          std::numeric_limits<uint64_t>::max());
                      return std::pair<uint64_t, uint64_t>(counter, total);
                    }),
        Add_Pair());

    double tm = t.stop();

    std::cout << "total found = " << std::get<0>(res) << " sum was "
              << std::get<1>(res) << "\n";

    return tm;
  }
  return 0;
}

double test_update(size_t n) {
  parlay::sequence<par> v = uniform_input(n, 20, true);

  tmap m1(v);

  timer t;
  t.start();
  auto add_f = [&] (par& kv) {
    return std::get<1>(kv) + 1;
  };
  for (size_t i=0; i<v.size(); i++) {
    m1.update(std::get<0>(v[i]), add_f);
    std::get<1>(v[i]) += 1;
  }
  double tm = t.stop();

  check(m1.size() == n, "map size is wrong");
  if (do_check) {
    check(contains_kv(m1, v), "contains_kv is wrong");
  }

#ifndef PARLAY_USE_STD_ALLOC
  std::cout << "With built tree, used:" << std::endl;
  parlay::internal::memory_usage();

  std::cout << "Per-bucket details: " << std::endl;
  parlay::internal::get_default_allocator().print_stats();
#endif

  // TODO: fix
  // tmap::GC::alloc::print_stats();
  // parlay::default_allocator.print_stats();
  return tm;
}

double test_multi_insert(size_t n, size_t m) {
//  parlay::sequence<par> v = uniform_input(n, 20, true);
//  parlay::sequence<par> u = uniform_input(m, (n / m) * 20, true);
  parlay::sequence<par> v = uniform_input_unsorted(n, 4294967295);
  parlay::sequence<par> u = uniform_input_unsorted(m, 4294967295);
  tmap m1(v);

  timer t;
  t.start();
  m1 = tmap::multi_insert(m1, u);
  double tm = t.stop();
  return tm;
}

double test_dest_multi_insert(size_t n, size_t m) {
  parlay::sequence<par> v = uniform_input(n, 20);
  parlay::sequence<par> u = uniform_input(m, (n / m) * 20, true);
  cout << "size of u: " << u.size() << endl;

  tmap m1(v);
  timer t;
  t.start();
  m1 = tmap::multi_insert(move(m1), u);
  double tm = t.stop();

  // check(check_multi_insert(v, u, m1), "multi_insert wrong");

  return tm;
}

double test_dest_multi_insert2(size_t n, size_t m) {
  parlay::sequence<key_type> v = uniform_input_unsorted2(n, 1UL << 40);
  std::vector<parlay::sequence<key_type>> us;
  for (size_t i = 0; i < n / m; i++) {
    us.push_back(uniform_input_unsorted2(m, 1UL << 40));
  }
  tset m1 = build_slow2(v.data(), n);
  // std::cout << "number elements = " << m1.size() << "\n";
  // return 0;
  timer t;
  t.start();
  if (m > 0) {
    for (size_t i = 0; i < n / m; i++) {
      m1 = tset::multi_insert(move(m1), us[i]);
    }
  }
  double tm = t.stop();
  // std::cout << "number elements = " << m1.size() << "\n";

  // check(check_multi_insert(v, u, m1), "multi_insert wrong");

  return tm;
}

class parallel_zipf {
  double c = 0; // Normalization constant
  double alpha;
  uint64_t max;
  parlay::sequence<double> sum_probs; // Pre-calculated sum of probabilities
  std::uniform_real_distribution<double> dist;

public:
  parallel_zipf(uint64_t m, double a, uint64_t seed) : alpha(a), max(m) {
    dist = std::uniform_real_distribution<double>(0, 1);
    // double c_test = 0;
    // for (uint64_t i = 1; i <= max; i++) {
    //   c_test = c_test + (1.0 / pow((double)i, alpha));
    // }
    // c_test = 1.0 / c_test;

    auto c_vec = parlay::delayed_tabulate(
        max, [&](size_t i) { return (1.0 / pow((double)i + 1, alpha)); });
    c = parlay::reduce(c_vec);
    c = 1.0 / c;
    // std::vector<double> sum_probs_test;
    // sum_probs_test.reserve(max + 1);
    // sum_probs_test.push_back(0);
    // for (uint64_t i = 1; i <= max; i++) {
    //   sum_probs_test.push_back(sum_probs_test[i - 1] +
    //                            c / pow((double)i, alpha));
    // }
    sum_probs = parlay::tabulate(max + 1, [&](size_t i) {
      if (i == 0) {
        return 0.0;
      } else {
        return c / pow((double)i, alpha);
      }
    });
    parlay::scan_inclusive_inplace(sum_probs);
  }

  uint64_t gen() {
    double z;
    // Pull a uniform random number (0 < z < 1)
    do {
      z = dist(get_rand_gen());
    } while (z == 0);

    // Map z to the value
    uint64_t low = 1;
    uint64_t high = max;
    do {
      uint64_t mid = (low + high) / 2;
      if (sum_probs[mid] >= z && sum_probs[mid - 1] < z) {
        // Assert that zipf_value is between 1 and N
        assert((mid >= 1) && (mid <= max));
        return mid;
      }
      if (sum_probs[mid] >= z) {
        high = mid - 1;
      } else {
        low = mid + 1;
      }
    } while (low <= high);
    assert(false);
    return 1;
  }
  parlay::sequence<uint64_t> gen_vector(uint64_t count) {
    return parlay::tabulate(count, [&](size_t i) {
      double z;
      // Pull a uniform random number (0 < z < 1)
      do {
        z = dist(get_rand_gen());
      } while (z == 0);
      uint64_t low = 1;
      uint64_t high = max;
      do {
        uint64_t mid = (low + high) / 2;
        if (sum_probs[mid] >= z && sum_probs[mid - 1] < z) {
          // Assert that zipf_value is between 1 and N
          assert((mid >= 1) && (mid <= max));
          return mid;
        }
        if (sum_probs[mid] >= z) {
          high = mid - 1;
        } else {
          low = mid + 1;
        }
      } while (low <= high);
      assert(false);
      return 1UL;
    });
  }
};

double test_dest_multi_insert2_zip(size_t n, size_t m) {
  parlay::sequence<key_type> v = uniform_input_unsorted2(n, 1UL << 34);
  std::vector<parlay::sequence<key_type>> us;
  parallel_zipf zip(34, .99, 0);
  for (size_t i = 0; i < n / m; i++) {
    us.push_back(zip.gen_vector(m));
  }
  tset m1 = build_slow2(v.data(), n);
  // std::cout << "number elements = " << m1.size() << "\n";
  // return 0;
  timer t;
  t.start();
  if (m > 0) {
    for (size_t i = 0; i < n / m; i++) {
      m1 = tset::multi_insert(move(m1), us[i]);
    }
  }
  double tm = t.stop();
  std::cout << "number elements = " << m1.size() << "\n";

  // check(check_multi_insert(v, u, m1), "multi_insert wrong");

  return tm;
}

double test_dest_multi_insert_cache(size_t n, size_t m) {

  tmap m1;
  timer t;
  t.start();

  // auto v = uniform_input_unsorted(n, 1UL<<40);

  // for (size_t i = 0; i < n; i++) {
  //   par el = v[i];
  //   // par el = {i,i};
  //   m1.insert(el);
  //   std::cout <<std::get<0>(el) <<", " <<std::get<1>(el)  << " number
  //   elements = " << m1.size() << "\n";
  // }

  for (size_t i = 0; i < n / m; i++) {
    auto v = uniform_input_unsorted(m, 1UL << 40);
    m1 = tmap::multi_insert(move(m1), v);
  }

  double tm = t.stop();
  std::cout << "number elements = " << m1.size() << "\n";

  // check(check_multi_insert(v, u, m1), "multi_insert wrong");

  return tm;
}

double stl_insertion(size_t n, size_t m) {
  parlay::sequence<par> v = uniform_input(n, 20);
  parlay::sequence<par> u = uniform_input(m, (n / m) * 20, true);

  map<key_type, key_type> m1;
  for (size_t i = 0; i < n; ++i) {
    auto [fst, snd] = v[i];
    m1.insert({fst, snd});
  }

  timer t;
  t.start();
  for (size_t i = 0; i < m; ++i) {
    auto [fst, snd] = u[i];
    m1.insert({fst, snd});
  }
  double tm = t.stop();

  return tm;
}

double stl_insertion_build(size_t n) {
  // parlay::sequence<par> v = uniform_input(n, 20, true);
  parlay::sequence<par> v = uniform_input_unsorted(n, 1000000000);
  map<key_type, key_type> m1;

  timer t;
  t.start();
  for (size_t i = 0; i < n; ++i) {
    auto [fst, snd] = v[i];
    m1.insert({fst, snd});
  }
  double tm = t.stop();

  return tm;
}

double test_build(size_t n) {
  // parlay::sequence<par> v = uniform_input_unsorted(n, 1000000000);
  timer t;
  t.start();
  parlay::sequence<par> v = uniform_input(n, 20, true);
  t.next("uniform generate time");

  timer tt;tt.start();
  tmap m1(v);
  double tm = tt.stop();
  t.next("build time");
  v.clear();

  std::cout << "m1.size = " << m1.size() << " n = " << n << std::endl;
  check(m1.size() == n, "map size is wrong");
  if (do_check) {
    check(contains(m1, v), "build is wrong");
  }

#ifndef PARLAY_USE_STD_ALLOC
  std::cout << "With built tree, used:" << std::endl;
  parlay::internal::memory_usage();

  std::cout << "Per-bucket details: " << std::endl;
  parlay::internal::get_default_allocator().print_stats();
  std::cout << "End of memory stats" << std::endl;
#endif
  tmap::GC::alloc::print_stats();
  tmap::GC::complex_alloc::print_stats();

  // TODO: fix
  // tmap::GC::alloc::print_stats();
  // parlay::default_allocator.print_stats();
  return tm;
}

double test_filter(size_t n) {
  parlay::sequence<par> v = uniform_input(n, 20);
  auto m1 = tmap(v);

  timer t;
  t.start();
  //auto cond = [](par t) { return true; };
  auto cond = [](par t) { return (std::get<0>(t) & 1) == 1; };
  //auto cond = [](par t) { return (parlay::hash64(t.first) & 1) == 1; };
  tmap res = tmap::filter(m1, cond);
  double tm = t.stop();
  std::cout << "m1.size = " << m1.size() << std::endl;
  //std::cout << "res.size = " << res.size() << std::endl;
  check(res.ref_cnt() == 1, "ref cnt is wrong");

  //tmap::GC::alloc::print_stats();
  check(m1.ref_cnt() == 1, "ref cnt is wrong");

  check(m1.size() == n, "map size is wrong");
  if (do_check) {
    check(check_filter(m1, res, cond), "filter is wrong");
    m1.check_structure();
    std::cout << "Filter is OK" << std::endl;
  }

  return tm;
}

// double test_map_reduce(size_t n) {
//   parlay::sequence<par> v = uniform_input(n, 20);
//   tmap m1(v);
//
//   timer t;
//   auto f = [&](par e) { return e.second; };
//   t.start();
//   tmap::map_reduce(m1, f, Add());
//   double tm = t.stop();
//
//   return tm;
// }

// double test_map_filter(size_t n) {
//   parlay::sequence<par> v = uniform_input(n, 20);
//   auto m1 = tmap(v);
//   timer t;
//   t.start();
//   auto cond = [](par t) { return (t.first & 1) == 1; };
//   auto res = tmap::map_filter(m1, cond);
//   double tm = t.stop();
//
//   tmap::GC::alloc::print_stats();
//   check(res.ref_cnt() == 1, "ref cnt is wrong");
//   check(m1.ref_cnt() == 1, "ref cnt is wrong");
//
//   return tm;
// }

// double test_map_set(size_t n) {
//   parlay::sequence<par> v = uniform_input(n, 20);
//   auto m1 = tmap(v);
//   timer t;
//   t.start();
//   auto f = [](par t) { return t; };  /* identity */
//   auto res = tmap::map_set(m1, f);
//   double tm = t.stop();
//
//   tmap::GC::alloc::print_stats();
//   check(res.ref_cnt() == 1, "ref cnt is wrong");
//   check(m1.ref_cnt() == 1, "ref cnt is wrong");
//
//   return tm;
// }

double test_aug_filter(size_t n, size_t m) {
  parlay::sequence<par> v = uniform_input(n, 20);
  key_type threshold = n - m;
  tmap m1(v);
  tmap res = m1;
  tmap res2 = m1;

  timer t;
  t.start();
#ifndef NO_AUG
  auto cond = [&](key_type t) {
    return (t > threshold); };
  res = tmap::aug_filter(std::move(res), cond);
  cout << "filter result: " << res.size() << endl;
#endif
  double tm = t.stop();

  auto cond2 = [&](par t) { return (std::get<1>(t) > threshold); };
  timer t2;
  t2.start();
  res2 = tmap::filter(move(res2), cond2);
  double tm2 = t2.stop();
  cout << "filter result: " << res2.size() << endl;
  cout << "Plain filter: " << tm2 << endl;

  if (do_check) {
    check(m1.size() == n, "map size is wrong");
  }
  // check(check_aug_filter(m1, res, cond), "filter is wrong");

  return tm;
}

double test_dest_union(size_t n, size_t m) {
  // parlay::sequence<par> v1 = uniform_input(n, 20);
  parlay::sequence<par> v1 = uniform_input_unsorted(n, 4294967295);
  // parlay::sequence<par> v1 = uniform_input_unsorted(n, 100);
  tmap m1(v1);
  tmap m1_copy(v1);

  // parlay::sequence<par> v2 = uniform_input(m, (n/m) * 20);
  parlay::sequence<par> v2 = uniform_input_unsorted(m, 4294967295);
  tmap m2(v2);
  tmap m2_copy(v2);

  cout << "constructed" << endl;

  timer t;
  t.start();
  tmap m3 = tmap::map_union(std::move(m1), std::move(m2));
  double tm = t.stop();
  cout << "done" << endl;

  check(m1.size() == 0, "map size is wrong");
  check(m2.size() == 0, "map size is wrong");
  if (do_check) {
    check(check_union(m1_copy, m2_copy, m3), "union is wrong");
  }

  return tm;
}

double test_dest_intersect(size_t n, size_t m) {
  // parlay::sequence<par> v1 = uniform_input(n, 20);
  parlay::sequence<par> v1 = uniform_input_unsorted(n, 4294967295);
  tmap m1(v1);
  tmap m1_copy(v1);

  // parlay::sequence<par> v2 = uniform_input(m, (n/m) * 2);
  parlay::sequence<par> v2 = uniform_input_unsorted(m, 4294967295);
  parlay::parallel_for(0, m / 2, [&](int i) { v2[i] = v1[i]; });
  tmap m2(v2);
  tmap m2_copy(v2);

  timer t;
  t.start();
  tmap m3 = tmap::map_intersect(move(m1), move(m2));
  double tm = t.stop();

  check(m1.size() == 0, "map size is wrong");
  check(m2.size() == 0, "map size is wrong");
  if (do_check) {
    check(check_intersect(m1_copy, m2_copy, m3), "intersect is wrong");
  }

  return tm;
}

double test_split(size_t n) {
  parlay::sequence<par> v = uniform_input(n, 20);

  tmap m1(v);

  // key_type key = v[n / 2].first;

  timer t;
  t.start();
  pair<tmap, tmap> res; // = m1.split(key);
  double tm = t.stop();

  check(m1.size() == n, "map size is wrong");
  check(res.first.size() + res.second.size() + 1 == n,
        "splitted map size is wrong");
  // check(check_split(key, v, res), "split is wrong");

  return tm;
}

// a destructive version for now
double test_difference(size_t n, size_t m) {
  parlay::sequence<par> v1 = uniform_input(n, 20);
  //parlay::sequence<par> v1 = uniform_input_unsorted(n, 4294967295);
  tmap m1(v1);
  // tmap m1_copy(v1);

  // parlay::sequence<par> v2 = uniform_input(m, (n/m) * 20);
  parlay::sequence<par> v2 = uniform_input(n, 20);
  //parlay::sequence<par> v2 = uniform_input_unsorted(m, 4294967295);
  parlay::parallel_for(0, m / 2, [&](int i) { v2[i] = v1[i]; });
  tmap m2(v2);
  // tmap m2_copy(v2);

  timer t;
  t.start();
  std::cout << "map diff start" << std::endl;
  tmap m3 = tmap::map_difference(move(m1), move(m2));
  double tm = t.stop();

  check(m1.size() == 0, "map size is wrong");
  check(m2.size() == 0, "map size is wrong");
  // check(check_difference(m1_copy, m2_copy, m3), "difference is wrong");

  return tm;
}

double test_find(size_t n, size_t m) {
  parlay::sequence<par> v1 = uniform_input(n, 20);
  key_type max_key = std::get<0>(v1[n - 1]);
  tmap m1(v1);

  parlay::sequence<par> v2 = uniform_input_unsorted(m, max_key);

  bool* v3 = new bool[m];

  timer t;
  t.start();
  auto f = [&](size_t i) {
    v3[i] = m1.find(std::get<0>(v2[i])).has_value();
  };
  parlay::parallel_for(0, m, f);

  double tm = t.stop();
  delete[] v3;

  return tm;
}

double test_size(size_t n) {
  parlay::sequence<key_type> v1 = uniform_input_unsorted2(n, 1UL << 40);
  tset m1(v1);

  auto noop = [&](const auto &et) { return 0; };
  size_t size_in_bytes = m1.size_in_bytes(noop);

  std::cout << "RESULT: n = " << n << ", size in bytes = " << size_in_bytes
            << std::endl;

//  uint8_t tmp[8];
//  size_t bytes = sizeof(key_type)*v1.size(); // values
//  size_t bits = sizeof(key_type)*v1.size(); // values
//  bytes += cpam::encodeUnsigned(tmp, 0, std::get<0>(v1[0]));
//  key_type prev = std::get<0>(v1[0]);
//  for (size_t i=1; i<v1.size(); i++) {
//    key_type diff = std::get<0>(v1[i]) - prev;
//    prev = std::get<0>(v1[i]);
//    bytes += cpam::encodeUnsigned(tmp, 0, diff);
//    bits += parlay::log2_up(diff);
//  }
//
//  std::cout << "RESULT: Lower bound (byte-encoded) = " << bytes << std::endl;
//  std::cout << "RESULT: Lower bound (bits) = " << bits << std::endl;

  return 1.0;
}

double test_range(size_t n, size_t m) {
  parlay::sequence<par> v1 = uniform_input(n, 20);
  key_type max_key = std::get<0>(v1[n - 1]);
  // std::cout << "Max_key = " << max_key << std::endl;
  tmap m1(v1);

  parlay::sequence<par> v2 = uniform_input_unsorted(m, max_key);
  // window size is 1/1000 of total width
  size_t win = max_key/1000;

  tmap* v3 = new tmap[m];

  timer t;
  t.start();
  auto f = [&](size_t i) {
//    std::cout << "m1.size = " << m1.size() << " cnt = " << m1.ref_cnt()
//      << std::endl;
    v3[i] = tmap::range(m1, std::get<0>(v2[i]), std::get<0>(v2[i]) + win);
  };

//  auto in_range = [&] (size_t l, size_t r) -> size_t {
//    size_t ct = 0;
//    for (size_t i=0; i<v1.size(); i++) {
//      if (l <= std::get<0>(v1[i]) && std::get<0>(v1[i]) <= r) ct++;
//    }
//    return ct;
//  };

  parlay::parallel_for(0, m, f);
//  for (size_t i=0; i<m; i++) {
//    f(i);
//  }
//  for (size_t i=0; i<std::min(m, (size_t)100); i++) {
//    size_t cor = in_range(std::get<0>(v2[i]), std::get<0>(v2[i]) +
//        win);
//    std::cout << m1.size() << " " << v3[i].size() << " " << " cor = "
//      << cor << " " << std::get<0>(v2[i]) << " " <<  (std::get<0>(v2[i]) + win) << std::endl;
//  }
  double tm = t.stop();

  // do deletion in parallel
  parlay::parallel_for(0, m, [&](size_t i) { v3[i] = tmap(); });
  delete[] v3;

  return tm;
}

double test_aug_range(size_t n, size_t m) {
  parlay::sequence<par> v1 = uniform_input_unsorted(n, 1000000000);
  key_type max_key = std::get<0>(v1[n - 1]);
  tmap m1(v1);

  parlay::sequence<par> v2 = uniform_input_unsorted(m, max_key);

  key_type* v3 = new key_type[m];

  timer t;
  t.start();
#ifndef NO_AUG
  // window size is 1/1000 of total width
  size_t win = max_key / 1000;

  parlay::parallel_for(0, m, [&](size_t i) {
    v3[i] = m1.aug_range(std::get<0>(v2[i]), std::get<0>(v2[i]) + win);
  });
#endif
  double tm = t.stop();

  delete[] v3;

  return tm;
}

//double test_aug_left(size_t n, size_t m) {
//  parlay::sequence<par> v1 = uniform_input(n, 20);
//  key_type max_key = v1[n - 1].first;
//  tmap m1(v1);
//
//  parlay::sequence<par> v2 = uniform_input_unsorted(m, max_key);
//
//  key_type* v3 = new key_type[m];
//
//  timer t;
//  t.start();
//#ifndef NO_AUG
//  parlay::parallel_for(0, m, [&](size_t i) { v3[i] = m1.aug_left(v2[i].first); });
//#endif
//  double tm = t.stop();
//
//  delete[] v3;
//
//  return tm;
//}

double stl_set_union(size_t n, size_t m) {
  parlay::sequence<par> v1 = uniform_input(n, 20);
  parlay::sequence<par> v2 = uniform_input(m, 20 * (n / m));

  // set <key_type> s1, s2, sret;
  // key_type* s1 = new key_type[n];
  // key_type* s2 = new key_type[m];
  vector<key_type> s1(n), s2(m);
  vector<key_type> sret2;

  for (size_t i = 0; i < n; ++i) {
    // s1.insert(v1[i].first);
    s1[i] = std::get<0>(v1[i]);
  }
  for (size_t i = 0; i < m; ++i) {
    // s2.insert(v2[i].first);
    s2[i] = std::get<0>(v2[i]);
  }

  timer t;
  t.start();
  // set_union(s1.begin(), s1.end(), s2.begin(), s2.end(), inserter(sret,
  // sret.begin()));
  // set_union(s1.begin(), s1.end(), s2.begin(), s2.end(), sret2.begin());
  // set_union(s1, s1+n, s2, s2+m, sret2.begin());
  set_union(s1.begin(), s1.end(), s2.begin(), s2.end(),
            std::back_inserter(sret2));
  double tm = t.stop();

  return tm;
}

double stl_map_union(size_t n, size_t m) {
  // parlay::sequence<par> v1 = uniform_input(n, 20);
  // parlay::sequence<par> v2 = uniform_input(m, 20 * (n / m));
  parlay::sequence<par> v1 = uniform_input_unsorted(n, 4294967295);
  parlay::sequence<par> v2 = uniform_input_unsorted(m, 4294967295);

  std::map<key_type, key_type> s1, s2;
  for (size_t i = 0; i < n; ++i) {
    auto [fst, snd] = v1[i];
    s1.insert({fst, snd});
  }
  for (size_t i = 0; i < m; ++i) {
    auto [fst, snd] = v2[i];
    s2.insert({fst, snd});
  }

  timer t;
  t.start();
  s1.merge(s2);
  // set_union(s1.begin(), s1.end(), s2.begin(), s2.end(), back_inserter(sret));
  double tm = t.stop();

  return tm;
}

double stl_vector_union(size_t n, size_t m) {
  parlay::sequence<par> v1 = uniform_input(n, 20);
  parlay::sequence<par> v2 = uniform_input(m, 20 * (n / m));

  vector<mapped> s1, s2, sret;
  sret.reserve(m + n);

  for (size_t i = 0; i < n; ++i) {
    s1.push_back(mapped(std::get<0>(v1[i]), std::get<1>(v1[i])));
  }
  for (size_t i = 0; i < m; ++i) {
    s2.push_back(mapped(std::get<0>(v2[i]), std::get<1>(v2[i])));
  }

  timer t;
  t.start();
  set_union(s1.begin(), s1.end(), s2.begin(), s2.end(), back_inserter(sret));
  double tm = t.stop();

  return tm;
}


//double intersect_multi_type(size_t n, size_t m) {
//  parlay::sequence<par> v1 = uniform_input(n, 2);
//  tmap m1(v1);
//
//  parlay::sequence<par> v2 = uniform_input(m, (n / m) * 2);
//  pair<key_type, bool>* vv2 = new pair<key_type, bool>[m];
//  for (size_t i = 0; i < m; i++) {
//    vv2[i].first = v2[i].first;
//    vv2[i].second = v2[i].second | 4;
//  }
//  // tmap m2(v2, v2 + m);
//  using tmap2 = pam_map<entry2>;
//  using tmap3 = pam_map<entry3>;
//  tmap3::reserve(m);
//  tmap2::reserve(m);
//  tmap2 m2(vv2, vv2 + m);
//
//  auto f = [&](key_type a, bool b) -> char {
//    if (b)
//      return 'a';
//    else
//      return 'b';
//  };  //+a%10;};
//
//  timer t;
//  t.start();
//  tmap3 m3;
//  m3 = tmap3::map_intersect(m1, m2, f);
//  double tm = t.stop();
//  cout << "size: " << m3.size() << ", " << m1.size() << ", " << m2.size()
//       << endl;
//  tmap::GC::print_stats();
//  tmap2::GC::print_stats();
//  tmap3::GC::print_stats();
//
//  // key_type kk = m3.select(3).first;
//  // cout << m3.find(kk).value << endl;
//
//  check(m1.size() == n, "map size is wrong");
//  check(m2.size() == m, "map size is wrong");
//  // check(check_intersect(m1, m2, m3), "intersect is wrong");
//
//  tmap3::finish();
//  tmap2::finish();
//
//  return tm;
//}

string test_name[] = {"persistent-union",                     // 0
                      "persistent-intersect",                 // 1
                      "insert",                               // 2
                      "std::map-insert",                      // 3
                      "build",                                // 4
                      "filter",                               // 5
                      "destructive-union",                    // 6
                      "destructive-intersect",                // 7
                      "split",                                // 8
                      "difference",                           // 9
                      "std::set_union",                       // 10
                      "std::vector_union",                    // 11
                      "find",                                 // 12
                      "delete",                               // 13
                      "multi_insert",                         // 14
                      "destructive multi_insert",             // 15
                      "stl_insertion_build",                  // 16
                      "test_deletion_destroy",                // 17
                      "test_range",                           // 18
                      "test_aug_range",                       // 19
                      "test_insertion_build",                 // 20
                      "stl_delete_destroy",                   // 21
                      "test_aug_left",                        // 22
                      "stl_map_union",                        // 23
                      "aug_filter",                           // 24
                      "insertion_build_persistent",           // 25
                      "test_incremental_union_nonpersistent", // 26
                      "intersect_multi_type",                 // 27
                      "flat_aug_range",                       // 28
                      "test_map_reduce",                      // 29
                      "test_map_filter",                      // 30
                      "test_map_set",                         // 31
                      "test_update",                          // 32
                      "test_map",                             // 33
                      "test_reduce",                          // 34
                      "test_size",                            // 35
                      "test_reduce2",                         // 36
                      "test_multi_insert2",                   // 37
                      "test_reduce3",                         // 38
                      "test_multi_insert_cache",              // 39
                      "test_map_range",                       // 40
                      "test_multi_insert2_zip",               // 41
                      "nothing"};

//double flat_aug_range(size_t n, size_t m) {
//  parlay::sequence<par> v1 = uniform_input(n, 20);
//  key_type max_key = v1[n - 1].first;
//  tmap m1(v1, true);
//
//  parlay::sequence<par> v2 = uniform_input_unsorted(m, max_key);
//
//  key_type* v3 = new key_type[m];
//  size_t win = v1[n - 1].first / 1000;
//
//  timer t;
//  t.start();
//  parlay::parallel_for(0, m, [&](size_t i) {
//    tmap m2 = tmap::range(m1, v2[i].first, v2[i].first + win);
//    auto f = [](par e) { return e.second; };
//    v3[i] = tmap::map_reduce(m2, f, Add());
//  });
//
//  double tm = t.stop();
//
//  delete[] v3;
//
//  return tm;
//}

double execute(size_t id, size_t n, size_t m) {
  switch (id) {
    case 0:
      return test_union(n, m);
    case 1:
      return test_intersect(n, m);
    case 2:
      return test_insertion(n, m);
    case 3:
      return stl_insertion(n, m);
    case 4:
      return test_build(n);
    case 5:
      return test_filter(n);
    case 6:
      return test_dest_union(n, m);
    case 7:
      return test_dest_intersect(n, m);
    case 8:
      return test_split(n);   // seems to be unused.
    case 9:
      return test_difference(n, m);
    case 10:
      return stl_set_union(n, m);
    case 11:
      return stl_vector_union(n, m);
    case 12:
      return test_find(n, m);
    case 13:
      return test_deletion(n, m);
    case 14:
      return test_multi_insert(n, m);
    case 15:
      return test_dest_multi_insert(n, m);
    case 16:
      return stl_insertion_build(n);
    case 17:
      return test_deletion_destroy(n);
    case 18:
      return test_range(n, m);
    case 19:
      return test_aug_range(n, m);
    case 20:
      return test_insertion_build(n);
    //case 21:
    //  return stl_delete_destroy(n);
    //case 22:
    //  return test_aug_left(n, m);
    case 23:
      return stl_map_union(n, m);
    case 24:
      return test_aug_filter(n, m);
    case 25:
      return test_insertion_build_persistent(n);
    case 26:
      return test_incremental_union_nonpersistent(n, m);
    //case 27:
    //  return intersect_multi_type(n, m);
    //case 28:
    //  return flat_aug_range(n, m);
    //case 29:
    //  return test_map_reduce(n);
    //case 30:
    //  return test_map_filter(n);
    //case 31:
    //  return test_map_set(n);
    case 32:
      return test_update(n);
    case 33:
      return test_map(n);
    case 34:
      return test_reduce(n);
    case 35:
      return test_size(n);
    case 36:
      return test_reduce2(n);
    case 37:
      return test_dest_multi_insert2(n, m);
    case 38:
      return test_reduce3(n);
    case 39:
      return test_dest_multi_insert_cache(n, m);
    case 40:
      return test_map_range(n, m);
    case 41:
      return test_dest_multi_insert2_zip(n, m);
    default:
      assert(false);
      return 0.0;
  }
}

void test_loop(size_t n, size_t m, size_t repeat, size_t test_id,
               bool randomize) {
  size_t threads = parlay::num_workers();
  size_t reserve_size = (test_id == 4 || test_id == 19) ? n : 4 * n;
  cout << "threads = " << threads << endl;
  parlay::sequence<double> times(repeat);
  for (size_t i = 0; i < (repeat+1); ++i) {
    cout << reserve_size << endl;
    tmap::reserve(reserve_size);
    double tm = execute(test_id, n, m);
    check(tmap::GC::num_used_nodes() == 0, "used nodes at end of test");

    // tmap::GC::alloc::print_stats();
    //std::cout << "C-Tree Block Size: " << tmap::Tree::kCompressionBlockSize << std::endl;
    cout << "RESULT" << fixed << setprecision(6)
         << "\ttest=" << test_name[test_id] << "\ttime=" << tm << "\tn=" << n
         << "\tm=" << m << "\titeration=" << i << "\tp=" << threads << endl;

    tmap::finish();
    if (i > 0)  // warmup on the first round
      times[i-1] = tm;
  }
  std::cout << "Average time: " <<
    (parlay::reduce(parlay::make_slice(times)) / times.size()) <<
    std::endl;
}

}


int main(int argc, char* argv[]) {
  commandLine P(
      argc, argv,
      "./testParallel [-n size1] [-m size2] [-r rounds] [-p] <testid>");
  size_t n = P.getOptionLongValue("-n", 10000000);
  size_t m = P.getOptionLongValue("-m", n);
  size_t repeat = P.getOptionIntValue("-r", 5);
  size_t test_id = cpam::str_to_int(P.getArgument(0));
  cpam::do_check = P.getOption("-check");
  bool randomize = P.getOption("-p");
  cpam::test_loop(n, m, repeat, test_id, randomize);
  return 0;
}
