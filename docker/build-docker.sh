#!/usr/bin/env bash

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd $DIR/..

DOCKER_IMAGE=${DOCKER_IMAGE:-thegcccoinpay/TheGCCcoind-develop}
DOCKER_TAG=${DOCKER_TAG:-latest}

BUILD_DIR=${BUILD_DIR:-.}

rm docker/bin/*
mkdir docker/bin
cp $BUILD_DIR/src/TheGCCcoind docker/bin/
cp $BUILD_DIR/src/TheGCCcoin-cli docker/bin/
cp $BUILD_DIR/src/TheGCCcoin-tx docker/bin/
strip docker/bin/TheGCCcoind
strip docker/bin/TheGCCcoin-cli
strip docker/bin/TheGCCcoin-tx

docker build --pull -t $DOCKER_IMAGE:$DOCKER_TAG -f docker/Dockerfile docker
