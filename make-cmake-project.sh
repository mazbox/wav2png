#!/bin/bash

cmake -Bcmake-build -GNinja -DCPM_SOURCE_CACHE=/tmp -Wno-dev -DBUILD_STANDALONE=ON
cmake --build cmake-build --config Release