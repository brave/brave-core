# Docker image for running Claude-driven PR review in CI.
# Uses Anthropic API + gh CLI; skills are mounted from the repo at runtime.
FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    ca-certificates \
    curl \
    git \
    jq \
    python3 \
    python3-pip \
    python3-venv \
    && rm -rf /var/lib/apt/lists/*

# Install GitHub CLI
RUN curl -sSfL https://github.com/cli/cli/releases/download/v2.40.1/gh_2.40.1_linux_amd64.tar.gz | tar xz -C /usr/local --strip-components=1

# Python dependencies for the review runner
RUN pip3 install --no-cache-dir anthropic>=0.39.0

# Working dir when running the review (repo root is mounted at /workspace at run time)
WORKDIR /workspace

# Runner script path and args are passed at run time so the repo mount is used
ENTRYPOINT ["python3"]
