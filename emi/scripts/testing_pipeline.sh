root=/tmp/emi
exp_index=exp2
result=$root/results/${exp_index}

good_mutant=$result/good_mutant

em_build_error_seeds=$result/seeds/em_build_error_seeds
em_runtime_error_seeds=$result/seeds/em_runtime_error_seeds
em_build_error_mutant=$result/mutant/em_build_error_mutant
em_runtime_error_mutant=$result/mutant/em_runtime_error_mutant

cheerp_build_error_seeds=$result/seeds/cheerp_build_error_seeds
cheerp_runtime_error_seeds=$result/seeds/cheerp_runtime_error_seeds
cheerp_build_error_mutant=$result/mutant/cheerp_build_error_mutant
cheerp_runtime_error_mutant=$result/mutant/cheerp_runtime_error_mutant

mkdir ${good_mutant} -p

mkdir ${em_build_error_seeds} -p
mkdir ${em_runtime_error_seeds} -p
mkdir ${em_build_error_mutant} -p
mkdir ${em_runtime_error_mutant} -p

mkdir ${cheerp_build_error_seeds} -p
mkdir ${cheerp_runtime_error_seeds} -p
mkdir ${cheerp_build_error_mutant} -p
mkdir ${cheerp_runtime_error_mutant} -p


# csmith_seeds=$root/seeds/csmith
csmith_seeds=$root/seeds/crazy_diamond
diff=$root/diff

# for every seed in csmith
for file in ${csmith_seeds}/*
do
  filename=${file##*/}
  cheerp_good_flag=0
  em_good_flag=0
  if [ ${file##*.} = "c" ]; then
    echo "processing [$filename]..."
    make move T=$file
    # run main.sh to get the original deadcode line numbers and output
    bash main.sh random

    # iterate the mutation 3 times
    for i in $(seq 1 4)
    do
      echo "generate the ${i}th mutation..."
      # generate the emi mutation cpp until get a valid one
      flag=0
      for j in $(seq 1 10)
      do
        make emi-mutate
        # check whether the mutation can build successfully, if not, continue generation
        timeout 5 make gcc-O3 T=random.mutate.c
        ret1=$?
        if [ $ret1 -eq 0 ]; then
          flag=1
          break
        fi
      done
      
      if [ $flag -eq 0 ]; then
        echo "can't generate valid mutation for ${filename}!"
        continue
      fi    
      
      timeout 10s make cheerp-old-O3 T=random.c > /dev/null 2>&1
      ret1=$?
      if [ $ret1 -ne 0 ]
      then
        echo "cheerp_build_error_seeds!"
        # move to cheerp error seed
        cp ${file} ${cheerp_build_error_seeds}/${filename}
      else
        # run seed
        timeout 2s v8 random_cheerp.js > seed.cheerp.out 2>&1
        ret2=$?
        if [ $ret2 -ne 0 ]; then
          echo "cheerp_runtime_error_seeds!"
          cp ${file} ${cheerp_runtime_error_seeds}/${filename}
        fi

        timeout 10s make cheerp-old-O3 T=random.mutate.c > /dev/null 2>&1
        ret3=$?
        if [ $ret3 -ne 0 ]
        then
          echo "cheerp_build_error_mutant!"
          mkdir ${cheerp_build_error_mutant}/${filename}.mutant.${i} -p
          cp random.c ${cheerp_build_error_mutant}/${filename}.mutant.${i}/
          cp random.mutate.c ${cheerp_build_error_mutant}/${filename}.mutant.${i}/
        else
          timeout 2s v8 random_cheerp.js > mutate.cheerp.out 2>&1
          ret4=$?
          if [ $ret4 -ne $ret2 ]; then
            echo "cheerp_runtime_error_mutant!"
            mkdir ${cheerp_runtime_error_mutant}/${filename}.mutant.${i} -p
            cp random.c ${cheerp_runtime_error_mutant}/${filename}.mutant.${i}/
            cp random.mutate.c ${cheerp_runtime_error_mutant}/${filename}.mutant.${i}/
          elif [ $ret4 -eq 0 ]; then
            diff seed.cheerp.out mutate.cheerp.out
            if [ $? -eq 1 ]; then
              echo "cheerp_runtime_error_mutant!"
              mkdir ${cheerp_runtime_error_mutant}/${filename}.mutant.${i} -p
              cp random.c ${cheerp_runtime_error_mutant}/${filename}.mutant.${i}/
              cp random.mutate.c ${cheerp_runtime_error_mutant}/${filename}.mutant.${i}/
            else
              cheerp_good_flag=1
            fi
          fi
        fi
      fi


      timeout 10s make em-O3 T=random.c > /dev/null 2>&1
      ret5=$?
      if [ $ret5 -ne 0 ]
      then
        echo "em_build_error_seeds!"
        # move to em error seed
        cp ${file} ${em_build_error_seeds}/${filename}
      else
        timeout 2s v8 random_em.js > seed.em.out 2>&1
        ret6=$?
        if [ $ret6 -ne 0 ]; then
          echo "em_runtime_error_seeds!"
          cp ${file} ${em_runtime_error_seeds}/${filename}
        fi

        timeout 10s make em-O3 T=random.mutate.c > /dev/null 2>&1
        ret7=$?
        if [ $ret7 -ne 0 ]; then
          echo "em_build_error_mutant!"
          mkdir ${em_build_error_mutant}/${filename}.mutant.${i} -p
          cp random.c ${em_build_error_mutant}/${filename}.mutant.${i}/
          cp random.mutate.c ${em_build_error_mutant}/${filename}.mutant.${i}/
        else
          timeout 2s v8 random_em.js > mutate.em.out 2>&1
          ret8=$?
          if [ $ret8 -ne $ret6 ]; then
            echo "em_runtime_error_mutant!"
            mkdir ${em_runtime_error_mutant}/${filename}.mutant.${i} -p
            cp random.c ${em_runtime_error_mutant}/${filename}.mutant.${i}/
            cp random.mutate.c ${em_runtime_error_mutant}/${filename}.mutant.${i}/
          elif [ $ret8 -eq 0 ]; then
            diff seed.em.out mutate.em.out
            if [ $? -eq 1 ]; then
              echo "em_runtime_error_mutant!"
              mkdir ${em_runtime_error_mutant}/${filename}.mutant.${i} -p
              cp random.c ${em_runtime_error_mutant}/${filename}.mutant.${i}/
              cp random.mutate.c ${em_runtime_error_mutant}/${filename}.mutant.${i}/
            else
              em_good_flag=1
            fi
          fi
        fi
      fi

      if [ ${cheerp_good_flag} -eq 1 ] && [ ${em_good_flag} -eq 1 ]; then
        echo "good_mutant!"
        mkdir ${good_mutant}/${filename}.mutant.${i} -p
        cp random.c ${good_mutant}/${filename}.mutant.${i}/
        cp random.mutate.c ${good_mutant}/${filename}.mutant.${i}/
      fi

    done
    

  fi
done

