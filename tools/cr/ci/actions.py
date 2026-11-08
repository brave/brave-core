# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Reads failed GitHub Actions jobs, and reruns them, through `gh api`."""

from __future__ import annotations

import json
import re

from ci import failures
from ci.failures import FailedBuild
from gh_cli import GhCli

# A job's page: `github.com/<owner>/<repo>/actions/runs/<run>/job/<job>`.
_JOB = re.compile(r'github\.com/([^/]+/[^/]+)/actions/runs/\d+/job/(\d+)')

# The timestamp the runner puts before each log line.
_TIMESTAMP = re.compile(r'^\d{4}-\d\d-\d\dT[\d:.]+Z ', re.MULTILINE)
# The byte order mark the log starts with.
_BOM = '\N{ZERO WIDTH NO-BREAK SPACE}'

# Step conclusions that fail a job.
_FAILED = ('failure', 'timed_out', 'cancelled')


class ActionsCi:
    def __init__(self, gh: GhCli | None = None) -> None:
        self._gh = gh or GhCli()

    @staticmethod
    def _job(url: str) -> tuple[str, int]:
        """The repository and job id of the job at `url`."""
        match = _JOB.search(url)
        if not match:
            raise ValueError(f'Not a GitHub Actions job URL: {url}')
        return match[1], int(match[2])

    def failed_builds(self, url: str) -> list[FailedBuild]:
        """The job at `url`, with its failed steps and the end of its log up
        to the first error."""
        repo, job_id = self._job(url)
        job = json.loads(self._gh.api(f'repos/{repo}/actions/jobs/{job_id}'))
        steps = [
            step
            for step in job.get('steps') or []
            if step.get('conclusion') in _FAILED
        ]
        problems = tuple(
            f'Step {step.get("number")}, {step.get("name")}: '
            f'{step.get("conclusion")}'
            for step in steps
        )
        log = self.full_log(url, timestamps=False)
        log_tail = failures.with_fatal_line(log, self._error_tail(log))
        summary = job.get('conclusion') or job.get('status') or ''
        if steps:
            summary += f' in {", ".join(step["name"] for step in steps)}'
        return [
            FailedBuild(
                name=job.get('name', str(job_id)),
                url=job.get('html_url', url),
                summary=summary,
                problems=problems,
                log_tail=log_tail,
            )
        ]

    def full_log(self, url: str, *, timestamps: bool = True) -> str:
        """The whole log of the job at `url`, without the runner's timestamps
        if asked."""
        repo, job_id = self._job(url)
        log = self._gh.api(f'repos/{repo}/actions/jobs/{job_id}/logs')
        log = log.removeprefix(_BOM)
        return log if timestamps else _TIMESTAMP.sub('', log)

    @staticmethod
    def _error_tail(log: str) -> str:
        """The log's lines up to its first `##[error]` one, or its last lines
        when it has none: the steps after a failed one are cleanup."""
        lines = log.splitlines()
        end = next(
            (i + 1 for i, line in enumerate(lines) if '##[error]' in line),
            len(lines),
        )
        return '\n'.join(lines[max(0, end - failures.LOG_TAIL_LINES) : end])

    def rerun(self, url: str) -> str:
        """Reruns the job at `url`, and the jobs that depend on it."""
        repo, job_id = self._job(url)
        self._gh.api(f'repos/{repo}/actions/jobs/{job_id}/rerun', method='POST')
        return f'Rerunning GitHub Actions job {job_id} of {repo}.'
