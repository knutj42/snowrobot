#!/usr/bin/env bash

# this scripts opens a http tunnel to the phones so that we can run the server locally

set -x

# note3
adb -s bf5bb477	reverse tcp:16000 tcp:16000

# note8
adb -s ce061716d349c31d037e	reverse tcp:16000 tcp:16000
