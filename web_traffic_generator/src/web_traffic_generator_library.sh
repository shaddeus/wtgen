#!/bin/bash

IP_CLIENT="192.168.1.2"
IP_SERVER="192.168.1.3"
PORT="4322"
TG_SENDER="on 2 tcp"

#
# Zajisti, aby TG opravdu zpracovalo parametry
# Tato funkce by se mela vzdy volat s ampersandem na konci pro fork
#
#   tgRun "on 15 tcp ..." &
#
tgRun () {
    echo "Try TG: $1"
    echo $1 | ./tg &> /dev/null
    while [ $? -ne 0 ]    # zkousi to odeslat, dokud se mu to opravdu nepovede
    do
        if [ $? -eq 134 ]; then
            echo "On other side is not running TG Server, we will exit..."
            exit 134
        elif [ $? -eq 255 ]; then
            echo ""
            echo "Try again TG: $1"
            sleep 3
        else
            echo "We got unexpected error no.$? at TG: $1"
            exit $?
        fi
        echo $1 | ./tg &> /dev/null
    done
    echo "Successfully TG: $1"
    exit 0 # this function is forked from script
}

# parametry:
#   1) mean
#   2) standard deviation    
randomLognormal () {
    rgln=`./rg -I $nextInit -D Lognormal -m $1 -s $2 -O "%.5f"`
    randomLognormalValue=`echo $rgln | awk -F' :' '{print $1}'`
    randomLognormalValueInteger=`echo $randomLognormalValue | sed 's/\.[0-9]*$//g'`
    nextInit=`echo $rgln | awk -F' :' '{print $2}'`
}

# parametry:
#   1) alpha
#   2) beta
randomWeibull () {
    rgw=`./rg -I $nextInit -D Weibull -a $1 -b $2 -O "%.5f"`
    randomWeibullValue=`echo $rgw | awk -F' :' '{print $1}'`
    randomWeibullValueInteger=`echo $randomWeibullValue | sed 's/\.[0-9]*$//g'`
    nextInit=`echo $rgw | awk -F' :' '{print $2}'`
}

# parametry:
#   1) alpha
#   2) beta
randomGamma () {
    rgg=`./rg -I $nextInit -D Gamma -a $1 -b $2 -O "%.5f"`
    randomGammaValue=`echo $rgg | awk -F' :' '{print $1}'`
    randomGammaValueInteger=`echo $randomGammaValue | sed 's/\.[0-9]*$//g'`
    nextInit=`echo $rgg | awk -F' :' '{print $2}'`
}

# parametry:
#   1) pravdepodobnost
randomGeometric () {
    rggeom=`./rg -I $nextInit -D Geometric -p $1 -O "%.5f"`
    randomGeometricValue=`echo $rggeom | awk -F' :' '{print $1}'`
    randomGeometricValueInteger=`echo $randomGeometricValue | sed 's/\.[0-9]*$//g'`
    nextInit=`echo $rggeom | awk -F' :' '{print $2}'`
}

# Inicializace nahodneho generatoru
nextInit=`./rg -S 0 | awk -F' :' '{print $2}'`
