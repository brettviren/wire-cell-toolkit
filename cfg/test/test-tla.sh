#!/bin/bash

mydir=$(dirname $(realpath $BASH_SOURCE))


jsonnet --tla-str name="wire cell toolkit" --tla-code port=9999 $mydir/a.jsonnet > a-got.json || exit -1
jsonnet -J $mydir -e 'local got=import "a-got.json"; local want=import "a-want.json"; assert (got == want); got' || exit -1
