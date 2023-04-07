#!/usr/bin/env bats

bats_load_library wct-bats.sh

@test "test_idft with torchdft" {
    usepkg aux                  # to get test_dft
    run test_idft cuFftDFT WireCellCuda
    echo "$output"
    [[ "$status" -eq 0 ]]
}

