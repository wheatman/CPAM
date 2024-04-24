all: run_graph_histories-CPAM-CPAM-Diff run_graph_histories-CPAM-CPAM run_graph_histories-PAM-CPAM-Diff run_graph_histories-PAM-CPAM run_graph_histories-PAM-PAM

run_graph_histories-CPAM-CPAM-Diff:		run_graph_histories.cc
	g++-11 -O3 -DNDEBUG -DUSE_DIFF_ENCODING -mcx16 -march=native -DHOMEGROWN -pthread -std=c++17 -Wall -I../../../parlaylib/include -I../ -I../../../pam/include -I../../../include -o run_graph_histories-CPAM-CPAM-Diff run_graph_histories.cc -L/usr/local/lib -ljemalloc -L`jemalloc-config --libdir` -Wl,-rpath,`jemalloc-config --libdir` -ljemalloc `jemalloc-config --libs`

run_graph_histories-CPAM-CPAM:		run_graph_histories.cc
	g++-11 -O3 -DNDEBUG -mcx16 -march=native -DHOMEGROWN -pthread -std=c++17 -Wall -I../../../parlaylib/include -I../../../pam/include -I../../../include -I../ -o run_graph_histories-CPAM-CPAM run_graph_histories.cc -L/usr/local/lib -ljemalloc  -L`jemalloc-config --libdir` -Wl,-rpath,`jemalloc-config --libdir` -ljemalloc `jemalloc-config --libs`

run_graph_histories-PAM-CPAM:		run_graph_histories.cc
	g++-11 -O3 -DNDEBUG -DUSE_PAM_UPPER -mcx16 -march=native -DHOMEGROWN -pthread -std=c++17 -Wall -I../../../parlaylib/include -I../../../pam/include -I../ -I../../../include -o run_graph_histories-PAM-CPAM run_graph_histories.cc -L/usr/local/lib -ljemalloc  -L`jemalloc-config --libdir` -Wl,-rpath,`jemalloc-config --libdir` -ljemalloc `jemalloc-config --libs`
       
run_graph_histories-PAM-CPAM-Diff:		run_graph_histories.cc
	g++-11 -O3 -DNDEBUG -DUSE_PAM_UPPER -DUSE_DIFF_ENCODING -mcx16 -march=native -DHOMEGROWN -pthread -std=c++17 -Wall -I../../../parlaylib/include -I../ -I../../../pam/include -I../../../include -o run_graph_histories-PAM-CPAM-Diff run_graph_histories.cc -L/usr/local/lib -ljemalloc  -L`jemalloc-config --libdir` -Wl,-rpath,`jemalloc-config --libdir` -ljemalloc `jemalloc-config --libs`
       

run_graph_histories-PAM-PAM:		run_graph_histories.cc
	g++-11 -O3 -DNDEBUG -DUSE_PAM_UPPER -DUSE_PAM -mcx16 -march=native -DHOMEGROWN -pthread -std=c++17 -Wall -I../../../parlaylib/include -I../ -I../../../pam/include -I../../../include -o run_graph_histories-PAM-PAM run_graph_histories.cc -L/usr/local/lib -ljemalloc  -L`jemalloc-config --libdir` -Wl,-rpath,`jemalloc-config --libdir` -ljemalloc `jemalloc-config --libs`
       

clean:
	rm -f run_graph_histories-CPAM-CPAM-Diff run_graph_histories-CPAM-CPAM run_graph_histories-PAM-CPAM-Diff run_graph_histories-PAM-CPAM run_graph_histories-PAM-PAM
