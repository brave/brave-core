#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Integration tests for `brockit.GenRustToolchain` against a fake Jenkins.

Where `gen_rust_toolchain_test.py` mocks `requests.Session`, these tests stand
up a real in-process HTTP server (`FakeJenkins`) that emulates the slice of the
Jenkins REST API the task talks to:

* the CSRF crumb issuer (`/crumbIssuer/api/json`),
* the parameterised trigger (`/job/<job>/buildWithParameters`, answered with a
  201 + `Location` header pointing at a transient queue item),
* the build queue item (`/queue/item/<id>/api/json`), which hands back an
  `executable` build URL once an "executor" picks the job up, and
* the Pipeline Stage View (`<build>/wfapi/describe`) with a plain build-API
  fallback (`<build>/api/json`).

The task drives genuine `requests` calls over a loopback socket, so basic-auth
and crumb headers, the `CHROMIUM_TAG` parameter, the `Location`-header handoff,
and the full queue -> build -> result lifecycle are all exercised end to end.

The fake advances each pipeline deterministically by counting polls (no wall
clock involved): a build stays QUEUED for `queue_polls_before_start` polls,
RUNNING for `running_polls_before_done` polls, then settles on `final_status`.
"""

from __future__ import annotations

import base64
import json
import tempfile
import threading
import unittest
from collections import defaultdict
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from urllib.parse import parse_qs, urlparse
from unittest.mock import patch

import brockit

# A concrete version so `_fetch_chromium_tag` short-circuits to `Version(...)`
# without touching git or the network.
TAG = '150.0.7850.1'

# Maps the canonical `final_status` onto the equivalent Pipeline Stage View
# (`wfapi`) status string the fake reports for a finished build.
_FINAL_TO_WFAPI = {
    'SUCCESS': 'SUCCESS',
    'FAILURE': 'FAILED',
    'UNSTABLE': 'UNSTABLE',
    'ABORTED': 'ABORTED',
}


class _NullLive:
    """Stand-in for `rich.live.Live` so the watch loop renders nothing."""

    def __init__(self, *args, **kwargs):
        pass

    def __enter__(self) -> _NullLive:
        return self

    def __exit__(self, *exc) -> bool:
        return False

    def update(self, *args, **kwargs) -> None:
        pass


def _job_from_build_path(path: str) -> str:
    """Pulls `<job>` out of a `/job/<job>/<build-no>` path."""
    parts = [segment for segment in path.split('/') if segment]
    # parts == ['job', '<job>', '<build-no>'].
    return parts[1]


class _Handler(BaseHTTPRequestHandler):
    """Routes the handful of endpoints the task hits to the owning fake."""

    # Silence the default per-request stderr logging.
    def log_message(self, *args) -> None:
        pass

    @property
    def fake(self) -> FakeJenkins:
        return self.server.fake

    def send_json(self, payload: dict, status: int = 200) -> None:
        body = json.dumps(payload).encode('utf-8')
        self.send_response(status)
        self.send_header('Content-Type', 'application/json')
        self.send_header('Content-Length', str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def send_empty(self, status: int, headers: dict | None = None) -> None:
        self.send_response(status)
        for key, value in (headers or {}).items():
            self.send_header(key, value)
        self.send_header('Content-Length', '0')
        self.end_headers()

    def do_GET(self) -> None:
        path = urlparse(self.path).path
        if path == '/crumbIssuer/api/json':
            self.fake.handle_crumb(self)
        elif path.startswith('/queue/item/') and path.endswith('/api/json'):
            qid = path[len('/queue/item/'):-len('/api/json')].strip('/')
            self.fake.handle_queue(self, qid)
        elif path.endswith('/wfapi/describe'):
            self.fake.handle_describe(
                self, _job_from_build_path(path[:-len('/wfapi/describe')]))
        elif path.startswith('/job/') and path.endswith('/api/json'):
            parts = [segment for segment in path.split('/') if segment]
            # ['job', <job>, 'api', 'json'] is the pipeline-level info, whereas
            # ['job', <job>, <build-no>, 'api', 'json'] is a build's API.
            if len(parts) == 4:
                self.fake.handle_job_info(self, parts[1])
            else:
                self.fake.handle_build_api(self, parts[1])
        else:
            self.send_empty(404)

    def do_POST(self) -> None:
        path = urlparse(self.path).path
        if path.startswith('/job/') and path.endswith('/buildWithParameters'):
            job = path[len('/job/'):-len('/buildWithParameters')].strip('/')
            self.fake.handle_trigger(self, job)
        else:
            self.send_empty(404)


class FakeJenkins:
    """An in-process HTTP server emulating the Jenkins endpoints the task uses.

    Use as a context manager; `base_url` is populated on entry. Behaviour is
    tuned via the public attributes below before (or during) a run.
    """

    def __init__(self,
                 *,
                 username: str = 'alice',
                 token: str = 'secret-token'):
        self.username = username
        self.token = token

        # The crumb to serve as `(field, value)`, or None to 404 the issuer
        # (which API-token auth treats as "no crumb needed").
        self.crumb: tuple[str, str] | None = ('Jenkins-Crumb', 'deadbeef')

        # Per-job HTTP status override for the trigger POST (job -> status).
        # Anything >= 400 makes that pipeline's trigger fail.
        self.trigger_status: dict[str, int] = {}

        # Per-job configured `display-name` (job -> display name). A job absent
        # here reports its own name as `displayName`, i.e. no display name set.
        self.display_names: dict[str, str] = {}

        # Lifecycle knobs, applied uniformly to every pipeline.
        self.queue_polls_before_start = 0
        self.running_polls_before_done = 1
        self.final_status = 'SUCCESS'
        self.serve_wfapi = True

        # Recorded trigger POSTs, one dict per call (job, params, auth, crumb).
        self.triggers: list[dict] = []

        self._lock = threading.Lock()
        self._next_qid = 1
        self._qid_to_job: dict[str, str] = {}
        self._queue_polls: dict[str, int] = defaultdict(int)
        self._describe_polls: dict[str, int] = defaultdict(int)
        self._build_polls: dict[str, int] = defaultdict(int)

        # Server state, populated on `__enter__` once the server is bound.
        self._httpd: ThreadingHTTPServer | None = None
        self._thread: threading.Thread | None = None
        self.base_url = ''

    def __enter__(self) -> FakeJenkins:
        self._httpd = ThreadingHTTPServer(('127.0.0.1', 0), _Handler)
        self._httpd.fake = self
        self._thread = threading.Thread(target=self._httpd.serve_forever,
                                        daemon=True)
        self._thread.start()
        host, port = self._httpd.server_address
        self.base_url = f'http://{host}:{port}'
        return self

    def __exit__(self, *exc) -> bool:
        self._httpd.shutdown()
        self._httpd.server_close()
        self._thread.join()
        return False

    def write_config(self, path: Path) -> None:
        """Writes a `~/.jenkins.json` pointing at this server."""
        path.write_text(json.dumps({
            'url': self.base_url,
            'username': self.username,
            'token': self.token,
        }),
                        encoding='utf-8',
                        newline='')

    @staticmethod
    def _decode_auth(handler: _Handler) -> tuple[str, str] | None:
        header = handler.headers.get('Authorization', '')
        if not header.startswith('Basic '):
            return None
        raw = base64.b64decode(header[len('Basic '):]).decode('utf-8')
        user, _, token = raw.partition(':')
        return user, token

    def handle_crumb(self, handler: _Handler) -> None:
        if self.crumb is None:
            handler.send_empty(404)
            return
        field, value = self.crumb
        handler.send_json({'crumbRequestField': field, 'crumb': value})

    def handle_trigger(self, handler: _Handler, job: str) -> None:
        crumb_field = self.crumb[0] if self.crumb else None
        self.triggers.append({
            'job': job,
            'params': parse_qs(urlparse(handler.path).query),
            'auth': self._decode_auth(handler),
            'crumb_header': handler.headers.get(crumb_field)
            if crumb_field else None,
        })

        status = self.trigger_status.get(job, 201)
        if status >= 400:
            handler.send_empty(status)
            return

        with self._lock:
            qid = str(self._next_qid)
            self._next_qid += 1
            self._qid_to_job[qid] = job
        handler.send_empty(201,
                           {'Location': f'{self.base_url}/queue/item/{qid}/'})

    def handle_job_info(self, handler: _Handler, job: str) -> None:
        # Jenkins reports the job name as `displayName` when none is set.
        handler.send_json({
            'name': job,
            'displayName': self.display_names.get(job, job),
        })

    def handle_queue(self, handler: _Handler, qid: str) -> None:
        job = self._qid_to_job.get(qid)
        if job is None:
            handler.send_empty(404)
            return
        with self._lock:
            polls = self._queue_polls[qid]
            self._queue_polls[qid] += 1
        if polls < self.queue_polls_before_start:
            handler.send_json({
                'cancelled': False,
                'executable': None,
                'why': 'Waiting for next available executor',
            })
            return
        build_url = f'{self.base_url}/job/{job}/100/'
        handler.send_json({
            'cancelled': False,
            'executable': {
                'url': build_url
            }
        })

    def handle_describe(self, handler: _Handler, job: str) -> None:
        if not self.serve_wfapi:
            # Emulate a controller without the Stage View plugin; the task
            # falls back to the plain build API.
            handler.send_empty(404)
            return
        with self._lock:
            polls = self._describe_polls[job]
            self._describe_polls[job] += 1
        if polls < self.running_polls_before_done:
            handler.send_json({
                'status': 'IN_PROGRESS',
                'durationMillis': 1000 * (polls + 1),
                'stages': [
                    {
                        'name': 'env',
                        'status': 'SUCCESS'
                    },
                    {
                        'name': 'build',
                        'status': 'IN_PROGRESS'
                    },
                ],
            })
            return
        wfapi_status = _FINAL_TO_WFAPI.get(self.final_status, 'SUCCESS')
        handler.send_json({
            'status': wfapi_status,
            'durationMillis': 99000,
            'stages': [{
                'name': 'build',
                'status': wfapi_status
            }],
        })

    def handle_build_api(self, handler: _Handler, job: str) -> None:
        with self._lock:
            polls = self._build_polls[job]
            self._build_polls[job] += 1
        if polls < self.running_polls_before_done:
            handler.send_json({
                'building': True,
                'duration': 0,
                'result': None
            })
            return
        handler.send_json({
            'building': False,
            'duration': 50000,
            'result': self.final_status,
        })


class GenRustToolchainIntegrationTest(unittest.TestCase):
    """Drives `GenRustToolchain.execute` against `FakeJenkins`."""

    def setUp(self):
        tmp = tempfile.TemporaryDirectory()
        self.addCleanup(tmp.cleanup)
        self.config_path = Path(tmp.name) / '.jenkins.json'
        patcher = patch.object(brockit, 'JENKINS_CONFIG_FILE',
                               self.config_path)
        patcher.start()
        self.addCleanup(patcher.stop)

    def _execute(self, fake: FakeJenkins, *, watch: bool = False) -> list:
        """Runs `execute` against `fake`, returning the watched-job list.

        The job list is captured by intercepting `_render_table`; because the
        `_WatchedJob` instances are mutated in place, the captured reference
        reflects their final state once `execute` returns.
        """
        fake.write_config(self.config_path)
        captured: dict[str, list] = {}

        def _record(_self, _version, _base_url, jobs):
            captured['jobs'] = jobs

        if watch:
            with patch.object(brockit.GenRustToolchain, '_render_table',
                              _record), \
                    patch('brockit.time.sleep'), \
                    patch('brockit.Live', _NullLive):
                brockit.GenRustToolchain().execute(tag=TAG, watch=True)
        else:
            brockit.GenRustToolchain().execute(tag=TAG)
        return captured.get('jobs', [])

    def test_triggers_every_pipeline_with_auth_param_and_crumb(self):
        with FakeJenkins() as fake:
            self._execute(fake)

        self.assertEqual({trigger['job']
                          for trigger in fake.triggers},
                         set(brockit.RUST_TOOLCHAIN_JOBS))
        self.assertEqual(len(fake.triggers), len(brockit.RUST_TOOLCHAIN_JOBS))
        for trigger in fake.triggers:
            self.assertEqual(trigger['params'], {'CHROMIUM_TAG': [TAG]})
            self.assertEqual(trigger['auth'], ('alice', 'secret-token'))
            self.assertEqual(trigger['crumb_header'], 'deadbeef')

    def test_missing_crumb_issuer_still_triggers_without_header(self):
        with FakeJenkins() as fake:
            fake.crumb = None
            self._execute(fake)

        self.assertEqual(len(fake.triggers), len(brockit.RUST_TOOLCHAIN_JOBS))
        for trigger in fake.triggers:
            self.assertIsNone(trigger['crumb_header'])

    def test_trigger_failure_raises_bad_outcome(self):
        with FakeJenkins() as fake:
            fake.trigger_status = {brockit.RUST_TOOLCHAIN_JOBS[0]: 500}
            fake.write_config(self.config_path)
            with self.assertRaises(brockit.BadOutcomeException):
                brockit.GenRustToolchain().execute(tag=TAG)

    def test_watch_runs_full_queue_to_success_lifecycle(self):
        with FakeJenkins() as fake:
            # Force a QUEUED -> RUNNING -> RUNNING -> SUCCESS progression so the
            # queue handoff and the running-state polling are both exercised.
            fake.queue_polls_before_start = 1
            fake.running_polls_before_done = 2
            jobs = self._execute(fake, watch=True)

        self.assertEqual(len(jobs), len(brockit.RUST_TOOLCHAIN_JOBS))
        for job in jobs:
            self.assertEqual(job.state, 'SUCCESS')
            self.assertEqual(job.stage, '(done)')
            self.assertEqual(job.elapsed, '1m39s')  # 99000 ms
            # The build URL was resolved from the queue item's executable.
            self.assertIn(f'/job/{job.job}/', job.build_url)

    def test_watch_uses_pipeline_display_name_for_bot(self):
        with FakeJenkins() as fake:
            fake.display_names = {
                job: f'Display of {job}'
                for job in brockit.RUST_TOOLCHAIN_JOBS
            }
            jobs = self._execute(fake, watch=True)

        for job in jobs:
            self.assertEqual(job.display_name, f'Display of {job.job}')
            self.assertEqual(job.bot, f'Display of {job.job}')

    def test_watch_bot_falls_back_to_job_name_without_display_name(self):
        with FakeJenkins() as fake:
            # No display names configured: Jenkins echoes the job name as
            # `displayName`, which must be treated as "unset".
            jobs = self._execute(fake, watch=True)

        for job in jobs:
            self.assertIsNone(job.display_name)
            self.assertEqual(job.bot, job.job)

    def test_watch_falls_back_to_build_api_and_reports_failure(self):
        with FakeJenkins() as fake:
            fake.serve_wfapi = False
            fake.final_status = 'FAILURE'
            jobs = self._execute(fake, watch=True)

        for job in jobs:
            self.assertEqual(job.state, 'FAILURE')
            self.assertEqual(job.stage, '(done)')
            self.assertEqual(job.elapsed, '50s')  # 50000 ms


if __name__ == '__main__':
    unittest.main()
