FROM debian:bookworm

ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && \
    apt-get install -y devscripts equivs

COPY . /build
WORKDIR /build
RUN mk-build-deps -i -t 'apt-get -y --no-install-recommends' && \
    dpkg-buildpackage -us -uc -b
