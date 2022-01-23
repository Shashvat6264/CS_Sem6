cut -d ' ' -f $2 $1|awk '{print tolower($0)}'|sort|uniq -c|sort -rn|awk '{print $2,$1}'>1e_output_$2_column.freq
