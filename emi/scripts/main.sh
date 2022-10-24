file_name=$1
rm $file_name.gcda
rm $file_name.gcno
rm $file_name.c.gcov
rm $file_name.txt
rm ${file_name}_out.txt
rm ${file_name}_coverage.txt
rm $file_name.gcda
gcc -O0 -fprofile-arcs -ftest-coverage -w -I /tmp/emi/include $file_name.c -o a.out
./a.out >> ${file_name}_out.txt
gcov $file_name.c
cat $file_name.c.gcov | grep "#####" >> $file_name.txt
while read -r line; do
    # echo $line
    tmp=${line#*:}
    tmp2=$(echo $tmp | grep :)
    while [ "$tmp2" != "" ]
    do
        tmp=${tmp%:*}
        tmp2=$(echo $tmp | grep :)
    done
    echo $tmp >> ${file_name}_coverage.txt
done < $file_name.txt
