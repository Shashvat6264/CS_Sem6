#!/bin/bash
for f in $(find $1 -type f);do
    mkdir -p $([[ ! $f == ${f##*.} ]]&&echo "${f##*.}"||echo "Nill")&&find $1 -type f -name "*.$_" -exec mv "{}" $_ \;
done
