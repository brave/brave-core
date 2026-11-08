# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Builds on Brave's CI systems, by URL: what failed, the log, a rerun.

A URL is a check's link, or a build a report lists. Each system's client is
made once, on first use.
"""

from __future__ import annotations

import functools
import urllib.parse

from ci.actions import ActionsCi
from ci.failures import FailedBuild
from ci.jenkins import JenkinsCi
from ci.teamcity import TeamCityCi

_JENKINS_HOST = 'ci.brave.com'
_TEAMCITY_HOST = 'teamcity.brave.com'

_CLIENTS = {
    'jenkins': JenkinsCi.from_config,
    'teamcity': TeamCityCi,
    'actions': ActionsCi,
}


class Unsupported(ValueError):
    """A URL no system here reads, or an option its system lacks."""


def name(url: str) -> str:
    """`jenkins`, `teamcity` or `actions` (a GitHub Actions job) for the
    systems this package reads, otherwise the URL's host."""
    parsed = urllib.parse.urlparse(url)
    host = parsed.hostname or ''
    if host == 'github.com' and '/actions/runs/' in parsed.path:
        return 'actions'
    return {_JENKINS_HOST: 'jenkins', _TEAMCITY_HOST: 'teamcity'}.get(
        host, host
    )


@functools.cache
def _client(system: str) -> JenkinsCi | TeamCityCi | ActionsCi:
    return _CLIENTS[system]()


def _for(url: str) -> JenkinsCi | TeamCityCi | ActionsCi:
    system = name(url)
    if system not in _CLIENTS:
        raise Unsupported(
            f'Not a Jenkins, TeamCity or GitHub Actions build: {url}'
        )
    return _client(system)


def failed_builds(url: str) -> list[FailedBuild]:
    """What failed in the build at `url`."""
    return _for(url).failed_builds(url)


def full_log(url: str) -> str:
    """The whole log of the build at `url`."""
    return _for(url).full_log(url)


def rerun(url: str, *, wipe_workspace: bool = False) -> str:
    """Queues the build at `url` again, and says so. For TeamCity, an
    aggregate reruns its failed dependencies; for GitHub Actions, the job
    reruns with the jobs that depend on it. Only Jenkins can wipe the
    workspace first."""
    client = _for(url)
    if isinstance(client, JenkinsCi):
        return client.rerun(url, wipe_workspace=wipe_workspace)
    if wipe_workspace:
        raise Unsupported('--wipe-workspace is for Jenkins only.')
    return client.rerun(url)
