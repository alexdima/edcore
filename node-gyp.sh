#!/usr/bin/env bash

# use ./node-gyp.sh configure -- -f xcode
# use make BUILDTYPE=Debug

HOME=~/.electron-gyp
./node_modules/.bin/node-gyp --dist-url=https://atom.io/download/electron --target=1.7.9 --runtime=electron $*
