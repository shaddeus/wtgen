#!/bin/bash

MY_DIR=`dirname $0`
source $MY_DIR/web_traffic_generator_library.sh

echo ""
echo "Web Traffic Generator: CLIENT"
echo ""

tgRun "on 15 tcp 0.0.0.0.$PORT server at 1.1 wait" &
sleep 15
echo "Recieving TG is starting"

# Main loop
while (true)
do
    # ----------------------------------
    #   Request for Main Object (HTML)
    # ----------------------------------
    randomLognormal 5.886805 0.000818831            # Request Size: Mean=360.4, S.D.=106.5
    echo ""
    echo "Request Size $randomLognormalValueInteger Bytes"
    tgRun "$TG_SENDER $IP_SERVER.$PORT arrival 0 length $randomLognormalValueInteger data $randomLognormalValueInteger" &
    
    
    # -------------------------------
    #   Wait for Main Object (HTML)
    # -------------------------------
    echo "Waiting for Main Object (HTML):"
    echo "tcpdump -i any src $IP_SERVER and dst $IP_CLIENT and port $PORT -c 1"
    tcpdump -i any src $IP_SERVER and dst $IP_CLIENT and port $PORT -c 1


    # ----------------
    #   Parsing Time
    # ----------------
    randomGamma 0.0903743316 1.4384615385           # Parsing Time: Mean=0.13, S.D.=0.187
    echo ""
    echo "Client is parsing Main page for $randomGammaValue Seconds"
    date
    ./microsleep $randomGammaValue
    date


    # -------------------------------
    #   Request for In-Line Objects
    # -------------------------------
    
    # Counting In-Line Objects
    randomGamma 2.7019736842 2.0540540541           # Number of In-Line Objects: Mean=5.55, S.D.=11.4
    numberOfInLineObjects=$randomGammaValueInteger
    echo ""
    echo "Number of requested in-line objects: $numberOfInLineObjects"
    
    # Sending requests for In-Line Objects
    for(( i=0 ; i<$numberOfInLineObjects ; i++ ))
    do
        randomGamma 0.344 2.5                       # In-Line Inter Arrival Time: Mean=0.86, S.D.=2.15
        randomLognormal 5.886805 0.000818831        # Request Size: Mean=360.4, S.D.=106.5
        tgRun "$TG_SENDER $IP_SERVER.$PORT at $randomGammaValue arrival 0 length $randomLognormalValueInteger data $randomLognormalValueInteger" &
    done


    # ----------------------
    #   Viewing (OFF) Time
    # ----------------------
    randomWeibull 4.67317 43.18939                  # Viewing (OFF) Time, Mean=39.5, S.D.=92.6
    echo ""
    echo "Client is viewing page for $randomWeibullValue Seconds"
    date
    ./microsleep $randomWeibullValue
    date
done

