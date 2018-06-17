#!/usr/bin/env bash
set -e
./build_webapp.sh
docker build -t knutj42/robots-controlcenter .
docker push knutj42/robots-controlcenter
