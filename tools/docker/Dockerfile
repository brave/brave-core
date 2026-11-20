# syntax=docker/dockerfile:1
# check=skip=SecretsUsedInArgOrEnv
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

# Shared Linux x64 checkout and build dependencies; refresh for new Chromium majors.
# https://github.com/brave/brave-browser/wiki/Linux-Development-Environment
FROM ubuntu:24.04 AS checkout
ARG DEBIAN_FRONTEND=noninteractive
ARG BUILD_UID=1001
ARG BUILD_GID=1001
ARG AST_GREP_VERSION=0.51.3

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential ca-certificates curl file fuse lsb-release \
    openssh-client python3 python-is-python3 ripgrep software-properties-common sudo unzip xz-utils \
    && add-apt-repository --yes ppa:git-core/ppa \
    && apt-get update && apt-get install -y --no-install-recommends git \
    && git --version \
    && dpkg --compare-versions "$(git --version | cut -d ' ' -f 3)" ge 2.46.0

RUN if ! getent group "${BUILD_GID}" >/dev/null; then \
      groupadd --gid "${BUILD_GID}" builder; \
    fi \
    && useradd --non-unique --uid "${BUILD_UID}" --gid "${BUILD_GID}" \
       --create-home --home-dir /home/builder --shell /bin/bash builder \
    && install -d -o "${BUILD_UID}" -g "${BUILD_GID}" /workspace

# Credential settings contain paths and a helper mode; credentials are runtime mounts.
ENV LANG=en_US.UTF-8 \
    RBE_service=rbe.ba.brave.com:443 \
    RBE_tls_client_auth_cert=/run/brave-rbe/engflow.crt \
    RBE_tls_client_auth_key=/run/brave-rbe/engflow.key \
    SISO_CREDENTIAL_HELPER=mTLS

COPY --chmod=755 <<'ENTRYPOINT' /usr/local/bin/brave-container-entrypoint
#!/bin/bash
set -euo pipefail
sdk=/workspace/src/third_party/depot_tools/win_toolchain/vs_files
# The bootstrap image has no SDK yet; init hooks create its first mount.
if [[ -d "${sdk}.ciopfs" ]] && ! mountpoint -q "$sdk"; then
  mkdir -p "$sdk"
  /workspace/src/build/ciopfs -o use_ino "${sdk}.ciopfs" "$sdk"
fi
exec "$@"
ENTRYPOINT

COPY --chmod=755 <<'BUILD' /usr/local/bin/brave-build
#!/bin/bash
set -euo pipefail
test -r "$RBE_tls_client_auth_cert"
test -r "$RBE_tls_client_auth_key"
exec pnpm run build "$@"
BUILD

COPY --chmod=755 <<'SYNC' /usr/local/bin/brave-linux-build
#!/bin/bash
set -euo pipefail
cd /workspace/src/brave
if [[ ${1:-} == --sync ]]; then
  exec pnpm run sync
fi
exec brave-build "$@"
SYNC

ARG BRAVE_HEAD
LABEL io.brave.checkout.format="6" io.brave.checkout.head="${BRAVE_HEAD}"
USER builder
WORKDIR /workspace
RUN git clone https://github.com/brave/brave-core.git src/brave \
    && git -C src/brave checkout --detach "${BRAVE_HEAD}"

WORKDIR /workspace/src/brave
COPY create_container.py build_brave_image.py /usr/local/lib/brave-docker/
USER root
RUN python3 /usr/local/lib/brave-docker/create_container.py --install-toolchain \
    && cd / \
    && npm install --global --prefix /usr/local "@ast-grep/cli@${AST_GREP_VERSION}"
USER builder
RUN <<'CONFIG'
cat > .env <<EOF
use_remoteexec=true
rbe_service=${RBE_service}
rbe_tls_client_auth_cert=${RBE_tls_client_auth_cert}
rbe_tls_client_auth_key=${RBE_tls_client_auth_key}
siso_cache_dir=/home/builder/.cache/brave/siso
quiet=false
EOF
CONFIG
# The image launcher synchronizes and builds this stage in a credential-mounted container.
ENTRYPOINT ["/usr/local/bin/brave-container-entrypoint"]
CMD ["sleep", "infinity"]

FROM checkout AS checkout-cached
# Enable the Git cache only when the image launcher supplies one.
COPY --from=git_cache --chown=${BUILD_UID}:${BUILD_GID} / /home/builder/.cache/brave/git/
RUN echo 'git_cache_path=/home/builder/.cache/brave/git' >> .env
