csmith=/path/bin/csmith

for i in $(seq 0 10000)
do
    $csmith > seed4/$i.c
    echo $i
done
