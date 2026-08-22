# Docker image for running Claude-driven PR review in CI.
# The runner script and review-prs skill doc are COPY'd from the build context (master at
# build time) so PR jobs do not execute arbitrary code from the PR branch with
# secrets. CI passes tokens on stdin to the entrypoint (not via docker -e). Uses Anthropic API + gh CLI.
FROM ubuntu:26.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends ca-certificates curl \
    && curl -fsSL https://cli.github.com/packages/githubcli-archive-keyring.gpg \
        -o /usr/share/keyrings/githubcli-archive-keyring.gpg \
    && chmod go+r /usr/share/keyrings/githubcli-archive-keyring.gpg \
    && echo "deb [arch=$(dpkg --print-architecture) signed-by=/usr/share/keyrings/githubcli-archive-keyring.gpg] https://cli.github.com/packages stable main" \
        >/etc/apt/sources.list.d/github-cli.list \
    && apt-get update && apt-get install -y --no-install-recommends \
        gh \
        git \
        jq \
        python3 \
        python3-pip \
        python3-venv \
    && rm -rf /var/lib/apt/lists/*

# Python deps in a venv (PEP 668 blocks system pip)
RUN python3 -m venv /opt/pr-review-claude/venv \
    && /opt/pr-review-claude/venv/bin/pip install --no-cache-dir "anthropic>=0.39.0"

# Unprivileged user with a real home (avoid world-writable /tmp as $HOME).
RUN useradd --create-home --home-dir /home/prreview --shell /usr/sbin/nologin --uid 10001 prreview \
    && mkdir -p /opt/pr-review-claude/skills/review-prs

# Trusted files from the default branch at image build time (not from PR checkout)
COPY .github/workflows/pr-review-claude.py /opt/pr-review-claude/
COPY agents/skills/review-prs/SKILL.md /opt/pr-review-claude/skills/review-prs/
RUN chmod -R a+rX /opt/pr-review-claude \
    && chown -R prreview:prreview /opt/pr-review-claude /home/prreview

WORKDIR /opt/pr-review-claude
USER prreview
ENV HOME=/home/prreview \
    XDG_CONFIG_HOME=/home/prreview/.config

ENTRYPOINT ["/opt/pr-review-claude/venv/bin/python", "/opt/pr-review-claude/pr-review-claude.py"]
