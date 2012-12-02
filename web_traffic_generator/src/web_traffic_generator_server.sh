#!/bin/bash

MY_DIR=`dirname $0`
source $MY_DIR/web_traffic_generator_library.sh

TMP_PATH="/tmp/web_traffic_generator"
mkdir -p $TMP_PATH

# Pri preruseni vrati false, pri dostani nejakeho paketu true 
short_wait_for_http_request () {
    echo "Short waiting for In-Line Object request"
    ./timelimit -t 3 -T 4 tcpdump -i any src $IP_CLIENT and dst $IP_SERVER and port $PORT -c 1
    if [ $? -eq 0 ]; then
        return true
    else
        return false
    fi
}

echo ""
echo "Web Traffic Generator: SERVER"
echo ""

tgRun "on 5 tcp $IP_SERVER.$PORT server at 1.1 wait" &
echo "Recieving TG is starting"

# Main loop
while (true)
do
    # ----------------------
    #   Main Object (HTML)
    # ----------------------
    # Waiting for request for Main Object
    echo ""
    echo "Waiting for Main Object request"
    tcpdump -i any src $IP_CLIENT and dst $IP_SERVER and port $PORT -c 1

    # Sending Main Object
    randomLognormal 9.2788240598 0.0002182073       # Main Object Size: Mean=10710, S.D.=25032
    echo ""
    echo "Transfer Main Object of size $randomLognormalValueInteger Bytes"
    tgRun "$TG_SENDER arrival 0 length $randomLognormalValueInteger data $randomLognormalValueInteger" &

    
    # -------------------
    #   In-Line Objects
    # -------------------
    randomGeometric 0.3703703704                    # Number of Cached Web-request: Mean=1.7             
    numberOfCachedInLineObjects=$randomGeometricValueInteger
    echo "Number of cached in-line objects $numberOfCachedInLineObjects"

    randomLognormal 2.4699162011 0.1275612256       # Number of Non-Cached Web-requests: Mean=12.6, S.D.=21.6
    numberOfNonCachedInLineObjects=$randomLognormalValueInteger
    echo "Number of non-cached in-line objects $numberOfNonCachedInLineObjects"

    numberOfInLineObjectsRequested=0
    while [ short_wait_for_http_request ]
    do
        echo "  We got request for In-Line Object"
        numberOfInLineObjectsRequested=$(($numberOfInLineObjectsRequested+1))
        if [ $numberOfInLineObjectsRequested -le $numberOfNonCachedInLineObjects ]; then
            randomLognormal 8.9554328042 0.0020940877   # In-Line Object Size: Mean=7758, S.D.=126168
            tgRun "$TG_SENDER arrival 0 length $randomLognormalValueInteger data $randomLognormalValueInteger" &
            echo "    Transfer In-Line Object of size $randomLognormalValueInteger Bytes"
        else
            echo "    In-Line Object is cached."
        fi
    done

done
