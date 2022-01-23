mkdir 1.b.files.out;for f in 1.b.files/*;do 
  sort $f -n>1.b.files.out/${f##*/};done;cat 1.b.files.out/*|sort -n|uniq -c|awk '{print $2,$1}'>1.b.out.txt
