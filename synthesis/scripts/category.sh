diff_dir=/tmp/CrazyDiamond/diff/
exp=exp15
result=/tmp/experiment/$exp/result

clang_error=$result/clang_error
cheerp_error=$result/cheerp_error
em_error=$result/em_error
cheerp_error_O0=$result/cheerp_error_O0
em_error_O0=$result/em_error_O0

timeout=$result/timeout
seg=$result/seg
unstable=$result/unstable
bad_case=$result/bad_case


em_diff=$result/em_diff
cheerp_diff=$result/cheerp_diff
em_diff_O0=$result/em_diff_O0
cheerp_diff_O0=$result/cheerp_diff_O0
same=$result/same

sum=0

mkdir $clang_error -p
mkdir $cheerp_error -p
mkdir $em_error_O0 -p
mkdir $cheerp_error_O0 -p
mkdir $em_error -p

mkdir $timeout -p
mkdir $seg -p
mkdir $unstable -p
mkdir $bad_case -p


mkdir $em_diff -p
mkdir $cheerp_diff -p
mkdir $em_diff_O0 -p
mkdir $cheerp_diff_O0 -p
mkdir $same -p

i=103

for i in $(seq 104 108)
do
    # {
        dir=/tmp/experiment/$exp/$i/fuzz/
        diff_dir_i=/tmp/experiment/$exp/$i/diff/
        mkdir $diff_dir_i -p
        cp $diff_dir/makefile $diff_dir_i
        echo "========================== $dir ========================="
        for file in $dir/*
        do
            # sleep 1
            filename=${file##*/}
            if [ ${file##*.} = "c" ]; then
                # echo "===================================================="
                echo "processing [$i-$filename]..."
                if [ ! -s $file ]; then
                    echo "$file is null"
                    rm $file
                    continue
                fi
                # if [ $sum -gt 200 ]
                #     then break
                # fi
                sum=`expr $sum + 1`
                cp $file $diff_dir_i/random.c
                cd $diff_dir_i
                
                
                
                
                make c > /dev/null 2>&1
                ret1=$?
                if [ $ret1 -ne 0 ]; then
                    echo "clang build error!"
                    cp random.c $clang_error/$i-$filename
                    
                    continue
                fi
                timeout 2s ./random.o > clang.out 2>&1
                ret=$?
                if [ $ret -eq 124 ]; then
                    echo "Timeout !"
                    cp random.c $timeout/$i-$filename
                    
                    continue
                elif [ $ret -ne 0 ]; then
                    echo "OS Error,  e.g. segmentation fault, zero division... !"
                    cp random.c $seg/$i-$filename
                    
                    continue
                else
                    
                    timeout 2s ./random.gcc.o > gcc.out 2>&1
                    diff clang.out gcc.out
                    if [ $? -eq 1 ];
                        then echo "Unstable !"
                        cp random.c $unstable/$i-$filename
                        continue
                    fi
                fi

                echo "GCC Run Succeed !"
                # test cheerp
                timeout 10s make cheerp-old > /dev/null 2>&1
                ret1=$?
                if [ $ret1 -ne 0 ]; then
                    echo "cheerp build error!"
                    cp random.c $cheerp_error/$i-$filename
                    continue
                fi
                timeout 10s make cheerp-old-O0 > /dev/null 2>&1
                ret1=$?
                if [ $ret1 -ne 0 ]; then
                    echo "cheerp O0 build error!"
                    cp random.c $cheerp_error_O0/$i-$filename
                    continue
                fi
                timeout 2s v8 random_cheerp.js > cheerp.out 2>&1 
                ret=$?
                if [ $ret -ne 0 ]; then
                    echo "OS Error,  e.g. segmentation fault, zero division... !"
                    cp random.c $bad_case/$i-$filename
                    continue
                fi
                timeout 2s v8 random_cheerp_O0.js > cheerp_O0.out 2>&1 
                ret=$?
                if [ $ret -ne 0 ]; then
                    echo "O0: OS Error,  e.g. segmentation fault, zero division... !"
                    cp random.c $bad_case/$i-$filename
                    continue
                fi
                echo "Cheerp Run Succeed !"

                # test Em
                timeout 10s make em > /dev/null 2>&1
                ret1=$?
                if [ $ret1 -ne 0 ]; then
                    echo "em build error!"
                    cp random.c $em_error/$i-$filename
                    continue
                fi
                timeout 10s make em-O0 > /dev/null 2>&1
                ret1=$?
                if [ $ret1 -ne 0 ]; then
                    echo "em build error!"
                    cp random.c $em_error_O0/$i-$filename
                    continue
                fi
                timeout 10s v8 random_em.js > em.out  2>&1
                ret=$?
                if [ $ret -ne 0 ]; then
                    echo "OS Error,  e.g. segmentation fault, zero division... !"
                    cp random.c $bad_case/$i-$filename
                    continue
                fi
                timeout 2s v8 random_em_O0.js > em_O0.out 2>&1 
                ret=$?
                if [ $ret -ne 0 ]; then
                    echo "O0: OS Error,  e.g. segmentation fault, zero division... !"
                    cp random.c $bad_case/$i-$filename
                    continue
                fi
                echo "Em Run Succeed !"
                
                echo "==================== Good Case ! ===================="
                has_diff=0
                diff clang.out em.out
                if [ $? -eq 1 ]; then
                    echo "GCC and EM have diff !"
                    cp random.c $em_diff/$i-$filename
                    has_diff=1
                fi
                diff clang.out em_O0.out
                if [ $? -eq 1 ]; then
                    echo "GCC and EM O0 have diff !"
                    cp random.c $em_diff_O0/$i-$filename
                    has_diff=1
                fi
                diff clang.out cheerp.out
                if [ $? -eq 1 ]; then
                    echo "GCC and CHEERP have diff !"
                    cp random.c $cheerp_diff/$i-$filename
                    has_diff=1
                fi
                diff clang.out cheerp_O0.out
                if [ $? -eq 1 ]; then
                    echo "GCC and CHEERP_O0 have diff !"
                    cp random.c $cheerp_diff_O0/$i-$filename
                    has_diff=1
                fi
                if [ $has_diff -eq 0 ]; then
                    cp random.c $same/$i-$filename
                fi
                # break
            fi
        done
        echo "$i done!"
    # }&
done