#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Tests for `brockit.GenRustToolchain`.

The file is split in two layers:

* `LoadJenkinsConfigTest` exercises the pure `~/.jenkins.json` reader,
  pointing `brockit.JENKINS_CONFIG_FILE` at a temp file. No network involved.

* `GenRustToolchainExecuteTest` drives `GenRustToolchain.execute` with a mocked
  `requests.Session`, asserting that every pipeline is triggered with the right
  URL, `CHROMIUM_TAG` parameter, auth, and CSRF crumb, and that a per-job
  failure surfaces as a `BadOutcomeException`.
"""

from __future__ import annotations

import json
import tempfile
import unittest
from pathlib import Path
from unittest.mock import MagicMock, patch

from rich.console import Console

import brockit

# A valid Jenkins config, written into the temp file by the execute tests.
VALID_CONFIG = {
    'url': 'https://ci.brave.com',
    'username': 'alice',
    'token': 'secret-token',
}


class LoadJenkinsConfigTest(unittest.TestCase):
    """Tests for `GenRustToolchain._load_jenkins_config`."""

    def setUp(self):
        tmp = tempfile.TemporaryDirectory()
        self.addCleanup(tmp.cleanup)
        self.config_path = Path(tmp.name) / '.jenkins.json'
        patcher = patch.object(brockit, 'JENKINS_CONFIG_FILE',
                               self.config_path)
        patcher.start()
        self.addCleanup(patcher.stop)

    def _write(self, data) -> None:
        self.config_path.write_text(json.dumps(data),
                                    encoding='utf-8',
                                    newline='')

    def test_reads_all_fields_and_strips_slash(self):
        """All three fields are returned, with any trailing slash dropped from
        the base URL so it can be concatenated with `/job/...` paths."""
        self._write({
            'url': 'https://ci.example.com/',
            'username': 'bob',
            'token': 'tok',
        })
        base_url, user, token = (
            brockit.GenRustToolchain._load_jenkins_config())
        self.assertEqual(base_url, 'https://ci.example.com')
        self.assertEqual(user, 'bob')
        self.assertEqual(token, 'tok')

    def test_missing_file_raises(self):
        with self.assertRaises(brockit.InvalidInputException):
            brockit.GenRustToolchain._load_jenkins_config()

    def test_missing_field_raises(self):
        self._write({'url': 'https://ci.brave.com', 'username': 'bob'})
        with self.assertRaises(brockit.InvalidInputException):
            brockit.GenRustToolchain._load_jenkins_config()

    def test_empty_field_raises(self):
        """A present-but-empty field is treated as missing."""
        self._write({
            'url': 'https://ci.brave.com',
            'username': '',
            'token': 'tok',
        })
        with self.assertRaises(brockit.InvalidInputException):
            brockit.GenRustToolchain._load_jenkins_config()

    def test_invalid_json_raises(self):
        self.config_path.write_text('{not valid json',
                                    encoding='utf-8',
                                    newline='')
        with self.assertRaises(brockit.InvalidInputException):
            brockit.GenRustToolchain._load_jenkins_config()


class GenRustToolchainExecuteTest(unittest.TestCase):
    """End-to-end tests for `GenRustToolchain.execute`."""

    def setUp(self):
        tmp = tempfile.TemporaryDirectory()
        self.addCleanup(tmp.cleanup)
        self.config_path = Path(tmp.name) / '.jenkins.json'
        self.config_path.write_text(json.dumps(VALID_CONFIG),
                                    encoding='utf-8',
                                    newline='')
        patcher = patch.object(brockit, 'JENKINS_CONFIG_FILE',
                               self.config_path)
        patcher.start()
        self.addCleanup(patcher.stop)

    @staticmethod
    def _make_session() -> MagicMock:
        """Builds a `requests.Session` mock whose crumb GET and build POST both
        succeed."""
        session = MagicMock()

        crumb_resp = MagicMock()
        crumb_resp.json.return_value = {
            'crumbRequestField': 'Jenkins-Crumb',
            'crumb': 'deadbeef',
        }
        crumb_resp.raise_for_status.return_value = None
        session.get.return_value = crumb_resp

        post_resp = MagicMock()
        post_resp.headers = {'Location': 'https://ci.brave.com/queue/item/1/'}
        post_resp.raise_for_status.return_value = None
        session.post.return_value = post_resp

        return session

    @patch('brockit.requests.Session')
    def test_triggers_all_jobs(self, session_cls):
        session = self._make_session()
        session_cls.return_value = session

        brockit.GenRustToolchain().execute(tag='150.0.7850.1')

        # Basic-auth credentials come straight from the config.
        self.assertEqual(session.auth, ('alice', 'secret-token'))

        # One POST per pipeline, each at the right buildWithParameters URL.
        self.assertEqual(session.post.call_count,
                         len(brockit.RUST_TOOLCHAIN_JOBS))
        posted_urls = {call.args[0] for call in session.post.call_args_list}
        self.assertEqual(
            posted_urls, {
                f'https://ci.brave.com/job/{job}/buildWithParameters'
                for job in brockit.RUST_TOOLCHAIN_JOBS
            })

        # Every call carries the resolved CHROMIUM_TAG and the crumb header.
        for call in session.post.call_args_list:
            self.assertEqual(call.kwargs['params'],
                             {'CHROMIUM_TAG': '150.0.7850.1'})
            self.assertEqual(call.kwargs['headers'],
                             {'Jenkins-Crumb': 'deadbeef'})

    @patch('brockit.requests.Session')
    @patch('brockit._fetch_chromium_tag')
    def test_resolves_label_via_fetch_chromium_tag(self, fetch, session_cls):
        """A label like `@latest-canary` is resolved through
        `_fetch_chromium_tag`, and the resolved version is what gets sent as
        `CHROMIUM_TAG`."""
        fetch.return_value = brockit.Version('151.0.1.2')
        session = self._make_session()
        session_cls.return_value = session

        brockit.GenRustToolchain().execute(tag='@latest-canary')

        fetch.assert_called_once_with('@latest-canary')
        for call in session.post.call_args_list:
            self.assertEqual(call.kwargs['params'],
                             {'CHROMIUM_TAG': '151.0.1.2'})

    @patch('brockit.requests.Session')
    def test_missing_crumb_issuer_proceeds_without_header(self, session_cls):
        """When the crumb issuer is unavailable, the builds are still triggered
        with no crumb header (API-token auth is crumb-exempt)."""
        session = self._make_session()
        session.get.side_effect = brockit.requests.RequestException('no crumb')
        session_cls.return_value = session

        brockit.GenRustToolchain().execute(tag='150.0.7850.1')

        self.assertEqual(session.post.call_count,
                         len(brockit.RUST_TOOLCHAIN_JOBS))
        for call in session.post.call_args_list:
            self.assertEqual(call.kwargs['headers'], {})

    @patch('brockit.requests.Session')
    def test_job_failure_raises_bad_outcome(self, session_cls):
        """A non-2xx response from any pipeline trigger surfaces as a
        `BadOutcomeException`."""
        session = self._make_session()
        failing = MagicMock()
        failing.raise_for_status.side_effect = brockit.requests.HTTPError(
            '403 Forbidden')
        session.post.return_value = failing
        session_cls.return_value = session

        with self.assertRaises(brockit.BadOutcomeException):
            brockit.GenRustToolchain().execute(tag='150.0.7850.1')

    @patch('brockit.requests.Session')
    def test_missing_config_raises_before_any_request(self, session_cls):
        """With no config file present, the task fails before issuing any
        Jenkins request."""
        self.config_path.unlink()
        session = self._make_session()
        session_cls.return_value = session

        with self.assertRaises(brockit.InvalidInputException):
            brockit.GenRustToolchain().execute(tag='150.0.7850.1')

        session.post.assert_not_called()


def _json_response(payload):
    """A response mock whose `.json()` yields `payload` and which is 2xx."""
    resp = MagicMock()
    resp.json.return_value = payload
    resp.raise_for_status.return_value = None
    return resp


def _dispatching_session(routes):
    """A session mock whose `.get` dispatches by first matching URL substring.

    `routes` is an ordered list of `(substring, payload)` pairs; a payload that
    is an `Exception` is raised (to simulate a failed/absent endpoint),
    otherwise it is returned as a JSON response.
    """
    session = MagicMock()

    def _get(url, timeout=None):  # pylint: disable=unused-argument
        for needle, payload in routes:
            if needle in url:
                if isinstance(payload, Exception):
                    raise payload
                return _json_response(payload)
        raise brockit.requests.RequestException(f'unexpected GET {url}')

    session.get.side_effect = _get
    return session


class RunWatchingTest(unittest.TestCase):
    """`--watch` must bypass `Task.run`'s status spinner (only one live
    display at a time) and drive `execute` with `watch=True`."""

    def test_drives_execute_with_watch_without_spinner(self):
        task = brockit.GenRustToolchain()
        with patch.object(task, 'execute') as execute, \
                patch('brockit.terminal.with_status') as with_status:
            task.run_watching(tag='150.0.7850.1')

        execute.assert_called_once_with(tag='150.0.7850.1', watch=True)
        with_status.assert_not_called()


class WatchedJobTest(unittest.TestCase):
    """Tests for the `_WatchedJob` helpers."""

    def _job(self, **kwargs):
        defaults = {
            'job': 'brave-browser-rust-toolchain-aux-build-linux-x64',
            'queue_url': 'https://ci.brave.com/queue/item/1/',
        }
        defaults.update(kwargs)
        return brockit._WatchedJob(**defaults)

    def test_bot_is_pipeline_name(self):
        self.assertEqual(self._job().bot,
                         'brave-browser-rust-toolchain-aux-build-linux-x64')

    def test_bot_prefers_display_name(self):
        self.assertEqual(self._job(display_name='Linux x64').bot, 'Linux x64')

    def test_is_terminal(self):
        self.assertTrue(self._job(state='SUCCESS').is_terminal)
        self.assertTrue(self._job(state='FAILURE').is_terminal)
        self.assertFalse(self._job(state='RUNNING').is_terminal)
        self.assertFalse(self._job(state='QUEUED').is_terminal)

    def test_link_prefers_build_url(self):
        job = self._job(build_url='https://ci.brave.com/job/x/5/')
        self.assertEqual(job.link('https://ci.brave.com'),
                         'https://ci.brave.com/job/x/5/')

    def test_link_falls_back_to_job_page(self):
        self.assertEqual(
            self._job().link('https://ci.brave.com'),
            'https://ci.brave.com/job/'
            'brave-browser-rust-toolchain-aux-build-linux-x64/')


class FormatDurationTest(unittest.TestCase):
    """Tests for `GenRustToolchain._format_duration`."""

    def test_empty_for_missing(self):
        self.assertEqual(brockit.GenRustToolchain._format_duration(None), '')
        self.assertEqual(brockit.GenRustToolchain._format_duration(0), '')

    def test_seconds_only(self):
        self.assertEqual(brockit.GenRustToolchain._format_duration(5000), '5s')

    def test_minutes_and_seconds_zero_padded(self):
        self.assertEqual(brockit.GenRustToolchain._format_duration(62000),
                         '1m02s')
        self.assertEqual(brockit.GenRustToolchain._format_duration(100000),
                         '1m40s')


class StateCellTest(unittest.TestCase):
    """Tests for `GenRustToolchain._state_cell`."""

    def test_running_is_animated_spinner(self):
        cell = brockit.GenRustToolchain._state_cell('RUNNING')
        self.assertIsInstance(cell, brockit.Spinner)

    def test_other_states_are_static_markup(self):
        cell = brockit.GenRustToolchain._state_cell('SUCCESS')
        self.assertIsInstance(cell, str)
        self.assertIn('SUCCESS', cell)


class LinkCellTest(unittest.TestCase):
    """The Build column renders URLs as styled, clickable hyperlinks."""

    def test_wraps_url_in_link_style(self):
        url = 'https://ci.brave.com/job/x/5/'
        cell = brockit.GenRustToolchain._link_cell(url)
        self.assertIsInstance(cell, brockit.Text)
        # The raw URL is preserved (so terminal URL detection still works)...
        self.assertEqual(cell.plain, url)
        # ...and an OSC 8 link plus the blue underline are applied.
        self.assertIn(f'link {url}', cell.style)
        self.assertIn('underline', cell.style)


class DimCellTest(unittest.TestCase):
    """`_dim_cell` mutes finished, non-successful rows except the State cell."""

    def test_passes_through_when_not_dim(self):
        self.assertEqual(brockit.GenRustToolchain._dim_cell('build', False),
                         'build')

    def test_wraps_string_in_dim(self):
        cell = brockit.GenRustToolchain._dim_cell('build', True)
        self.assertIsInstance(cell, brockit.Text)
        self.assertEqual(cell.plain, 'build')
        self.assertIn('dim', [span.style for span in cell.spans])

    def test_dims_existing_text_preserving_base_style(self):
        link = brockit.GenRustToolchain._link_cell('https://ci.brave.com/x/')
        cell = brockit.GenRustToolchain._dim_cell(link, True)
        # The hyperlink styling is preserved and dim is layered on top.
        self.assertIn('link https://ci.brave.com/x/', cell.style)
        self.assertIn('dim', [span.style for span in cell.spans])


class RenderTableDimTest(unittest.TestCase):
    """Finished non-successful rows render dim; successful ones do not."""

    DIM = '\x1b[2m'  # SGR code Rich emits for the `dim` style.

    def _render(self, state):
        job = brockit._WatchedJob(
            job='brave-browser-rust-toolchain-aux-build-linux-x64',
            queue_url='',
            build_url='https://ci.brave.com/job/x/5/',
            state=state,
            stage='(done)',
            elapsed='5m00s')
        table = brockit.GenRustToolchain()._render_table(
            brockit.Version('150.0.7850.1'), 'https://ci.brave.com', [job])
        console = Console(force_terminal=True, width=200)
        with console.capture() as capture:
            console.print(table)
        return capture.get()

    def test_failure_row_is_dimmed(self):
        self.assertIn(self.DIM, self._render('FAILURE'))

    def test_aborted_row_is_dimmed(self):
        self.assertIn(self.DIM, self._render('ABORTED'))

    def test_success_row_is_not_dimmed(self):
        self.assertNotIn(self.DIM, self._render('SUCCESS'))


class ElapsedClockTest(unittest.TestCase):
    """`_ElapsedClock` ticks forward from its anchor on each render."""

    @patch('brockit.time.monotonic')
    def test_ticks_from_anchor(self, monotonic):
        clock = brockit._ElapsedClock(base_millis=60000, anchor=100.0)
        monotonic.return_value = 130.0  # 30s past the anchor
        self.assertEqual(clock.__rich__().plain, '1m30s')

    @patch('brockit.time.monotonic')
    def test_uses_base_when_no_time_passed(self, monotonic):
        clock = brockit._ElapsedClock(base_millis=5000, anchor=100.0)
        monotonic.return_value = 100.0
        self.assertEqual(clock.__rich__().plain, '5s')


class ElapsedCellTest(unittest.TestCase):
    """The Elapsed cell is a live clock only while RUNNING with a duration."""

    def _job(self, **kwargs):
        defaults = {'job': 'job-linux', 'queue_url': ''}
        defaults.update(kwargs)
        return brockit._WatchedJob(**defaults)

    def test_running_with_anchor_is_clock(self):
        job = self._job(state='RUNNING',
                        duration_millis=1000,
                        elapsed_anchor=10.0,
                        elapsed='1s')
        self.assertIsInstance(brockit.GenRustToolchain._elapsed_cell(job),
                              brockit._ElapsedClock)

    def test_running_without_duration_is_static(self):
        """A RUNNING build whose duration hasn't resolved yet shows a dash, not
        a clock anchored to nothing."""
        self.assertEqual(
            brockit.GenRustToolchain._elapsed_cell(self._job(state='RUNNING')),
            '—')

    def test_terminal_shows_static_server_value(self):
        job = self._job(state='SUCCESS',
                        duration_millis=100000,
                        elapsed_anchor=10.0,
                        elapsed='1m40s')
        self.assertEqual(brockit.GenRustToolchain._elapsed_cell(job), '1m40s')


class ResolveDisplayNameTest(unittest.TestCase):
    """Tests for `GenRustToolchain._resolve_display_name`."""

    BASE = 'https://ci.brave.com'

    def _job(self):
        return brockit._WatchedJob(job='job-linux', queue_url='')

    def test_sets_display_name_when_distinct_from_job_name(self):
        job = self._job()
        session = _dispatching_session([('/job/job-linux/api/json', {
            'name': 'job-linux',
            'displayName': 'Linux x64',
        })])

        brockit.GenRustToolchain()._resolve_display_name(
            session, self.BASE, job)

        self.assertEqual(job.display_name, 'Linux x64')
        self.assertEqual(job.bot, 'Linux x64')

    def test_leaves_none_when_display_name_equals_job_name(self):
        """Jenkins echoes the job name as `displayName` when none is set; that
        must not be treated as a real display name."""
        job = self._job()
        session = _dispatching_session([('api/json', {
            'name': 'job-linux',
            'displayName': 'job-linux',
        })])

        brockit.GenRustToolchain()._resolve_display_name(
            session, self.BASE, job)

        self.assertIsNone(job.display_name)
        self.assertEqual(job.bot, 'job-linux')

    def test_leaves_none_on_request_failure(self):
        job = self._job()
        session = _dispatching_session([
            ('api/json', brockit.requests.RequestException('boom'))
        ])

        brockit.GenRustToolchain()._resolve_display_name(
            session, self.BASE, job)

        self.assertIsNone(job.display_name)


class PollJobTest(unittest.TestCase):
    """Drives the `_poll_job` state machine through its transitions."""

    QUEUE_URL = 'https://ci.brave.com/queue/item/1/'
    BUILD_URL = 'https://ci.brave.com/job/x/5/'

    def _job(self, **kwargs):
        defaults = {'job': 'job-linux', 'queue_url': self.QUEUE_URL}
        defaults.update(kwargs)
        return brockit._WatchedJob(**defaults)

    def test_still_queued_records_reason(self):
        job = self._job()
        session = _dispatching_session([('queue/item', {
            'cancelled': False,
            'executable': None,
            'why': 'Waiting for next available executor',
        })])

        brockit.GenRustToolchain()._poll_job(session, job)

        self.assertEqual(job.state, 'QUEUED')
        self.assertEqual(job.stage, 'Waiting for next available executor')
        self.assertIsNone(job.build_url)

    def test_cancelled_queue_item(self):
        job = self._job()
        session = _dispatching_session([('queue/item', {'cancelled': True})])

        brockit.GenRustToolchain()._poll_job(session, job)

        self.assertEqual(job.state, 'CANCELLED')

    def test_dequeue_then_running_stage(self):
        """Once an executor picks the job up, the build is resolved and the
        running stage is read from wfapi in the same poll."""
        job = self._job()
        session = _dispatching_session([
            ('queue/item', {
                'executable': {
                    'url': self.BUILD_URL
                }
            }),
            ('wfapi/describe', {
                'status': 'IN_PROGRESS',
                'durationMillis': 62000,
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
            }),
        ])

        brockit.GenRustToolchain()._poll_job(session, job)

        self.assertEqual(job.build_url, self.BUILD_URL)
        self.assertEqual(job.state, 'RUNNING')
        self.assertEqual(job.stage, 'build')
        self.assertEqual(job.elapsed, '1m02s')
        # The raw duration is also anchored so the row can tick between polls.
        self.assertEqual(job.duration_millis, 62000)
        self.assertIsNotNone(job.elapsed_anchor)

    def test_running_to_success(self):
        job = self._job(build_url=self.BUILD_URL, state='RUNNING')
        session = _dispatching_session([('wfapi/describe', {
            'status': 'SUCCESS',
            'durationMillis': 100000,
            'stages': [{
                'name': 's3-upload',
                'status': 'SUCCESS'
            }],
        })])

        brockit.GenRustToolchain()._poll_job(session, job)

        self.assertEqual(job.state, 'SUCCESS')
        self.assertEqual(job.stage, '(done)')
        self.assertEqual(job.elapsed, '1m40s')

    def test_falls_back_to_build_api_without_stage_view(self):
        """When wfapi is unavailable, state/result come from the plain build
        API instead."""
        job = self._job(build_url=self.BUILD_URL, state='RUNNING')
        session = _dispatching_session([
            ('wfapi/describe',
             brockit.requests.HTTPError('404 no stage view')),
            ('api/json', {
                'building': False,
                'result': 'FAILURE',
                'duration': 50000
            }),
        ])

        brockit.GenRustToolchain()._poll_job(session, job)

        self.assertEqual(job.state, 'FAILURE')
        self.assertEqual(job.stage, '(done)')
        self.assertEqual(job.elapsed, '50s')

    def test_terminal_job_is_not_repolled(self):
        job = self._job(state='SUCCESS')
        session = _dispatching_session([])

        brockit.GenRustToolchain()._poll_job(session, job)

        session.get.assert_not_called()

    def test_missing_queue_url_marks_unknown(self):
        job = self._job(queue_url='')
        session = _dispatching_session([])

        brockit.GenRustToolchain()._poll_job(session, job)

        self.assertEqual(job.state, 'UNKNOWN')
        session.get.assert_not_called()


class WatchLoopTest(unittest.TestCase):
    """Smoke test for `execute(watch=True)`: triggers all jobs, then polls
    until every pipeline reaches a terminal state."""

    def setUp(self):
        tmp = tempfile.TemporaryDirectory()
        self.addCleanup(tmp.cleanup)
        self.config_path = Path(tmp.name) / '.jenkins.json'
        self.config_path.write_text(json.dumps(VALID_CONFIG),
                                    encoding='utf-8',
                                    newline='')
        patcher = patch.object(brockit, 'JENKINS_CONFIG_FILE',
                               self.config_path)
        patcher.start()
        self.addCleanup(patcher.stop)

    @patch('brockit.time.sleep')
    @patch('brockit.Live')
    @patch('brockit.requests.Session')
    def test_watch_polls_until_all_terminal(self, session_cls, live_cls,
                                            sleep):
        # Every build is already SUCCESS on the first poll, so the loop should
        # break before ever sleeping.
        def _get(url, timeout=None):  # pylint: disable=unused-argument
            if 'crumbIssuer' in url:
                return _json_response({
                    'crumbRequestField': 'Jenkins-Crumb',
                    'crumb': 'x'
                })
            if 'queue/item' in url:
                return _json_response(
                    {'executable': {
                        'url': 'https://ci.brave.com/job/x/5/'
                    }})
            if 'wfapi/describe' in url:
                return _json_response({
                    'status': 'SUCCESS',
                    'durationMillis': 1000,
                    'stages': [{
                        'name': 'build',
                        'status': 'SUCCESS'
                    }],
                })
            raise brockit.requests.RequestException(url)

        post_resp = MagicMock()
        post_resp.headers = {'Location': 'https://ci.brave.com/queue/item/1/'}
        post_resp.raise_for_status.return_value = None
        session = MagicMock()
        session.post.return_value = post_resp
        session.get.side_effect = _get
        session_cls.return_value = session

        brockit.GenRustToolchain().execute(tag='150.0.7850.1', watch=True)

        # All four pipelines were triggered, the live display was entered, and
        # because every build was terminal on the first poll, we never slept.
        self.assertEqual(session.post.call_count,
                         len(brockit.RUST_TOOLCHAIN_JOBS))
        live_cls.assert_called()
        sleep.assert_not_called()


if __name__ == '__main__':
    unittest.main()
