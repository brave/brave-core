# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""What failed on a pull request, across the CI systems that ran its checks."""

from __future__ import annotations

from collections.abc import Callable

from ci import github
from ci.failures import CheckFailures, FailedBuild
from ci.jenkins import JenkinsCi
from ci.teamcity import TeamCityCi, build_id


def collect(
    checks: github.PullRequestChecks,
    *,
    jenkins: Callable[[], JenkinsCi] = JenkinsCi.from_config,
    teamcity: Callable[[], TeamCityCi] = TeamCityCi,
) -> list[CheckFailures]:
    """The failed builds behind each of `checks`' failed checks.

    A check whose builds can't be read (no credentials, a CI system this
    package doesn't read) is still listed, with why.
    """
    clients: dict[str, JenkinsCi | TeamCityCi] = {}
    result = []
    for check in checks.in_state(github.State.FAILURE):
        builds: list[FailedBuild] = []
        error = ''
        try:
            if check.system == 'teamcity':
                client = clients.setdefault('teamcity', teamcity())
                builds = client.failed_builds(build_id(check.url))
            elif check.system == 'jenkins':
                client = clients.setdefault('jenkins', jenkins())
                builds = client.failed_builds(check.url)
            else:
                error = f'{check.system or "unknown"} checks are not read'
        except Exception as e:  # pylint: disable=broad-exception-caught
            # One unreadable check shouldn't hide the others.
            error = f'{type(e).__name__}: {e}'
        result.append(
            CheckFailures(
                check=check.name,
                url=check.url,
                system=check.system,
                builds=tuple(builds),
                error=error,
            )
        )
    return result
