source=/tmp/experiment/exp15/bricks
dir=exp15
count=400

for i in $(seq 104 108)
do
  {
    log=/tmp/experiment/$dir/$i/synthesize.out
    fuzz=/tmp/experiment/$dir/$i/fuzz/
    mkdir $fuzz -p
    date
    /opt/llvm/llvm-project/build/bin/synthesize -s $source -o $fuzz -l 200 -f $count > $log 2>&1
    date
    rm $fuzz/*.out
    for j in $(seq 0 399)
    do
      echo $fuzz/fuzz$j.c
      /opt/llvm/llvm-project/build/bin/pointerGuard $fuzz/fuzz$j.c -- -I /tmp/include -m32 -w >  $fuzz/fuzz$j.clean.1.c
      /opt/llvm/llvm-project/build/bin/shiftGuard $fuzz/fuzz$j.clean.1.c -- -I /tmp/include -m32 -w >  $fuzz/fuzz$j.clean.2.c
      /opt/llvm/llvm-project/build/bin/floatGuard $fuzz/fuzz$j.clean.2.c -- -I /tmp/include -m32 -w >  $fuzz/fuzz$j.c
      rm $fuzz/fuzz$j.clean.1.c
      rm $fuzz/fuzz$j.clean.2.c
      # break
    done
    echo "${i} over!"
  }&
  echo "============== $i : $! ++++++++++"
done

wait
echo "Over"