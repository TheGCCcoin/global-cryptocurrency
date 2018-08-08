FROM debian:stretch
LABEL maintainer="GCC Developers <dev@TheGCCcoin.org>"
LABEL description="Dockerised GCCCore, built from Travis"

RUN apt-get update && apt-get -y upgrade && apt-get clean && rm -fr /var/cache/apt/*

COPY bin/* /usr/bin/
