#!/bin/bash

testdir=$(dirname $(readlink -f $BASH_SOURCE))
topdir=$(dirname $testdir)

ubdir=$topdir/pgrapher/experiment/uboone

failed=""
for thing in wcls-sim-drift wcls-sim-drift-simchannel wcls-sim wcls-sim-nf-sp
do
    try="$ubdir/${thing}.jsonnet"
    echo $try
    time jsonnet -J $topdir $try >/dev/null
    if [ "$?" != "0" ] ; then
        echo "failed: $try"
        failed="$failed $try"
    fi
done
if [ -n "$failed" ] ; then
    exit 1
fi
