f=2;i=$1;while(($i>1));do
    [ $(($i%$f)) == 0 ]&&{ echo -n "$f ";i=$(($i/$f));}||f=$(($f+1));done