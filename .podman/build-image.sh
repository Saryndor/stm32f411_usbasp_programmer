#!/usr/bin/bash

podman build \
  --build-arg USER_UID=$(id -u) \
  --build-arg USER_GID=$(id -g) \
  -t f4-usbasp-eabi:15.2 .

