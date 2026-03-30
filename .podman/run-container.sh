#!/usr/bin/bash

podman run -d \
       --name c-f4-usbasp-eabi \
       --userns=keep-id \
       -v "$PWD"/..:/workspace \
       -v /dev/bus/usb:/dev/bus/usb \
       -w /workspace \
       --mount type=bind,source=/run/user/$UID/keyring/ssh,target=/run/host-services/ssh-auth.sock \
       -e SSH_AUTH_SOCK=/run/host-services/ssh-auth.sock \
       --add-host=host.containers.internal:host-gateway \
       f4-usbasp-eabi:15.2 sleep infinity

