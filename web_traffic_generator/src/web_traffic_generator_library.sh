#!/bin/bash

IP_CLIENT="192.168.1.2"
IP_SERVER="192.158.1.3"
PORT="4322"
TG_SENDER="on 15 tcp"

#
# Zajisti, aby TG opravdu zpracovalo parametry
# Tato funkce by se mela vzdy volat s ampersandem na konci pro fork
#
#   tgRun "on 15 tcp ..." &
#
tgRun () {
    echo ""
    echo "Try TG: $1"
    while ( ! (echo "$1" | ./tg -f &> /dev/null) )    # zkousi to odeslat, dokud se mu to opravdu nepovede
    do
        if [ $? -eq 134 ]; then
            echo "On other side is not running TG Server, we will exit..."
            exit 134
        elif [ $? -eq 255 ]; then
            echo ""
            echo "Try again TG: $1"
            sleep 3
        else
            echo "We got unexpected error no.$?"
            exit $?
        fi
    done
    echo ""
    echo "Successfully execute via TG: $1"
    exit 0 # this function is forked from script
}

# parametry:
#   1) mean
#   2) standard deviation    
randomLognormal () {
    rgln=`./rg -I $nextInit -D Lognormal -m $1 -s $2`
    randomLognormalValue=`echo $rgln | awk -F' :' '{print $1}'`
    randomLognormalValueInteger=`echo $randomLognormalValue | sed 's/\.[0-9]*$//g'`
    nextInit=`echo $rgln | awk -F' :' '{print $2}'`
}

# parametry:
#   1) alpha
#   2) beta
randomWeibull () {
    rgw=`./rg -I $nextInit -D Weibull -a $1 -b $2`
    randomWeibullValue=`echo $rgw | awk -F' :' '{print $1}'`
    randomWeibullValueInteger=`echo $randomWeibullValue | sed 's/\.[0-9]*$//g'`
    nextInit=`echo $rgw | awk -F' :' '{print $2}'`
}

# parametry:
#   1) alpha (shape ~ mean)
#   2) beta (scale ~ SD)
randomGamma () {
    rgg=`./rg -I $nextInit -D Gamma -a $1 -b $2`
    randomGammaValue=`echo $rgg | awk -F' :' '{print $1}'`
    randomGammaValueInteger=`echo $randomGammaValue | sed 's/\.[0-9]*$//g'`
    nextInit=`echo $rgg | awk -F' :' '{print $2}'`
}

# parametry:
#   1) pravdepodobnost
randomGeometric () {
    rggeom=`./rg -I $nextInit -D Geometric -p $1`
    randomGeometricValue=`echo $rggeom | awk -F' :' '{print $1}'`
    randomGeometricValueInteger=`echo $randomGeometricValue | sed 's/\.[0-9]*$//g'`
    nextInit=`echo $rggeom | awk -F' :' '{print $2}'`
}

# Inicializace nahodneho generatoru
nextInit=`./rg -S 0 | awk -F' :' '{print $2}'`
