# Source: https://gist.githubusercontent.com/indygreg/5608534/raw/30704c59364ce7a8c69a02ee7f1cfb23d1ffcb2c/Dockerfile
# Blog post: https://gregoryszorc.com/blog/2013/05/19/using-docker-to-build-firefox/

# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# Create a build and development environment for Firefox.

FROM ubuntu:16.04
MAINTAINER Gregory Szorc "gps@mozilla.com"

RUN apt-get update && apt-get install -y \
    autoconf2.13 \
    build-essential \
    sudo \
    unzip \
    yasm \
    zip \
    libasound2-dev \
    libcurl4-openssl-dev \
    libdbus-1-dev \
    libdbus-glib-1-dev \
    libgtk2.0-dev \
    libiw-dev \
    libnotify-dev \
    libxt-dev \
    mesa-common-dev \
    uuid-dev \
    binutils-gold \
    bash-completion \
    curl \
    emacs \
    git \
    man-db \
    python-dev \
    python-pip \
    vim
RUN pip install mercurial

RUN useradd -m firefox
RUN hg clone https://hg.mozilla.org/mozilla-central
