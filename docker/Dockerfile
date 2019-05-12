FROM debian:stretch
LABEL maintainer="Kepler Developers <dev@kepler.org>"
LABEL description="Dockerised KeplerCore, built from Travis"

RUN apt-get update && apt-get -y upgrade && apt-get clean && rm -fr /var/cache/apt/*

COPY bin/* /usr/bin/
