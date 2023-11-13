all: bfs bc pr cc mis flatsnap

flatsnap: Flatsnap-CPAM

Flatsnap-CPAM:		Flatsnap.cc
	g++ -O3 -g -DNDEBUG -DUSE_DIFF_ENCODING -mcx16 -march=native -DHOMEGROWN -pthread -std=c++17 -Wall -I../../../parlaylib/include -I../../../pam/include -I../../../include -I../ -o Flatsnap-CPAM Flatsnap.cc -L/usr/local/lib -ljemalloc -L`jemalloc-config --libdir` -Wl,-rpath,`jemalloc-config --libdir` -ljemalloc `jemalloc-config --libs`


bc: BC-CPAM

BC-CPAM:		BC.cc
	g++ -O3 -g -DNDEBUG -DUSE_DIFF_ENCODING -mcx16 -march=native -DHOMEGROWN -pthread -std=c++17 -Wall -I../../../parlaylib/include -I../../../pam/include -I../../../include -I../ -o BC-CPAM BC.cc -L/usr/local/lib -ljemalloc -L`jemalloc-config --libdir` -Wl,-rpath,`jemalloc-config --libdir` -ljemalloc `jemalloc-config --libs`


bfs: BFS-CPAM

BFS-CPAM:		BFS.cc
	g++ -O3 -g -DNDEBUG -DUSE_DIFF_ENCODING -mcx16 -march=native -DHOMEGROWN -pthread -std=c++17 -Wall -I../../../parlaylib/include -I../../../pam/include -I../../../include -I../ -o BFS-CPAM BFS.cc -L/usr/local/lib -ljemalloc -L`jemalloc-config --libdir` -Wl,-rpath,`jemalloc-config --libdir` -ljemalloc `jemalloc-config --libs`

pr: PR-CPAM

PR-CPAM:		PR.cc
	g++ -O3 -g -DNDEBUG -DUSE_DIFF_ENCODING -mcx16 -march=native -DHOMEGROWN -pthread -std=c++17 -Wall -I../../../parlaylib/include -I../../../pam/include -I../../../include -I../ -o PR-CPAM PR.cc -L/usr/local/lib -ljemalloc -L`jemalloc-config --libdir` -Wl,-rpath,`jemalloc-config --libdir` -ljemalloc `jemalloc-config --libs`

cc: CC-CPAM

CC-CPAM:		CC.cc
	g++ -O3 -g -DNDEBUG -DUSE_DIFF_ENCODING -mcx16 -march=native -DHOMEGROWN -pthread -std=c++17 -Wall -I../../../parlaylib/include -I../../../pam/include -I../../../include -I../ -o CC-CPAM CC.cc -L/usr/local/lib -ljemalloc -L`jemalloc-config --libdir` -Wl,-rpath,`jemalloc-config --libdir` -ljemalloc `jemalloc-config --libs`


mis: MIS-CPAM

MIS-CPAM:		MIS.cc
	g++ -O3 -DNDEBUG -DUSE_DIFF_ENCODING -mcx16 -march=native -DHOMEGROWN -pthread -std=c++17 -Wall -I../../../parlaylib/include -I../../../pam/include -I../../../include -I../ -o MIS-CPAM MIS.cc -L/usr/local/lib -ljemalloc -L`jemalloc-config --libdir` -Wl,-rpath,`jemalloc-config --libdir` -ljemalloc `jemalloc-config --libs`

clean:
	rm -f Flatsnap-CPAM BC-CPAM BFS-CPAM MIS-CPAM

