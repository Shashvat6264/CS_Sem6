REQ_HEADERS="Host,Accept,User-Agent,X-Cloud-Trace-Context"

curl -s https://www.example.com -o example.html

curl -s http://ip.jsontest.com | jq -r '.ip'

data=$(curl -s http://header.jsontest.com/)

for n in ${REQ_HEADERS//,/ }; do
    echo $data | jq -r ".\"${n}\""
done

for f in $1/*.json; do
    "$(curl -s -d "json=`cat $f`" -X POST http://validate.jsontest.com/ | jq -r '.validate')" == "true" && echo $f >> valid.txt || echo $f >> invalid.txt
done