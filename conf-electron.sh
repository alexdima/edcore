#!/bin/bash

ELECTRON_VERSION=1.6.6
ELECTRON_GYP_HOME=~/.electron-gyp
mkdir -p $ELECTRON_GYP_HOME

npm_config_disturl=https://atom.io/download/electron \
npm_config_target=$ELECTRON_VERSION \
npm_config_runtime=electron \
HOME=$ELECTRON_GYP_HOME \
npm $*