# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Example recipe exercising the `brave_core_checkout` module."""

from __future__ import annotations

import post_process

DEPS = [
    'brave_core_checkout',
    'chromium_checkout',
    'env',
    'path',
    'raw_io',
    'step',
]


def RunSteps(api):
    mode = api.env.get('MODE')
    if mode == 'dotenv':
        api.brave_core_checkout.set_config('brave')
        api.brave_core_checkout.apply_config('asan')
        api.brave_core_checkout.ensure_checkout()
        return
    if mode == 'rbe':
        # Without the `use_remoteexec` property, RBE is opted into by item.
        api.brave_core_checkout.set_config('brave')
        api.brave_core_checkout.apply_config('rbe')
        api.brave_core_checkout.ensure_checkout()
        return
    if mode == 'ensure_checkout':
        api.brave_core_checkout.ensure_checkout()
        return
    if mode:
        api.brave_core_checkout.checkout()
        return
    api.brave_core_checkout.deploy('third_party/node')
    # Deploy the bootstrap shims and put them first on PATH for the block; the
    # prefix is dropped again once the `with` exits.
    with api.brave_core_checkout.bootstrap_on_path():
        api.step('npm version', ['npm', '--version'])


def GenTests(api):
    # Fresh clone: `.git` absent, so it clones and sparse-checks-out; the
    # requested paths are seeded so the post-checkout existence checks pass.
    yield api.test(
        'clone',
        api.brave_core_checkout.with_git_cache(),
        api.path.files('b/src/brave/third_party/node', 'b/src/brave/tools/cr'),
        api.post_process(
            post_process.MustRun, 'clone brave-core (shallow, sparse)'
        ),
        api.post_process(post_process.MustRun, 'sparse-checkout add'),
        api.post_process(post_process.MustRun, 'npm version'),
        api.post_process(post_process.StatusSuccess),
    )
    # Existing checkout: `.git` present, so it fetches/checks-out the ref
    # instead of cloning.
    yield api.test(
        'reuse',
        api.brave_core_checkout.with_git_cache(),
        api.path.dirs('b/src/brave/.git'),
        api.path.files('b/src/brave/third_party/node', 'b/src/brave/tools/cr'),
        api.post_process(post_process.MustRun, 'fetch brave-core ref'),
        api.post_process(post_process.MustRun, 'checkout brave-core ref'),
        api.post_process(
            post_process.DoesNotRun, 'clone brave-core (shallow, sparse)'
        ),
        api.post_process(post_process.StatusSuccess),
    )
    # Already deployed: the live sparse set already covers both requested
    # paths (`third_party/node` and `tools/cr`), so nothing is re-added.
    #
    # `deploy` runs once directly and once more via `bootstrap_on_path`, so the
    # sparse set is listed twice and each listing is seeded separately -- the
    # second is `sparse-checkout list (2)`, having been named in the same
    # namespace as the first.
    yield api.test(
        'already deployed',
        api.brave_core_checkout.with_git_cache(),
        api.path.files('b/src/brave/third_party/node', 'b/src/brave/tools/cr'),
        api.step_data(
            'sparse-checkout list',
            stdout=api.raw_io.output_text('third_party/node\ntools/cr\n'),
        ),
        api.step_data(
            'sparse-checkout list (2)',
            stdout=api.raw_io.output_text('third_party/node\ntools/cr\n'),
        ),
        api.post_process(post_process.DoesNotRun, 'sparse-checkout add'),
        api.post_process(post_process.DoesNotRun, 'sparse-checkout add (2)'),
        api.post_process(post_process.MustRun, 'npm version'),
        api.post_process(post_process.StatusSuccess),
    )
    # Requested path missing after checkout -> the module raises.
    yield api.test(
        'missing path',
        api.brave_core_checkout.with_git_cache(),
        api.post_process(post_process.StatusException),
        api.post_process(post_process.DropExpectation),
        status='EXCEPTION',
    )
    # Full checkout: a fresh clone from the git cache mirror, on a branch.
    yield api.test(
        'checkout branch',
        api.env.set('MODE', 'checkout'),
        api.brave_core_checkout.brave_core_ref('refs/heads/1.80.x'),
        api.brave_core_checkout.with_git_cache(),
        api.brave_core_checkout.git_cache_populated(),
        # A branch is mirrored anyway: no `--ref` for it.
        api.post_process(post_process.DoesNotRun, 'git cache populate (2)'),
        api.post_process(
            post_process.MustRun, 'brave-core clone from git cache'
        ),
        api.post_process(
            post_process.StepCommandContains,
            'brave-core checkout ref',
            ['origin/1.80.x'],
        ),
        api.post_process(
            post_process.MustRun, 'brave-core restore origin push url'
        ),
        api.post_process(post_process.StatusSuccess),
    )
    # A release tag is mirrored and checked out by its fully-qualified name.
    yield api.test(
        'checkout tag',
        api.env.set('MODE', 'checkout'),
        api.brave_core_checkout.brave_core_ref('refs/tags/v1.80.100'),
        api.brave_core_checkout.with_git_cache(),
        api.brave_core_checkout.git_cache_populated(),
        api.post_process(
            post_process.StepCommandContains,
            'git cache populate',
            ['--ref', 'refs/tags/v1.80.100'],
        ),
        api.post_process(
            post_process.StepCommandContains,
            'brave-core checkout tag',
            ['refs/tags/v1.80.100'],
        ),
        api.post_process(post_process.StatusSuccess),
    )
    # No `brave_core_ref` property: defaults to `master`.
    yield api.test(
        'checkout default ref',
        api.env.set('MODE', 'checkout'),
        api.brave_core_checkout.with_git_cache(),
        api.brave_core_checkout.git_cache_populated(),
        api.post_process(
            post_process.StepCommandContains,
            'brave-core checkout ref',
            ['origin/master'],
        ),
        api.post_process(post_process.StatusSuccess),
    )
    # Existing checkout: re-pointed at the mirror, then fetched to the tag.
    yield api.test(
        'checkout existing tag',
        api.env.set('MODE', 'checkout'),
        api.brave_core_checkout.brave_core_ref('refs/tags/v1.80.100'),
        api.brave_core_checkout.with_git_cache(),
        api.brave_core_checkout.git_cache_populated(),
        api.brave_core_checkout.existing_checkout(),
        api.post_process(
            post_process.MustRun, 'brave-core point origin at git cache'
        ),
        api.post_process(
            post_process.StepCommandContains,
            'brave-core fetch tag',
            ['refs/tags/v1.80.100:refs/tags/v1.80.100'],
        ),
        api.post_process(
            post_process.DoesNotRun, 'brave-core clone from git cache'
        ),
        api.post_process(post_process.StatusSuccess),
    )
    # Chromium missing: deployed through `chromium_checkout`, then
    # `pnpm run sync` runs. The mirror lookups are seeded per call, as
    # brave-core and Chromium each run `git cache exists`.
    yield api.test(
        'ensure_checkout deploys chromium',
        api.env.set('MODE', 'ensure_checkout'),
        api.brave_core_checkout.with_git_cache(),
        api.step_data(
            'git cache exists',
            stdout=api.raw_io.output_text('/b/cache/brave-core'),
        ),
        api.step_data(
            'git cache exists (2)',
            stdout=api.raw_io.output_text('/b/cache/chromium'),
        ),
        api.brave_core_checkout.chromium_tag('155.0.8059.16'),
        api.post_process(post_process.MustRun, 'clone from git cache'),
        api.post_process(
            post_process.StepCommandContains,
            'git cache populate (2)',
            ['--ref', 'refs/tags/155.0.8059.16'],
        ),
        api.post_process(
            post_process.StepCommandContains,
            'read chromium tag',
            ['show', 'refs/heads/master:package.json'],
        ),
        api.post_process(post_process.MustRun, 'pnpm run sync'),
        api.post_process(post_process.StatusSuccess),
    )
    # Chromium already deployed (`chrome/VERSION` present): no sync.
    yield api.test(
        'ensure_checkout chromium present',
        api.env.set('MODE', 'ensure_checkout'),
        api.brave_core_checkout.with_git_cache(),
        api.brave_core_checkout.git_cache_populated(),
        api.chromium_checkout.existing_checkout(),
        api.post_process(post_process.DoesNotRun, 'clone from git cache'),
        api.post_process(post_process.DoesNotRun, 'pnpm run sync'),
        api.post_process(post_process.StatusSuccess),
    )
    # The `.env` is composed from the module's properties and config items, and
    # written before `pnpm run sync`.
    yield api.test(
        'dotenv',
        api.env.set('MODE', 'dotenv'),
        api.brave_core_checkout.with_git_cache(),
        api.brave_core_checkout.git_cache_populated(),
        api.brave_core_checkout.chromium_mirror_populated(),
        api.brave_core_checkout.rbe(siso_cache_dir='/b/siso'),
        api.post_process(post_process.MustRun, 'write .env'),
        api.post_process(post_process.StatusSuccess),
    )
    # The `rbe` item can be applied by itself, whatever the properties say.
    yield api.test(
        'rbe item',
        api.env.set('MODE', 'rbe'),
        api.brave_core_checkout.with_git_cache(),
        api.brave_core_checkout.git_cache_populated(),
        api.brave_core_checkout.chromium_mirror_populated(),
        api.post_process(
            post_process.StepCommandContains,
            'write .env',
            ['rbe_service=rbe.ba.brave.com:443\nuse_remoteexec=true\n'],
        ),
        api.post_process(post_process.StatusSuccess),
    )
