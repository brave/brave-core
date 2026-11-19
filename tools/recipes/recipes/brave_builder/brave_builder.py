# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Check out, build and test Brave, as the builder from the build's
properties."""

from __future__ import annotations

from dataclasses import dataclass

import post_process
from recipe_api import RecipeScriptApi
from recipe_modules import (
    brave_core_checkout,
    ci_issues,
    ci_notifications,
    brave_tests,
    brave_tests_builder_config,
    chromium_checkout,
    flake_analysis,
    isolate,
    path,
    platform,
    raw_io,
    sanitizer_findings,
)
from recipe_test_api import RecipeTestApi


@dataclass
class DEPS(RecipeScriptApi):
    brave_core_checkout: brave_core_checkout.API
    ci_issues: ci_issues.API
    ci_notifications: ci_notifications.API
    brave_tests: brave_tests.API
    brave_tests_builder_config: brave_tests_builder_config.API
    flake_analysis: flake_analysis.API
    isolate: isolate.API
    path: path.API
    sanitizer_findings: sanitizer_findings.API


@dataclass
class TEST_DEPS(RecipeTestApi):
    brave_core_checkout: brave_core_checkout.TEST_API
    brave_tests: brave_tests.TEST_API
    brave_tests_builder_config: brave_tests_builder_config.TEST_API
    chromium_checkout: chromium_checkout.TEST_API
    flake_analysis: flake_analysis.TEST_API
    isolate: isolate.TEST_API
    platform: platform.TEST_API
    raw_io: raw_io.TEST_API


def RunSteps(api: DEPS) -> None:
    builder = api.brave_tests_builder_config.lookup_builder()
    if builder is None:
        _deprecated_run_steps(api)
        return
    builder_id, builder_config = builder
    api.brave_core_checkout.configure_build(builder_id, builder_config)
    api.brave_core_checkout.ensure_checkout()
    api.brave_core_checkout.compile()
    gtests = api.brave_tests.gtests(builder_id)
    isolates = (
        api.isolate.isolate_tests(
            api.path.chromium_src,
            f'out/{builder_id.builder}',
            [test['test'] for test in gtests],
        )
        if api.isolate.enabled
        else {}
    )
    test_run = api.brave_tests.run_tests(builder_id)
    if isolates:
        analysis = api.flake_analysis.analyse(
            test_run,
            {
                test['name']: flake_analysis.Rerunnable(
                    isolates[test['test']], tuple(test.get('args', ()))
                )
                for test in gtests
            },
        )
        test_run = analysis.exonerate(test_run)
    findings = api.sanitizer_findings.report(test_run.sanitizer_reports)
    filing = api.ci_issues.file(
        builder=builder_id.builder,
        findings=findings,
        outcomes=test_run.outcomes,
    )
    api.ci_notifications.report(filing)
    api.brave_tests.raise_on_failure(test_run)


def _deprecated_run_steps(api: DEPS) -> None:
    """Deprecated: check out, build and test without a builder."""
    api.brave_core_checkout.set_config('deprecated_brave')
    api.brave_core_checkout.ensure_checkout()
    api.brave_core_checkout.compile()
    api.brave_core_checkout.run_tests()


def GenTests(api: TEST_DEPS):
    # Runs as the builder from its properties: `pnpm` syncs and builds what the
    # builder says, and the `.env` carries only what stays machine-local.
    yield api.test(
        'builder',
        api.brave_tests_builder_config.ci_build(builder='linux-x64-asan-brave'),
        api.brave_core_checkout.with_git_cache(),
        api.brave_core_checkout.git_cache_populated(),
        api.chromium_checkout.existing_checkout(),
        api.brave_core_checkout.rbe(siso_cache_dir='/b/siso'),
        api.platform.name('linux'),
        api.post_process(
            post_process.StepCommandContains,
            'pnpm run sync',
            ['pnpm', 'run', 'sync', '--builder', 'linux-x64-asan-brave'],
        ),
        api.post_process(
            post_process.StepCommandContains,
            'build',
            ['pnpm', 'run', 'build', '--builder', 'linux-x64-asan-brave'],
        ),
        api.brave_tests.test_spec('brave_unit_tests', 'brave_browser_tests'),
        api.post_process(
            post_process.StepCommandContains,
            'brave_unit_tests',
            [
                '--build-dir',
                '[WORKSPACE]/b/src/out/linux-x64-asan-brave',
                '--builder',
                'linux-x64-asan-brave',
                '--builder-name=linux-x64-asan-brave',
                '--builder-group=brave.sanitisers',
                '--test-type=brave_unit_tests',
                '--parse-gtest-output',
                'brave_unit_tests',
            ],
        ),
        api.post_process(post_process.MustRun, 'brave_browser_tests'),
        api.post_process(post_process.DoesNotRun, 'test brave_all_unit_tests'),
        api.post_process(post_process.DoesNotRun, 'isolate tests'),
        api.post_process(post_process.StatusSuccess),
    )
    # A builder with `archive_isolates` archives its suites before it runs
    # them, with the client certificate builds already use. It reruns a
    # failing test from its isolate and asks upstream about it: one flaky
    # or failing there too does not fail the build or get filed.
    yield api.test(
        'isolates',
        api.brave_tests_builder_config.ci_build(
            builder='linux-x64-official', builder_group='brave.linux'
        ),
        api.brave_core_checkout.with_git_cache(),
        api.brave_core_checkout.git_cache_populated(),
        api.chromium_checkout.existing_checkout(),
        api.brave_core_checkout.rbe(siso_cache_dir='/b/siso'),
        api.isolate.enabled(),
        api.properties.environ(
            RBE_tls_client_auth_cert='/certs/client.crt',
            RBE_tls_client_auth_key='/certs/client.key',
        ),
        api.platform.name('linux'),
        api.brave_tests.test_spec(
            'base_unittests',
            'brave_unit_tests',
            builder='linux-x64-official',
            builder_group='brave.linux',
        ),
        api.post_process(
            post_process.StepCommandContains,
            'isolate tests.base_unittests.mb isolate',
            ['out/linux-x64-official', 'base_unittests'],
        ),
        api.post_process(
            post_process.StepCommandContains,
            'isolate tests.brave_unit_tests.archive.upload',
            ['--service', 'rbe.ba.brave.com:443'],
        ),
        api.brave_tests.test_results(
            'base_unittests',
            {
                'name': 'Upstream.Fails',
                'flaky': False,
                'status': 'FAILURE',
                'message': 'Expected equality',
                'output': '',
            },
        ),
        api.step_data('base_unittests', retcode=1),
        api.flake_analysis.rerun('base_unittests', {'Upstream.Fails': 10}),
        api.flake_analysis.upstream_status(
            'base_unittests', 'Upstream.Fails', 'failing'
        ),
        api.post_process(
            post_process.MustRun,
            'flake analysis.base_unittests.rerun.download',
        ),
        api.post_process(post_process.MustRun, 'brave_unit_tests'),
        api.post_process(post_process.StatusSuccess),
        api.post_process(post_process.DropExpectation),
    )
    # Deprecated: fresh workspace without a builder: brave-core and Chromium
    # are deployed and synced, then built and tested.
    yield api.test(
        'fresh',
        api.brave_core_checkout.with_git_cache(),
        api.step_data(
            'git cache populate exists (after)',
            stdout=api.raw_io.output_text('/b/cache/brave-core'),
        ),
        api.step_data(
            'git cache populate exists (after) (2)',
            stdout=api.raw_io.output_text('/b/cache/chromium'),
        ),
        api.brave_core_checkout.brave_core_ref('refs/heads/1.80.x'),
        api.brave_core_checkout.rbe(siso_cache_dir='/b/siso'),
        api.post_process(
            post_process.StepCommandContains,
            'brave-core checkout ref',
            ['origin/1.80.x'],
        ),
        api.post_process(post_process.MustRun, 'write .env'),
        api.post_process(post_process.MustRun, 'pnpm run sync'),
        api.post_process(
            post_process.StepCommandContains, 'build', ['--target=brave:all']
        ),
        api.platform.name('linux'),
        api.post_process(post_process.MustRun, 'test brave_all_unit_tests'),
        api.post_process(post_process.MustRun, 'test brave_browser_tests'),
        api.post_process(
            post_process.MustRun, 'test brave_interactive_ui_tests'
        ),
        api.post_process(post_process.MustRun, 'test brave_network_tests'),
        api.post_process(
            post_process.StepCommandContains,
            'test brave_all_unit_tests',
            ['pnpm', 'run', 'test', 'brave_all_unit_tests'],
        ),
        api.post_process(post_process.StatusSuccess),
    )
    # Deprecated: Chromium already deployed: synced to brave-core's version,
    # then built and tested.
    yield api.test(
        'chromium present',
        api.brave_core_checkout.with_git_cache(),
        api.brave_core_checkout.git_cache_populated(),
        api.chromium_checkout.existing_checkout(),
        api.post_process(post_process.MustRun, 'pnpm run sync'),
        api.platform.name('mac'),
        api.post_process(post_process.MustRun, 'build'),
        api.post_process(
            post_process.StepCommandContains,
            'test brave_network_tests',
            ['pnpm', 'run', 'test', 'brave_network_tests'],
        ),
        api.post_process(post_process.StatusSuccess),
    )
    # A failing suite does not stop the others; the run still fails.
    yield api.test(
        'suite fails',
        api.brave_core_checkout.with_git_cache(),
        api.brave_core_checkout.git_cache_populated(),
        api.chromium_checkout.existing_checkout(),
        api.step_data('test brave_browser_tests', retcode=1),
        api.post_process(
            post_process.MustRun, 'test brave_interactive_ui_tests'
        ),
        api.post_process(post_process.StatusException),
        api.post_process(post_process.DropExpectation),
        status='EXCEPTION',
    )
    # A failing build stops the run before any tests.
    yield api.test(
        'build fails',
        api.brave_core_checkout.with_git_cache(),
        api.brave_core_checkout.git_cache_populated(),
        api.chromium_checkout.existing_checkout(),
        api.step_data('build', retcode=1),
        api.post_process(post_process.DoesNotRun, 'test brave_all_unit_tests'),
        api.post_process(post_process.StatusFailure),
        api.post_process(post_process.DropExpectation),
        status='FAILURE',
    )
