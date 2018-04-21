#!/usr/bin/env bash
set -e
pushd webapp
npm run build
rm -rf ../controlcenter/controlcenter/static/*
cp -a build/static/*  ../controlcenter/controlcenter/static/
cp build/index.html  ../controlcenter/controlcenter/templates/index.html
popd

docker build -t knutj42/robots-controlcenter .
docker push knutj42/robots-controlcenter
