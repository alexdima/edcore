#!/usr/bin/env bash

# use ./node-gyp.sh configure -- -f xcode
# use make BUILDTYPE=Debug

ELECTRON_VERSION=1.7.7

# npm_config_disturl=https://atom.io/download/electron \
# npm_config_target=$ELECTRON_VERSION \
# npm_config_runtime=electron \
HOME=~/.electron-gyp \

./node_modules/.bin/node-gyp --dist-url=https://atom.io/download/electron --target=$ELECTRON_VERSION $*
