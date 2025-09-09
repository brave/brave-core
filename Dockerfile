# syntax=docker/dockerfile:1.4
FROM ubuntu:22.04 AS chromium-build-base

ENV DEBIAN_FRONTEND=noninteractive
ARG USER=ubuntu
ARG UID=1000
ARG GID=1000

# install-build-deps.sh --no-prompt installs keyboard-configuration which prompts the user to select the keyboard
RUN --mount=type=cache,target=/var/cache/apt \
    echo "keyboard-configuration keyboard-configuration/layoutcode string us" | debconf-set-selections && \
    echo "keyboard-configuration keyboard-configuration/modelcode string pc105" | debconf-set-selections && \
    apt update && \
    apt install -y keyboard-configuration \
        git ssh openssh-client curl lsb-release sudo build-essential python3-setuptools python3-distutils-extra


ARG CHROMIUM_REF=main

# We frontload the install script to ensure we have installed everything we need without needing to fetch everything and we can go non-root
RUN curl https://chromium.googlesource.com/chromium/src/+/$CHROMIUM_REF/build/install-build-deps.py\?format\=text | base64 -d > install-build-deps.py
RUN curl https://chromium.googlesource.com/chromium/src/+/$CHROMIUM_REF/build/install-build-deps.sh\?format\=text | base64 -d > install-build-deps.sh
RUN --mount=type=cache,target=/var/cache/apt \
    chmod +x ./install-build-deps.sh ./install-build-deps.py && \
    ./install-build-deps.sh --no-prompt

# From here on we are not root anymore
# NOTE this won't be needed in ubuntu 24.04 
RUN useradd -rm -d /home/ubuntu -s /bin/bash -g root -G sudo -u $UID $USER
WORKDIR /home/$USER
USER $USER

# Setup bash, so we can access nvm and other tools we choose to install
SHELL ["/usr/bin/bash", "-l", "-c"]
ENV BASH_ENV=/home/$USER/.bash_profile
ENV PROFILE=$BASH_ENV
RUN touch "$BASH_ENV" && echo '. "$BASH_ENV"' >> ~/.bashrc

# install node and npm
ENV NVM_DIR=/home/$USER/.nvm
RUN mkdir -p $NVM_DIR && \
    curl https://raw.githubusercontent.com/nvm-sh/nvm/v0.40.3/install.sh | bash

ENV NODE_VERSION=24.5.0
RUN source $NVM_DIR/nvm.sh && \
    nvm install $NODE_VERSION && \
    nvm alias default $NODE_VERSION

RUN git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git /home/ubuntu/depot_tools
ENV PATH=/home/ubuntu/depot_tools:$PATH
ENV GIT_CACHE_PATH=/home/ubuntu/.cache/git


RUN git config --global user.name "docker" \
    git config --global user.email "docker@email.com" \
    git config --global core.autocrlf false \
    git config --global core.filemode false \
    git config --global cache.cachepath $GIT_CACHE_PATH && \
    git config --global checkout.workers 16

# ---------------------------------------------------------------------------------
# Setup a minimal image so you can build brave without needing to download anything
# You can volume mount your sourcecode onto /home/ubuntu/src/brave
FROM chromium-build-base AS brave-dev

ADD .gclient .gclient

# RUN echo "build_type=$BUILD_TYPE --target_os=$TARGET_OS --target_arch=$TARGET_ARCH" 
# RUN echo projects_chrome_repository_url=https://chromium.googlesource.com/chromium/src > .env


# Setup git cache for gclient; we give the cache-mount a dedicated name uid
# This folder will only exist in the cache to ensure we don't bloat the image
# We enable sharing to maximize reuse in a matrix build.
# The id is required to ensure gclient doesn't hang on macos
RUN --mount=type=cache,id=git-cache,uid=$UID,sharing=locked,target=$GIT_CACHE_PATH \
    mkdir -p $GIT_CACHE_PATH && \
    rm -rf $GIT_CACHE_PATH/*.locked && \
    ls $GIT_CACHE_PATH && \
    gclient sync --no-history \
                --revision src@140.0.7339.80 \
                --revision src/brave@master

# --------------------------------------------------------------------------------------
# Prebuilt image for faster incremental builds
# you can use secret mounts to use your env file and rbe certs
# The certificates won't become part of the docker image but
# but some env secrets might end up args.gn files of the build
# docker buildx bake --secret id=env,target=/path/to/file
# To run tests - unless target platform is not linux - 
#    you will need to copy out the test binary and test data onto the host
# eg. docker cp brave-dev-prebuilt-container:/home/ubuntu/src/out/brave-browser-test .
FROM brave-dev AS brave-dev-prebuilt
WORKDIR /home/ubuntu/src/brave
ARG BUILD_TYPE=Static

RUN --mount=type=secret,id=env,optional=true \
    --mount=type=secret,id=rbe,optional=true \
    npm run build       -- $BUILD_TYPE  --target_os=$TARGET_OS --target_arch=$TARGET_ARCH

RUN --mount=type=secret,id=env,optional=true \
    --mount=type=secret,id=rbe,optional=true \
    npm run create_dist -- $BUILD_TYPE  --target_os=$TARGET_OS --target_arch=$TARGET_ARCH