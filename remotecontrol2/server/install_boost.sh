#!/usr/bin/env bash
set -xeuf -o pipefail

curl --location -o boost_1_84_0.tar.gz https://archives.boost.io/release/1.84.0/source/boost_1_84_0.tar.gz
tar xzf boost_1_84_0.tar.gz
pushd boost_1_84_0
./bootstrap.sh
sudo ./b2 cxxflags=-std=c++20 install -j $(nproc)
popd
