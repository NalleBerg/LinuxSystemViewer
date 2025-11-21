#!/bin/bash
# Script to run LSV with accessibility bus warning suppressed
export NO_AT_BRIDGE=1
cd "$(dirname "$0")"
./build/LSV "$@"