#!/bin/bash

MY_DIR=`dirname $0`
source $MY_DIR/web_traffic_generator_library.sh

TMP_PATH="/tmp/web_traffic_generator"
mkdir -p $TMP_PATH

# Pri preruseni vrati false, pri dostani nejakeho paketu true 
short_wait_for_http_request () {
    echo "Short waiting for In-Line Object request:"
    echo "  timeout 10 tcpdump -i any src $IP_CLIENT and dst $IP_SERVER and port $PORT -c 1 &> /dev/null"
    timeout 10 tcpdump -i any src $IP_CLIENT and dst $IP_SERVER and port $PORT -c 1 &> /dev/null
    if [ $? -eq 0 ]; then
        return 1
    else
        return 0
    fi
}

echo ""
echo "---------------------------------"
echo "  Web Traffic Generator: SERVER"
echo "---------------------------------"

tgRun "on 15 tcp 0.0.0.0.$PORT server at 1.1 wait" &
sleep 15
echo "Recieving TG is starting"

# Main loop
while (true)
do
    # ----------------------
    #   Main Object (HTML)
    # ----------------------
    # Waiting for request for Main Object
    echo ""
    echo "Waiting for Main Object request:"
    echo "  tcpdump -i any src $IP_CLIENT and dst $IP_SERVER and port $PORT -c 1 &> /dev/null"
    tcpdump -i any src $IP_CLIENT and dst $IP_SERVER and port $PORT -c 1 &> /dev/null

    # Sending Main Object
    randomLognormal 8.3459005226 1.36604            # Main Object Size: Mean=10710, S.D.=25032
    echo ""
    echo "Transfer Main Object of size $randomLognormalValueInteger Bytes"
    tgRun "$TG_SENDER $IP_CLIENT.$PORT arrival 0 length $randomLognormalValueInteger data $randomLognormalValueInteger" &

    
    # -------------------
    #   In-Line Objects
    # -------------------
    randomGeometric 0.3703703704                    # Number of Cached Web-request: Mean=1.7             
    numberOfCachedInLineObjects=$randomGeometricValueInteger
    echo "Number of cached in-line objects $numberOfCachedInLineObjects"

    randomLognormal 1.8482636612 1.17084            # Number of Non-Cached Web-requests: Mean=12.6, S.D.=21.6
    numberOfNonCachedInLineObjects=$randomLognormalValueInteger
    echo "Number of non-cached in-line objects $numberOfNonCachedInLineObjects"

    numberOfInLineObjectsRequested=0
    while [ short_wait_for_http_request -eq 1 ]
    do
        echo "  We got request for In-Line Object"
        numberOfInLineObjectsRequested=$(($numberOfInLineObjectsRequested+1))
        if [ $numberOfInLineObjectsRequested -le $numberOfNonCachedInLineObjects ]; then
            randomLognormal 6.1657058475 2.36253   # In-Line Object Size: Mean=7758, S.D.=126168
            tgRun "$TG_SENDER $IP_CLIENT.$PORT arrival 0 length $randomLognormalValueInteger data $randomLognormalValueInteger" &
            echo "    Transfer In-Line Object of size $randomLognormalValueInteger Bytes"
        else
            echo "    In-Line Object is cached."
        fi
    done

done

