#!/usr/bin/env sh
set -eu
printf '%s\n' 'Start `make debug-server` in another terminal, then press Enter.'
read dummy
make debug
