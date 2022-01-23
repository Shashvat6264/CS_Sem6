mkdir files_mod;for f in $1/*;do
    cat $f|awk '{print NR,$0}'|sed 's/\s\+/,/g'>./files_mod/${f##*/};done