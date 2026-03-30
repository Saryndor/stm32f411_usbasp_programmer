FROM debian:trixie-slim

ARG USERNAME=arm-none-eabi
ARG USER_UID
ARG USER_GID

ENV TZ=Europe/Berlin

RUN ln -snf /usr/share/zoneinfo/$TZ /etc/localtime \
    && echo $TZ > /etc/timezone 

   
RUN apt-get update && apt-get install -y --no-install-recommends \
    python3 python3-pip python-is-python3 \
    build-essential \
    wget\
    cmake \
    make \
    ninja-build \
    openocd \
    git \
    openssh-client \
    && rm -rf /var/lib/apt/lists/*


RUN groupadd -g ${USER_GID} ${USERNAME} \
    && useradd -m -u ${USER_UID} -g ${USER_GID} -s /bin/bash ${USERNAME} \
    && mkdir -p /workspace && chown ${USERNAME}:${USERNAME} /workspace 


WORKDIR /opt

RUN mkdir gcc-arm-none-eabi \
    && wget -qO - https://developer.arm.com/-/media/Files/downloads/gnu/15.2.rel1/binrel/arm-gnu-toolchain-15.2.rel1-x86_64-arm-none-eabi.tar.xz \
       | tar -xv --xz -C gcc-arm-none-eabi --strip-components 1
    
ENV PATH=/opt/gcc-arm-none-eabi/bin:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin
    
# Create working directory
WORKDIR /workspace

# Default command
CMD ["/bin/bash"]

