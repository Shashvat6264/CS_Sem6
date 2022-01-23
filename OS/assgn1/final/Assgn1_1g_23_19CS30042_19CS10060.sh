for i in {1..150};do
    shuf -i 1-`expr $RANDOM + 10` -n 10|paste -sd ,>>$1;done;cut -d , -f $2 $1|grep -q $3&&echo "YES"||echo "NO"