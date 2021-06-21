#!/bin/bash

# This will eventually be ensconsed in wcb

# Note, file path and name space patterns used here MUST match what
# are assumed in the Jsonnet and moo templates.

export MOO_LOAD_PATH=$HOME/dev/moo/examples/jsoncpp:$HOME/dev/wct/cfg

set -x
do_one () {
    local Pkg=$1; shift
    local Comp=$1; shift
    
    local pkg="${Pkg,,}"
    local comp="${Comp,,}"
    in="$pkg/schema/${pkg}-${comp}-schema.jsonnet"

    fpath="WireCell${Pkg}/Cfg/${Comp}"
    dpath="WireCell${Pkg}.Cfg.${Comp}"
    outdir="$pkg/inc/$fpath"
    mkdir -p "$outdir"

    moo -g '/lang:ocpp-jsoncpp.jsonnet' \
        -M $HOME/dev/wct/cfg \
        -M $HOME/dev/wct/util/schema \
        -A path="$dpath" \
        -A os="$in" \
        render omodel.jsonnet ostructs.hpp.j2 \
        > $outdir/Structs.hpp

    moo -g '/lang:ocpp-jsoncpp.jsonnet' \
        -M $HOME/dev/wct/cfg \
        -M $HOME/dev/wct/util/schema \
        -A path="$dpath" \
        -A os="$in" \
        render omodel.jsonnet onljs.hpp.j2 \
        > $outdir/Nljs.hpp
}


do_one Util Base
do_one Gen TrackDepos
