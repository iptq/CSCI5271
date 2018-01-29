#!/bin/bash

dpkg --add-architecture i386

#apt-get update
apt-get install -y \
    gcc-multilib \
    libncursesw5 libncursesw5-dev \
    libncursesw5:i386 libncursesw5-dev:i386
