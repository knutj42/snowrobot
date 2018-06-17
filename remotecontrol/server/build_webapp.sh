#!/usr/bin/env bash
set -e
pushd webapp
npm run build
rm -rf ../controlcenter/controlcenter/static/*
cp -a build/static/*  ../controlcenter/controlcenter/static/
cp build/index.html  ../controlcenter/controlcenter/templates/index.html
popd

