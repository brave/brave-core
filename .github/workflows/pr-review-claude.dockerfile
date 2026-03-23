# Docker image for running Claude-driven PR review in CI.
# The runner script and review skill are COPY'd from the build context (master at
# build time) so PR jobs do not execute arbitrary code from the PR branch with
# secrets. Uses Anthropic API + gh CLI.
FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    ca-certificates \
    curl \
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

# Python dependencies for the review runner
RUN pip3 install --no-cache-dir anthropic>=0.39.0

# Trusted files from the default branch at image build time (not from PR checkout)
COPY .github/workflows/pr-review-claude.py /opt/pr-review-claude/pr-review-claude.py
COPY .claude/skills/review/SKILL.md /opt/pr-review-claude/skills/review/SKILL.md
RUN chmod -R a+rX /opt/pr-review-claude

# Run as a non-root user already present in the image (satisfies linters; no useradd).
WORKDIR /opt/pr-review-claude

USER nobody

ENTRYPOINT ["python3", "/opt/pr-review-claude/pr-review-claude.py"]
