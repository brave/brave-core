#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Tests for `ci.JenkinsCi` and the `--watch` machinery.

The file is split in two layers:

* `FromConfigTest` exercises the pure `~/.jenkins.json` reader, pointing
  `ci.JENKINS_CONFIG_FILE` at a temp file. No network involved.

* `TriggerTest` drives `JenkinsCi.trigger` with a mocked `requests.Session`,
  asserting that every job URL is triggered with the right
  `buildWithParameters` URL, `CHROMIUM_TAG` parameter, auth, and CSRF crumb, and
  that a per-job failure surfaces as a `BadOutcomeException`.

The remaining classes unit-test the watch helpers (`_WatchedJob`,
`_ElapsedClock`, `_poll_job`, the cell renderers, and the watch loop).
"""

from __future__ import annotations

import json
import tempfile
import unittest
from pathlib import Path
from unittest.mock import MagicMock, patch

from rich.console import Console
from rich.spinner import Spinner
from rich.text import Text

import ci

# Two job URLs shaped like the frozen toolchain data (a view-scoped job path
# with a trailing slash). Using two proves `trigger` fans out over the list.
JOB_URLS = (
    'https://ci.brave.com/view/toolchains/job/job-linux/',
    'https://ci.brave.com/view/toolchains/job/job-win/',
)

# A valid Jenkins config: only username/token are required now (the server root
# is derived from the job URLs).
VALID_CONFIG = {'username': 'alice', 'token': 'secret-token'}


class FromConfigTest(unittest.TestCase):
    """Tests for `JenkinsCi.from_config`."""

    def setUp(self):
        tmp = tempfile.TemporaryDirectory()
        self.addCleanup(tmp.cleanup)
        self.config_path = Path(tmp.name) / '.jenkins.json'
        patcher = patch.object(ci, 'JENKINS_CONFIG_FILE', self.config_path)
        patcher.start()
        self.addCleanup(patcher.stop)

    def _write(self, data) -> None:
        self.config_path.write_text(json.dumps(data),
                                    encoding='utf-8',
                                    newline='')

    def test_reads_credentials(self):
        self._write({'username': 'bob', 'token': 'tok'})
        launcher = ci.JenkinsCi.from_config()
        # pylint: disable=protected-access
        self.assertEqual(launcher._username, 'bob')
        self.assertEqual(launcher._token, 'tok')

    def test_missing_file_raises(self):
        with self.assertRaises(ci.InvalidInputException):
            ci.JenkinsCi.from_config()

    def test_missing_field_raises(self):
        self._write({'username': 'bob'})
        with self.assertRaises(ci.InvalidInputException):
            ci.JenkinsCi.from_config()

    def test_empty_field_raises(self):
        """A present-but-empty field is treated as missing."""
        self._write({'username': '', 'token': 'tok'})
        with self.assertRaises(ci.InvalidInputException):
            ci.JenkinsCi.from_config()

    def test_invalid_json_raises(self):
        self.config_path.write_text('{not valid json',
                                    encoding='utf-8',
                                    newline='')
        with self.assertRaises(ci.InvalidInputException):
            ci.JenkinsCi.from_config()


class ServerRootTest(unittest.TestCase):
    """The crumb/auth server root is derived from the job URLs."""

    def test_scheme_and_host_only(self):
        # pylint: disable=protected-access
        self.assertEqual(ci.JenkinsCi._server_root(JOB_URLS),
                         'https://ci.brave.com')


class TriggerTest(unittest.TestCase):
    """End-to-end tests for `JenkinsCi.trigger`."""

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

    def _trigger(self, session, **kwargs):
        with patch('ci.requests.Session', return_value=session):
            ci.JenkinsCi('alice', 'secret-token').trigger(
                JOB_URLS, params={'CHROMIUM_TAG': '150.0.7850.1'}, **kwargs)

    def test_triggers_all_jobs(self):
        session = self._make_session()
        self._trigger(session)

        # Basic-auth credentials come straight from the launcher.
        self.assertEqual(session.auth, ('alice', 'secret-token'))

        # One POST per job URL, each at the right buildWithParameters URL.
        self.assertEqual(session.post.call_count, len(JOB_URLS))
        posted_urls = {call.args[0] for call in session.post.call_args_list}
        self.assertEqual(posted_urls,
                         {f'{url}buildWithParameters'
                          for url in JOB_URLS})

        # Every call carries the CHROMIUM_TAG param and the crumb header.
        for call in session.post.call_args_list:
            self.assertEqual(call.kwargs['params'],
                             {'CHROMIUM_TAG': '150.0.7850.1'})
            self.assertEqual(call.kwargs['headers'],
                             {'Jenkins-Crumb': 'deadbeef'})

    def test_crumb_fetched_from_derived_root(self):
        session = self._make_session()
        self._trigger(session)
        self.assertEqual(session.get.call_args.args[0],
                         'https://ci.brave.com/crumbIssuer/api/json')

    def test_missing_crumb_issuer_proceeds_without_header(self):
        """When the crumb issuer is unavailable, the builds are still triggered
        with no crumb header (API-token auth is crumb-exempt)."""
        session = self._make_session()
        session.get.side_effect = ci.requests.RequestException('no crumb')
        self._trigger(session)

        self.assertEqual(session.post.call_count, len(JOB_URLS))
        for call in session.post.call_args_list:
            self.assertEqual(call.kwargs['headers'], {})

    def test_job_failure_raises_bad_outcome(self):
        """A non-2xx response from any job trigger surfaces as a
        `BadOutcomeException`."""
        session = self._make_session()
        failing = MagicMock()
        failing.raise_for_status.side_effect = ci.requests.HTTPError(
            '403 Forbidden')
        session.post.return_value = failing

        with self.assertRaises(ci.BadOutcomeException):
            self._trigger(session)

    def test_single_job_trigger(self):
        """A single-URL toolchain (Xcode/Windows) triggers exactly one job."""
        session = self._make_session()
        with patch('ci.requests.Session', return_value=session):
            ci.JenkinsCi('alice', 'secret-token').trigger(
                (JOB_URLS[0], ), params={'CHROMIUM_TAG': '150.0.7850.1'})
        self.assertEqual(session.post.call_count, 1)

    def test_no_properties_omits_the_param(self):
        session = self._make_session()
        self._trigger(session)
        for call in session.post.call_args_list:
            self.assertNotIn('PROPERTIES', call.kwargs['params'])

    def test_properties_rendered_as_json(self):
        session = self._make_session()
        self._trigger(session, properties={'a': 1, 'b': 'x'})
        # Neither URL in JOB_URLS is a Windows job, so the JSON is verbatim.
        for call in session.post.call_args_list:
            self.assertEqual(call.kwargs['params']['PROPERTIES'],
                             '{"a": 1, "b": "x"}')

    def test_properties_escaped_only_for_windows_jobs(self):
        """Windows jobs escape every quote; other jobs keep the JSON as-is."""
        win = ('https://ci.brave.com/view/toolchains/job/'
               'brave-browser-rust-toolchain-aux-build-windows-x64/')
        linux = ('https://ci.brave.com/view/toolchains/job/'
                 'brave-browser-rust-toolchain-aux-build-linux-x64/')
        session = self._make_session()
        with patch('ci.requests.Session', return_value=session):
            ci.JenkinsCi('alice', 'secret-token').trigger(
                (win, linux),
                params={'CHROMIUM_TAG': '150.0.7850.1'},
                properties={'k': 'v'})

        by_url = {
            call.args[0]: call.kwargs['params']['PROPERTIES']
            for call in session.post.call_args_list
        }
        self.assertEqual(by_url[f'{win}buildWithParameters'],
                         '{\\"k\\": \\"v\\"}')
        self.assertEqual(by_url[f'{linux}buildWithParameters'], '{"k": "v"}')


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
        raise ci.requests.RequestException(f'unexpected GET {url}')

    session.get.side_effect = _get
    return session


class WatchedJobTest(unittest.TestCase):
    """Tests for the `_WatchedJob` helpers."""

    def _job(self, **kwargs):
        defaults = {'url': JOB_URLS[0], 'queue_url': 'q/'}
        defaults.update(kwargs)
        return ci._WatchedJob(**defaults)

    def test_name_parsed_from_url(self):
        self.assertEqual(self._job().name, 'job-linux')

    def test_bot_is_job_name(self):
        self.assertEqual(self._job().bot, 'job-linux')

    def test_bot_prefers_display_name(self):
        self.assertEqual(self._job(display_name='Linux x64').bot, 'Linux x64')

    def test_is_terminal(self):
        self.assertTrue(self._job(state='SUCCESS').is_terminal)
        self.assertTrue(self._job(state='FAILURE').is_terminal)
        self.assertFalse(self._job(state='RUNNING').is_terminal)
        self.assertFalse(self._job(state='QUEUED').is_terminal)

    def test_link_prefers_build_url(self):
        job = self._job(build_url='https://ci.brave.com/job/x/5/')
        self.assertEqual(job.link, 'https://ci.brave.com/job/x/5/')

    def test_link_falls_back_to_job_url(self):
        self.assertEqual(self._job().link, JOB_URLS[0])


class FormatDurationTest(unittest.TestCase):
    """Tests for `ci._format_duration`."""

    def test_empty_for_missing(self):
        self.assertEqual(ci._format_duration(None), '')
        self.assertEqual(ci._format_duration(0), '')

    def test_seconds_only(self):
        self.assertEqual(ci._format_duration(5000), '5s')

    def test_minutes_and_seconds_zero_padded(self):
        self.assertEqual(ci._format_duration(62000), '1m02s')
        self.assertEqual(ci._format_duration(100000), '1m40s')


class StateCellTest(unittest.TestCase):
    """Tests for `JenkinsCi._state_cell`."""

    def test_running_is_animated_spinner(self):
        self.assertIsInstance(ci.JenkinsCi._state_cell('RUNNING'), Spinner)

    def test_other_states_are_static_markup(self):
        cell = ci.JenkinsCi._state_cell('SUCCESS')
        self.assertIsInstance(cell, str)
        self.assertIn('SUCCESS', cell)


class LinkCellTest(unittest.TestCase):
    """The Build column renders URLs as styled, clickable hyperlinks."""

    def test_wraps_url_in_link_style(self):
        url = 'https://ci.brave.com/job/x/5/'
        cell = ci.JenkinsCi._link_cell(url)
        self.assertIsInstance(cell, Text)
        self.assertEqual(cell.plain, url)
        self.assertIn(f'link {url}', cell.style)
        self.assertIn('underline', cell.style)


class DimCellTest(unittest.TestCase):
    """`_dim_cell` mutes finished, non-successful rows except the State cell."""

    def test_passes_through_when_not_dim(self):
        self.assertEqual(ci.JenkinsCi._dim_cell('build', False), 'build')

    def test_wraps_string_in_dim(self):
        cell = ci.JenkinsCi._dim_cell('build', True)
        self.assertIsInstance(cell, Text)
        self.assertEqual(cell.plain, 'build')
        self.assertIn('dim', [span.style for span in cell.spans])

    def test_dims_existing_text_preserving_base_style(self):
        link = ci.JenkinsCi._link_cell('https://ci.brave.com/x/')
        cell = ci.JenkinsCi._dim_cell(link, True)
        self.assertIn('link https://ci.brave.com/x/', cell.style)
        self.assertIn('dim', [span.style for span in cell.spans])


class RenderTableDimTest(unittest.TestCase):
    """Finished rows render dim (whatever the outcome); running ones do not."""

    DIM = '\x1b[2m'  # SGR code Rich emits for the `dim` style.

    def _render(self, state):
        job = ci._WatchedJob(url=JOB_URLS[0],
                             queue_url='',
                             build_url='https://ci.brave.com/job/x/5/',
                             state=state,
                             stage='(done)',
                             elapsed='5m00s')
        table = ci.JenkinsCi('u', 't')._render_table('Rust toolchain', [job])
        console = Console(force_terminal=True, width=200)
        with console.capture() as capture:
            console.print(table)
        return capture.get()

    def test_failure_row_is_dimmed(self):
        self.assertIn(self.DIM, self._render('FAILURE'))

    def test_aborted_row_is_dimmed(self):
        self.assertIn(self.DIM, self._render('ABORTED'))

    def test_success_row_is_dimmed(self):
        # A finished pipeline is dimmed regardless of outcome, so attention
        # stays on the ones still in flight.
        self.assertIn(self.DIM, self._render('SUCCESS'))

    def test_running_row_is_not_dimmed(self):
        self.assertNotIn(self.DIM, self._render('RUNNING'))


class ElapsedClockTest(unittest.TestCase):
    """`_ElapsedClock` ticks forward from its anchor on each render."""

    @patch('ci.time.monotonic')
    def test_ticks_from_anchor(self, monotonic):
        clock = ci._ElapsedClock(base_millis=60000, anchor=100.0)
        monotonic.return_value = 130.0  # 30s past the anchor
        self.assertEqual(clock.__rich__().plain, '1m30s')

    @patch('ci.time.monotonic')
    def test_uses_base_when_no_time_passed(self, monotonic):
        clock = ci._ElapsedClock(base_millis=5000, anchor=100.0)
        monotonic.return_value = 100.0
        self.assertEqual(clock.__rich__().plain, '5s')


class ElapsedCellTest(unittest.TestCase):
    """The Elapsed cell is a live clock only while RUNNING with a duration."""

    def _job(self, **kwargs):
        defaults = {'url': JOB_URLS[0], 'queue_url': ''}
        defaults.update(kwargs)
        return ci._WatchedJob(**defaults)

    def test_running_with_anchor_is_clock(self):
        job = self._job(state='RUNNING',
                        duration_millis=1000,
                        elapsed_anchor=10.0,
                        elapsed='1s')
        self.assertIsInstance(ci.JenkinsCi._elapsed_cell(job),
                              ci._ElapsedClock)

    def test_running_without_duration_is_static(self):
        """A RUNNING build whose duration hasn't resolved yet shows a dash, not
        a clock anchored to nothing."""
        self.assertEqual(
            ci.JenkinsCi._elapsed_cell(self._job(state='RUNNING')), '—')

    def test_terminal_shows_static_server_value(self):
        job = self._job(state='SUCCESS',
                        duration_millis=100000,
                        elapsed_anchor=10.0,
                        elapsed='1m40s')
        self.assertEqual(ci.JenkinsCi._elapsed_cell(job), '1m40s')


class ResolveDisplayNameTest(unittest.TestCase):
    """Tests for `JenkinsCi._resolve_display_name`."""

    def _job(self):
        return ci._WatchedJob(url=JOB_URLS[0], queue_url='')

    def test_sets_display_name_when_distinct_from_job_name(self):
        job = self._job()
        session = _dispatching_session([('/job/job-linux/api/json', {
            'name': 'job-linux',
            'displayName': 'Linux x64',
        })])

        ci.JenkinsCi('u', 't')._resolve_display_name(session, job)

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

        ci.JenkinsCi('u', 't')._resolve_display_name(session, job)

        self.assertIsNone(job.display_name)
        self.assertEqual(job.bot, 'job-linux')

    def test_leaves_none_on_request_failure(self):
        job = self._job()
        session = _dispatching_session([
            ('api/json', ci.requests.RequestException('boom'))
        ])

        ci.JenkinsCi('u', 't')._resolve_display_name(session, job)

        self.assertIsNone(job.display_name)


class PollJobTest(unittest.TestCase):
    """Drives the `_poll_job` state machine through its transitions."""

    QUEUE_URL = 'https://ci.brave.com/queue/item/1/'
    BUILD_URL = 'https://ci.brave.com/job/x/5/'

    def _job(self, **kwargs):
        defaults = {'url': JOB_URLS[0], 'queue_url': self.QUEUE_URL}
        defaults.update(kwargs)
        return ci._WatchedJob(**defaults)

    def test_still_queued_records_reason(self):
        job = self._job()
        session = _dispatching_session([('queue/item', {
            'cancelled': False,
            'executable': None,
            'why': 'Waiting for next available executor',
        })])

        ci.JenkinsCi('u', 't')._poll_job(session, job)

        self.assertEqual(job.state, 'QUEUED')
        self.assertEqual(job.stage, 'Waiting for next available executor')
        self.assertIsNone(job.build_url)

    def test_cancelled_queue_item(self):
        job = self._job()
        session = _dispatching_session([('queue/item', {'cancelled': True})])

        ci.JenkinsCi('u', 't')._poll_job(session, job)

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

        ci.JenkinsCi('u', 't')._poll_job(session, job)

        self.assertEqual(job.build_url, self.BUILD_URL)
        self.assertEqual(job.state, 'RUNNING')
        self.assertEqual(job.stage, 'build')
        self.assertEqual(job.elapsed, '1m02s')
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

        ci.JenkinsCi('u', 't')._poll_job(session, job)

        self.assertEqual(job.state, 'SUCCESS')
        self.assertEqual(job.stage, '(done)')
        self.assertEqual(job.elapsed, '1m40s')

    def test_falls_back_to_build_api_without_stage_view(self):
        """When wfapi is unavailable, state/result come from the plain build
        API instead."""
        job = self._job(build_url=self.BUILD_URL, state='RUNNING')
        session = _dispatching_session([
            ('wfapi/describe', ci.requests.HTTPError('404 no stage view')),
            ('api/json', {
                'building': False,
                'result': 'FAILURE',
                'duration': 50000
            }),
        ])

        ci.JenkinsCi('u', 't')._poll_job(session, job)

        self.assertEqual(job.state, 'FAILURE')
        self.assertEqual(job.stage, '(done)')
        self.assertEqual(job.elapsed, '50s')

    def test_terminal_job_is_not_repolled(self):
        job = self._job(state='SUCCESS')
        session = _dispatching_session([])

        ci.JenkinsCi('u', 't')._poll_job(session, job)

        session.get.assert_not_called()

    def test_missing_queue_url_marks_unknown(self):
        job = self._job(queue_url='')
        session = _dispatching_session([])

        ci.JenkinsCi('u', 't')._poll_job(session, job)

        self.assertEqual(job.state, 'UNKNOWN')
        session.get.assert_not_called()


class WatchLoopTest(unittest.TestCase):
    """Smoke test for `trigger(watch=True)`: triggers all jobs, then polls
    until every pipeline reaches a terminal state."""

    @patch('ci.time.sleep')
    @patch('ci.Live')
    @patch('ci.requests.Session')
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
            raise ci.requests.RequestException(url)

        post_resp = MagicMock()
        post_resp.headers = {'Location': 'https://ci.brave.com/queue/item/1/'}
        post_resp.raise_for_status.return_value = None
        session = MagicMock()
        session.post.return_value = post_resp
        session.get.side_effect = _get
        session_cls.return_value = session

        succeeded = ci.JenkinsCi('alice', 'secret-token').trigger(
            JOB_URLS,
            params={'CHROMIUM_TAG': '150.0.7850.1'},
            watch=True,
            title='Rust toolchain')

        self.assertEqual(session.post.call_count, len(JOB_URLS))
        live_cls.assert_called()
        sleep.assert_not_called()
        # Every pipeline was SUCCESS, so the watch reports success.
        self.assertTrue(succeeded)

    @patch('ci.time.sleep')
    @patch('ci.Live')
    @patch('ci.requests.Session')
    def test_watch_returns_false_when_a_pipeline_fails(self, session_cls,
                                                       live_cls, sleep):
        del live_cls, sleep

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
                    'status': 'FAILED',
                    'durationMillis': 1000,
                    'stages': [{
                        'name': 'build',
                        'status': 'FAILED'
                    }],
                })
            raise ci.requests.RequestException(url)

        post_resp = MagicMock()
        post_resp.headers = {'Location': 'https://ci.brave.com/queue/item/1/'}
        post_resp.raise_for_status.return_value = None
        session = MagicMock()
        session.post.return_value = post_resp
        session.get.side_effect = _get
        session_cls.return_value = session

        succeeded = ci.JenkinsCi('alice', 'secret-token').trigger(
            JOB_URLS, params={'CHROMIUM_TAG': '150.0.7850.1'}, watch=True)
        self.assertFalse(succeeded)

    def test_non_watch_trigger_reports_success(self):
        session = TriggerTest._make_session()
        with patch('ci.requests.Session', return_value=session):
            succeeded = ci.JenkinsCi('alice', 'secret-token').trigger(
                JOB_URLS, params={'CHROMIUM_TAG': '150.0.7850.1'})
        self.assertTrue(succeeded)


if __name__ == '__main__':
    unittest.main()
