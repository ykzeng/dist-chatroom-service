# param 1: [master/local hostname], e.g., lenss-comp1.cse.tamu.edu
# param 2: [master/local port], e.g., 10009
# param 3: [slave port], e.g., 10010
mHostname="lenss-comp1.cse.tamu.edu"
mPort="10009"
cPort="10010"

if [ ! -z "$1" ]
  then
    mHostname=$1
fi
if [ ! -z "$2" ]
  then
    mPort=$2
fi
if [ ! -z "$3" ]
  then
    cPort=$3
fi
killall fbsd
# run master server
../fbsd -p $mPort &
# run chat server
../fbsd -p $cPort -m "$mHostname:$mPort" &
