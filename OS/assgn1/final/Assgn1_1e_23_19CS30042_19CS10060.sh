VERBOSE=0;[[ $1 == "-v" ]]&&VERBOSE=1;REQ_HEADERS="Host,Accept,User-Agent,X-Cloud-Trace-Context";function log (){
    if [[ $VERBOSE -eq 1 ]];then
        echo "$@";fi;}
curl -s https://www.example.com -o example.html;log "First GET successful";curl -s http://ip.jsontest.com|jq -r '.ip';log "Second GET successful";data=$(curl -s http://header.jsontest.com/);log "Third GET successful";for n in ${REQ_HEADERS//,/ };do
    echo $data|jq -r ".\"${n}\"";done;for f in $1/*.json;do
    "$(curl -s -d "json=`cat $f`" -X POST http://validate.jsontest.com/|jq -r '.validate')" == "true"&&echo $f>>valid.txt||echo $f>>invalid.txt;done
