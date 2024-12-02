#!/bin/bash

set -xe

git clone https://github.com/microsoft/vcpkg.git --depth 1
./vcpkg/bootstrap-vcpkg.sh

./vcpkg/vcpkg new --application
