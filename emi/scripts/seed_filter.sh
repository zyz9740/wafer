root=/tmp/emi
diff=$root/diff
bad_seeds=$root/bad_seeds

# csmith_seeds=$root/seeds/csmith
csmith_seeds=$root/seeds/crazy_diamond
csmith_bad_seeds=${bad_seeds}/crazy_diamond
mkdir ${csmith_bad_seeds} -p


# for every seed program
for file in ${csmith_seeds}/*
do
  filename=${file##*/}
  if [ ${file##*.} = "c" ]; then
    echo "processing [$filename]..."
    # cp seed program to random.c
    make move T=$file

    # use gcc to compile
    make gcc-O0 T=random.c

    # check the compile result, if it is not 0, move to bad_seeds
    ret1=$?
    if [ $ret1 -ne 0 ]; then
      echo "Gcc build error!"
      mv $file ${csmith_bad_seeds}/$filename
      continue
    fi

    # run the binary twice, check whether the output and result are the same, if not, move to bad_seeds
    timeout 5s ./random.o > gcc.out1 2>&1
    ret2=$?
    if [ $ret2 -ne 0 ]; then
      echo "Run time out!"
      mv $file ${csmith_bad_seeds}/$filename
      continue
    fi

    timeout 5s ./random.o > gcc.out2 2>&1
    ret3=$?
    if [ $ret3 -ne 0 ]; then
      echo "Run time out!"
      mv $file ${csmith_bad_seeds}/$filename
      continue
    fi

    diff gcc.out1 gcc.out2
    ret4=$?
    if [ $ret4 -eq 1 ]; then
      echo "Unstable!"
      mv $file ${csmith_bad_seeds}/$filename
      continue
    fi

    # run the coverage script, if the coverage output is empty, move to bad_seeds
    coverage_file=$diff/random_coverage.txt
    timeout 10s bash main.sh random
    if [ $? -ne 0 ] && [ ! -s ${coverage_file} ]; then
      echo "DeadCode file is empty!"
      mv $file ${csmith_bad_seeds}/$filename
      continue
    fi
    # break
  fi
done


