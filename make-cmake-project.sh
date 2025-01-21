#!/bin/bash

cmake -Bcmake-build -GNinja -DCPM_SOURCE_CACHE=/tmp -Wno-dev
cmake --build cmake-build --config Release