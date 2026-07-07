# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""CI (Jenkins) integration for Brockit toolchain pipelines.

This module owns everything specific to launching and watching CI jobs. A caller
hands `JenkinsCi.trigger` one or more fully-qualified job URLs plus the build
parameters, and optionally asks to watch the pipelines to completion with a
live-updating table.
"""

from __future__ import annotations

import contextlib
from dataclasses import dataclass
import json
from pathlib import Path
import time
import urllib.parse

import requests
from rich.box import Box
from rich.live import Live
from rich.padding import Padding
from rich.spinner import Spinner
from rich.table import Table
from rich.text import Text

from exceptions import BadOutcomeException, InvalidInputException
from terminal import console, terminal

# Configuration file holding the Jenkins credentials used to trigger toolchain
# pipelines. See `JenkinsCi.from_config` for the expected schema.
JENKINS_CONFIG_FILE = Path.home() / '.jenkins.json'

# How often `--watch` polls Jenkins for pipeline progress.
WATCH_POLL_INTERVAL_SECONDS = 8

# Maps Pipeline Stage View (`wfapi`) statuses onto the canonical states the
# watch table tracks. Statuses not listed (e.g. NOT_EXECUTED) leave the state
# unchanged.
_WFAPI_STATUS_TO_STATE = {
    'IN_PROGRESS': 'RUNNING',
    'PAUSED_PENDING_INPUT': 'RUNNING',
    'SUCCESS': 'SUCCESS',
    'FAILED': 'FAILURE',
    'ABORTED': 'ABORTED',
    'UNSTABLE': 'UNSTABLE',
}

# States meaning a pipeline has finished and no longer needs polling.
_TERMINAL_WATCH_STATES = frozenset(
    {'SUCCESS', 'FAILURE', 'ABORTED', 'UNSTABLE', 'CANCELLED'})

# Minimalist box for the `--watch` table: column dividers plus a single
# header rule, no outer frame (paired with `show_edge=False`). Each of the
# eight lines is four chars -- left edge, cell fill, column divider, right
# edge -- in the order Rich's `Box` expects (top, head, head-rule, mid, row,
# foot-rule, foot, bottom). Edge/foot lines are placeholders never drawn once
# the edge is hidden and the table has no footer.
_WATCH_TABLE_BOX = Box('    \n'  # top
                       '  │ \n'  # head
                       ' -- \n'  # head rule
                       '  │ \n'  # mid
                       '  │ \n'  # row
                       ' -- \n'  # foot rule
                       '  │ \n'  # foot
                       '    \n')  # bottom

# Icon + rich style for each watch state, used to render the State column.
_WATCH_STATE_STYLE = {
    'QUEUED': ('⏳', 'dim'),
    'RUNNING': ('🔧', 'cyan'),
    'SUCCESS': ('✔️', 'green'),
    'FAILURE': ('🚨', 'bold red'),
    'ABORTED': ('🛑', 'yellow'),
    'CANCELLED': ('🚫', 'yellow'),
    'UNSTABLE': ('⚠️', 'yellow'),
    'UNKNOWN': ('🤔', 'dim'),
}


def _format_duration(millis) -> str:
    """Formats a Jenkins millisecond duration as e.g. "12m04s"."""
    if not millis:
        return ''
    total_seconds = int(millis) // 1000
    minutes, seconds = divmod(total_seconds, 60)
    if minutes:
        return f'{minutes}m{seconds:02d}s'
    return f'{seconds}s'


@dataclass
class _WatchedJob:
    """Mutable per-pipeline state tracked while `--watch` polls Jenkins."""

    # The fully-qualified Jenkins job URL (ends with a trailing slash), taken
    # verbatim from the toolchain's static data.
    url: str

    # The transient queue-item URL from the trigger's `Location` header.
    queue_url: str

    # The build URL, resolved once an executor dequeues the job.
    build_url: str | None = None

    # Canonical state: QUEUED / RUNNING / SUCCESS / FAILURE / ABORTED /
    # UNSTABLE / CANCELLED / UNKNOWN.
    state: str = 'QUEUED'

    # Human-readable current stage (or the queue reason while QUEUED).
    stage: str = ''

    # Pre-formatted elapsed build time (e.g. "12m04s"), as of the last poll.
    # Used as-is for non-running rows. RUNNING rows tick forward from the two
    # fields below instead (see `_ElapsedClock`).
    elapsed: str = ''

    # The raw server-reported build duration (ms) at the last poll, and the
    # monotonic clock read at that same instant. Together they anchor a
    # RUNNING row's locally-ticking elapsed counter. Both None until a build
    # is resolved.
    duration_millis: int | None = None
    elapsed_anchor: float | None = None

    # The pipeline's configured `display-name`, resolved once when watching
    # starts. None until resolved, or when the pipeline sets no display name.
    display_name: str | None = None

    @property
    def name(self) -> str:
        """The Jenkins job name parsed from the job URL's last `/job/` segment.

        `https://ci.brave.com/view/toolchains/job/<name>/` -> `<name>`.
        """
        return self.url.rstrip('/').rsplit('/job/', 1)[-1].strip('/')

    @property
    def bot(self) -> str:
        """Label for the table's Bot column.

        The pipeline's configured `display-name` when it has one, otherwise
        its Jenkins job name.
        """
        return self.display_name or self.name

    @property
    def is_terminal(self) -> bool:
        """Whether the pipeline has finished and no longer needs polling."""
        return self.state in _TERMINAL_WATCH_STATES

    @property
    def link(self) -> str:
        """The best link available: the build URL, else the job page."""
        return self.build_url or self.url


class _ElapsedClock:
    """Renderable that advances a RUNNING job's elapsed time between polls.

    Rich re-renders the `Live` tree on every refresh, so recomputing the value
    here lets the counter tick smoothly at the Live refresh rate instead of
    jumping once per poll.
    """

    def __init__(self, base_millis: int, anchor: float) -> None:
        # The build duration the server reported at the last poll.
        self._base_millis = base_millis

        # The monotonic clock read at that same poll, anchoring the local tick.
        self._anchor = anchor

    def __rich__(self) -> Text:
        elapsed_millis = (self._base_millis +
                          (time.monotonic() - self._anchor) * 1000)
        return Text(_format_duration(elapsed_millis), justify='right')


class JenkinsCi:
    """Triggers (and optionally watches) Jenkins pipelines.
    """

    def __init__(self, username: str, token: str) -> None:
        # Jenkins credentials, sent as HTTP basic auth on every request.
        self._username = username
        self._token = token

    @classmethod
    def from_config(cls) -> JenkinsCi:
        """Builds a `JenkinsCi` with the credentials."""
        if not JENKINS_CONFIG_FILE.is_file():
            raise InvalidInputException(
                f'Jenkins config not found at {JENKINS_CONFIG_FILE}. Create it '
                'with [bold cyan]username[/] and [bold cyan]token[/] fields.')

        try:
            config = json.loads(
                JENKINS_CONFIG_FILE.read_bytes().decode('utf-8'))
        except json.JSONDecodeError as e:
            raise InvalidInputException(
                f'Failed to parse {JENKINS_CONFIG_FILE}: {e}') from e

        missing = [key for key in ('username', 'token') if not config.get(key)]
        if missing:
            raise InvalidInputException(
                f'{JENKINS_CONFIG_FILE} is missing required field(s): '
                f'{", ".join(missing)}.')

        return cls(config['username'], config['token'])

    @staticmethod
    def _server_root(job_urls: tuple[str, ...]) -> str:
        """Derives `scheme://host` from the job URLs (all share one host)."""
        parsed = urllib.parse.urlparse(job_urls[0])
        return f'{parsed.scheme}://{parsed.netloc}'

    @staticmethod
    def _get_crumb(session: requests.Session, base_url: str) -> dict[str, str]:
        """Fetches a Jenkins CSRF crumb as a ready-to-merge header dict.

        Returns an empty dict when the crumb issuer is unavailable. API-token
        auth is usually crumb-exempt, so a missing issuer is treated as "no
        crumb needed" rather than a hard failure.
        """
        try:
            response = session.get(f'{base_url}/crumbIssuer/api/json',
                                   timeout=15)
            response.raise_for_status()
            data = response.json()
            return {data['crumbRequestField']: data['crumb']}
        except (requests.RequestException, KeyError, ValueError):
            return {}

    def trigger(self,
                job_urls: tuple[str, ...],
                *,
                params: dict[str, str] | None = None,
                properties: object = None,
                watch: bool = False,
                title: str = '') -> bool:
        """Triggers one or more pipelines, optionally watching to completion.

        Args:
            job_urls:
                Fully-qualified Jenkins job URLs to trigger (one for the
                hermetic toolchains, four for Rust).
            params:
                The optional `buildWithParameters` query parameters (e.g.
                `{'CHROMIUM_TAG': '150.0.7850.1'}`). Jobs that take none (they
                read `properties` instead) can omit it.
            properties:
                A JSON-serialisable payload used by the recipes engine as the
                properties of a job.
            watch:
                When True, show a live-updating table of each pipeline's stage
                and status until all finish.
            title:
                Heading used for the watch table and the trigger log line.

        Returns:
            Whether every pipeline finished successfully. Always True when not
            watching (the builds were kicked off but not awaited). When watching
            it reflects the watched outcome (a Ctrl+C detach counts as False).
        """
        base_url = self._server_root(job_urls)
        payload = None if properties is None else json.dumps(properties)

        session = requests.Session()
        session.auth = (self._username, self._token)
        crumb = self._get_crumb(session, base_url)

        if title:
            terminal.log_task(f'Triggering pipelines for {title}:')

        watched: list[_WatchedJob] = []
        failures: list[str] = []
        for url in job_urls:
            job_params = dict(params or {})
            if payload is not None:
                # Windows agents unwrap PROPERTIES through cmd, which eats bare
                # double quotes, so escape them for those jobs only.
                job_params['PROPERTIES'] = (payload.replace('"', '\\"')
                                            if '-windows-' in url else payload)
            try:
                response = session.post(f'{url}buildWithParameters',
                                        params=job_params,
                                        headers=crumb,
                                        timeout=30)
                response.raise_for_status()
            except requests.RequestException as e:
                failures.append(url)
                console.log(Padding(f'✘ {url}: {e}', (0, 4)))
                continue

            # Jenkins' 201 Location header points at the transient queue item,
            # not the build: the build number isn't assigned until an executor
            # picks the job up, and the queue URL only serves JSON.
            job = _WatchedJob(url=url,
                              queue_url=response.headers.get('Location', ''))
            watched.append(job)
            if not watch:
                # Link the job page, which always resolves and surfaces the
                # queued/running build.
                terminal.log_task(f'[bold]✔️ [/]{job.name} ➜ {url}')

        if failures:
            raise BadOutcomeException(
                'Failed to trigger the following pipelines:\n%s' %
                '\n'.join(f'    * {url}' for url in failures))

        if watch:
            return self._watch(session, title, watched)

        return True

    @staticmethod
    def _get_json(session: requests.Session, url: str) -> dict | None:
        """GETs `url` and returns the parsed JSON, or None on any failure."""
        try:
            response = session.get(url, timeout=15)
            response.raise_for_status()
            return response.json()
        except (requests.RequestException, ValueError):
            return None

    def _resolve_display_name(self, session: requests.Session,
                              job: _WatchedJob) -> None:
        """Records a pipeline's `display-name` for the Bot column, if set.

        Jenkins returns the job name as `displayName` when no display name is
        configured, so a value equal to the job name is treated as "unset" and
        left as None. `_WatchedJob.bot` then falls back to the job name.
        """
        info = self._get_json(session,
                              f'{job.url}api/json?tree=displayName,name')
        if info is None:
            return
        display_name = info.get('displayName')
        if display_name and display_name != info.get('name'):
            job.display_name = display_name

    def _poll_job(self, session: requests.Session, job: _WatchedJob) -> None:
        """Refreshes one pipeline's state in place.

        Resolves the queue item to a build the first time an executor picks it
        up, then tracks the running build's stage and result via the Pipeline
        Stage View API (falling back to the plain build API).
        """
        if job.is_terminal:
            return

        if job.build_url is None:
            if not job.queue_url:
                job.state = 'UNKNOWN'
                job.stage = 'no queue item returned'
                return
            item = self._get_json(session, f'{job.queue_url}api/json')
            if item is None:
                return  # Transient; retry on the next poll.
            if item.get('cancelled'):
                job.state = 'CANCELLED'
                job.stage = 'queue item cancelled'
                return
            executable = item.get('executable')
            if not executable:
                # Still queued; surface Jenkins' reason (e.g. "Waiting for
                # next available executor").
                job.state = 'QUEUED'
                job.stage = (item.get('why') or 'waiting').strip()
                return
            job.build_url = executable.get('url') or ''
            job.state = 'RUNNING'

        self._refresh_build_state(session, job)

    def _record_elapsed(self, job: _WatchedJob, millis) -> None:
        """Anchors a job's elapsed time to the latest server-reported duration.

        Stores the raw duration and a monotonic timestamp so a RUNNING row can
        tick forward between polls (see `_ElapsedClock`), and keeps the
        formatted string used to render every non-running row.
        """
        job.duration_millis = int(millis) if millis else None
        job.elapsed_anchor = time.monotonic()
        job.elapsed = _format_duration(millis)

    def _refresh_build_state(self, session: requests.Session,
                             job: _WatchedJob) -> None:
        """Updates state/stage/elapsed for a job that already has a build."""
        describe = self._get_json(session, f'{job.build_url}wfapi/describe')
        if describe is not None:
            job.state = _WFAPI_STATUS_TO_STATE.get(describe.get('status', ''),
                                                   job.state)
            self._record_elapsed(job, describe.get('durationMillis'))
            stages = describe.get('stages') or []
            running = [s for s in stages if s.get('status') == 'IN_PROGRESS']
            if running:
                # The deepest reported in-progress stage is the most specific.
                job.stage = running[-1].get('name', '')
            elif job.is_terminal:
                job.stage = '(done)'
            elif stages:
                job.stage = stages[-1].get('name', '')
            return

        # Fallback for jobs without the Stage View plugin: the plain build API
        # gives building/result but no per-stage detail.
        info = self._get_json(session, f'{job.build_url}api/json')
        if info is None:
            return
        self._record_elapsed(job, info.get('duration'))
        if info.get('building'):
            job.state = 'RUNNING'
            job.stage = 'building'
        else:
            job.state = info.get('result') or 'UNKNOWN'
            job.stage = '(done)'

    @staticmethod
    def _state_cell(state: str) -> str | Spinner:
        """Renders the State column.

        RUNNING gets an animated throbber (the `Live` display drives the
        animation); every other state is a static icon + label.
        """
        if state == 'RUNNING':
            return Spinner('dots', text='RUNNING', style='cyan')
        icon, style = _WATCH_STATE_STYLE.get(state, ('?', 'dim'))
        return f'[{style}]{icon} {state}[/]'

    @staticmethod
    def _elapsed_cell(job: _WatchedJob) -> str | _ElapsedClock:
        """Renders the Elapsed column.

        A RUNNING build with a resolved duration ticks forward between polls
        via `_ElapsedClock`. Every other state shows the static,
        server-accurate value captured at the last poll.
        """
        if (job.state == 'RUNNING' and job.duration_millis is not None
                and job.elapsed_anchor is not None):
            return _ElapsedClock(job.duration_millis, job.elapsed_anchor)
        return job.elapsed or '—'

    @staticmethod
    def _link_cell(url: str) -> Text:
        """Renders a URL as a styled, clickable hyperlink for the Build column.

        Table cells skip the `ReprHighlighter` that `console.log`/`print` run
        over their output, so a bare URL string renders as plain text. Wrapping
        it in a `Text` with a `link` style emits the OSC 8 hyperlink escape
        (clickable in capable terminals) and the blue underline mirrors the
        look Rich gives auto-detected URLs elsewhere.
        """
        return Text(url, style=f'underline blue link {url}')

    @staticmethod
    def _dim_cell(renderable: str | Text, dim: bool) -> str | Text:
        """Dims a non-state cell for finished, non-successful rows.

        Returns the renderable untouched when `dim` is False. Otherwise wraps
        it in Rich's `dim` style (preserving any existing style, such as the
        Build column's hyperlink) so the whole row reads as muted -- except the
        State cell, which the caller leaves coloured so the outcome stands out.
        """
        if not dim:
            return renderable
        text = renderable if isinstance(renderable, Text) else Text(
            str(renderable))
        text.stylize('dim')
        return text

    def _render_table(self, title: str, jobs: list[_WatchedJob]) -> Table:
        """Builds the per-pipeline progress table rendered in place by Live."""
        table = Table(title=title,
                      title_justify='left',
                      box=_WATCH_TABLE_BOX,
                      show_edge=False,
                      expand=True)
        table.add_column('Bot', no_wrap=True)
        table.add_column('State', no_wrap=True)
        table.add_column('Stage', no_wrap=True)
        table.add_column('Elapsed', justify='right', no_wrap=True)
        table.add_column('Build', overflow='fold')
        for job in jobs:
            # A finished job is dimmed across the row, except its State cell,
            # which keeps its colour so the outcome still stands out. This keeps
            # attention on the pipelines still in flight.
            dim = job.is_terminal
            table.add_row(self._dim_cell(job.bot, dim),
                          self._state_cell(job.state),
                          self._dim_cell(job.stage or '—', dim),
                          self._dim_cell(self._elapsed_cell(job), dim),
                          self._dim_cell(self._link_cell(job.link), dim))
        return table

    @staticmethod
    @contextlib.contextmanager
    def _suspended_outer_status():
        """Pause any active Brockit status spinner around the watch display.

        rich allows only one live display at a time. Run standalone there is no
        spinner and this is a no-op, but when a lift drives the watch mid-run
        the outer `Task.run` status spinner (set up by `terminal.with_status`)
        is live and would clash with the table. Stop it for the duration of the
        watch and restore it afterwards.
        """
        status = terminal.status
        if status is not None:
            status.stop()
        try:
            yield
        finally:
            if status is not None:
                status.start()

    def _watch(self, session: requests.Session, title: str,
               jobs: list[_WatchedJob]) -> bool:
        """Polls the triggered pipelines, updating an in-place table.

        Returns whether every pipeline finished with a SUCCESS state. A Ctrl+C
        detach counts as not-all-successful.
        """
        terminal.log_task('Watching pipelines — press [bold cyan]Ctrl+C[/] to '
                          'stop watching (builds keep running).')
        # Resolve each pipeline's display name once up front so the Bot column
        # shows it from the very first render.
        for job in jobs:
            self._resolve_display_name(session, job)
        try:
            # The throbber on RUNNING rows animates off the Live's own refresh
            # (~12.5fps matches the `dots` spinner cadence); the data itself is
            # only re-polled every WATCH_POLL_INTERVAL_SECONDS. Any outer lift
            # spinner is suspended so the two live displays don't clash.
            with self._suspended_outer_status(), Live(
                    self._render_table(title, jobs),
                    console=console,
                    refresh_per_second=12.5) as live:
                while True:
                    for job in jobs:
                        self._poll_job(session, job)
                    live.update(self._render_table(title, jobs))
                    if all(job.is_terminal for job in jobs):
                        break
                    time.sleep(WATCH_POLL_INTERVAL_SECONDS)
        except KeyboardInterrupt:
            # Detach cleanly: the builds keep running on CI.
            console.log('[yellow]Stopped watching. Builds continue on CI:[/]')
            for job in jobs:
                console.log(Padding(f'{job.bot}: {job.link}', (0, 4)))
            return False

        return all(job.state == 'SUCCESS' for job in jobs)
