#!/usr/bin/env bash

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd $DIR/..

DOCKER_IMAGE=${DOCKER_IMAGE:-keplerpay/keplerd-develop}
DOCKER_TAG=${DOCKER_TAG:-latest}

BUILD_DIR=${BUILD_DIR:-.}

rm docker/bin/*
mkdir docker/bin
cp $BUILD_DIR/src/keplerd docker/bin/
cp $BUILD_DIR/src/kepler-cli docker/bin/
cp $BUILD_DIR/src/kepler-tx docker/bin/
strip docker/bin/keplerd
strip docker/bin/kepler-cli
strip docker/bin/kepler-tx

docker build --pull -t $DOCKER_IMAGE:$DOCKER_TAG -f docker/Dockerfile docker
