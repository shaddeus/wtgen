#!/bin/bash

MY_DIR=`dirname $0`
source $MY_DIR/library.sh

echo ""
echo "---------------------------------"
echo "  Web Traffic Generator: CLIENT"
echo "---------------------------------"
echo ""

startTgServer

# Main loop
while (true)
do
    # ----------------------------------
    #   Request for Main Object (HTML)
    # ----------------------------------
    randomLognormal 5.8453551293 0.289342            # Request Size: Mean=360.4, S.D.=106.5
    echo "Request Size $randomLognormalValueInteger Bytes"
    tgRun "$TG_SENDER $IP_SERVER.$PORT arrival 0 length $randomLognormalValueInteger data $randomLognormalValueInteger" &
    
    
    # -------------------------------
    #   Wait for Main Object (HTML)
    # -------------------------------
    echo "Waiting for Main Object (HTML):"
    timeout 10 tcpdump -i any src $IP_SERVER and dst $IP_CLIENT and port $PORT -c 1 &> /dev/null
    if [ $? -ne 0 ]; then
        echo "We did not get a Main Object, so we try it request again"
        continue
    fi

    # ----------------
    #   Parsing Time
    # ----------------
    randomGamma 0.4832851955 0.2689923077          # Parsing Time: Mean=0.13, S.D.=0.187
    echo "Client is parsing Main page for $randomGammaValue Seconds"
    date
    ./microsleep $randomGammaValue
    date


    # -------------------------------
    #   Request for In-Line Objects
    # -------------------------------
    
    # Counting In-Line Objects
    randomGamma 0.2370152355 23.4162162162          # Number of In-Line Objects: Mean=5.55, S.D.=11.4
    numberOfInLineObjects=$randomGammaValueInteger
    echo "Number of requested in-line objects: $numberOfInLineObjects"
    
    # Sending requests for In-Line Objects
    for(( i=0 ; i<$numberOfInLineObjects ; i++ ))
    do
        randomGamma 0.16 5.375                      # In-Line Inter Arrival Time: Mean=0.86, S.D.=2.15
        randomLognormal 5.8453551293 0.289342       # Request Size: Mean=360.4, S.D.=106.5
        tgRun "$TG_SENDER $IP_SERVER.$PORT at $randomGammaValue arrival 0 length $randomLognormalValueInteger data $randomLognormalValueInteger" &
    done


    # ----------------------
    #   Viewing (OFF) Time
    # ----------------------
    randomWeibull 4.67317 43.18939                  # Viewing (OFF) Time, Mean=39.5, S.D.=92.6
    echo "Client is viewing page for $randomWeibullValue Seconds"
    date
    ./microsleep $randomWeibullValue
    date
done

