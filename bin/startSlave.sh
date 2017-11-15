# param 1: [master hostname]:[master port], e.g., lenss-comp1.cse.tamu.edu:10009
# param 2: [slave port], e.g., 10010
mHost="lenss-comp1.cse.tamu.edu:10009"
cPort="10010"
if [ ! -z "$1" ]
  then
    mHost=$1
fi
if [ ! -z "$1" ]
  then
    cPort=$2
fi
killall fbsd
# run slave chat server
../fbsd -p $cPort -m $mHost
