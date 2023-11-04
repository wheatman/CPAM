

ROUNDS=10

function run {
  echo $1
  echo "BFS"
  numactl -i all ./BFS-CPAM -s -rounds $ROUNDS -src $2 $1 | grep "time per iter"

  echo "PR"
  numactl -i all ./PR-CPAM -s -rounds $ROUNDS -src $2 $1 | grep "time per iter"


  echo "BC"
  numactl -i all ./BC-CPAM -s -rounds $ROUNDS -src $2 $1 | grep "time per iter"

  echo "CC"
  numactl -i all ./CC-CPAM -s -rounds $ROUNDS -src $2 $1 | grep "time per iter"
}


run  /home/ubuntu/graphs/soc-LiveJournal1_sym.adj    0           
run  /home/ubuntu/graphs/com-orkut.ungraph.adj  1000               
run  /home/ubuntu/graphs/rmat_ligra.adj       0            
run  /home/ubuntu/graphs/er_graph.adj                 0      
run  /home/ubuntu/graphs/twitter.adj            12      
run  /home/ubuntu/graphs/com-friendster.ungraph.adj            100000     
