# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
#
# For some of the code lifted below.
#
# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""Configuration schema and items for a brave-core build.

A build runs as a builder: `configure_build()` sets the `brave` config from
the builder's spec, as upstream's `chromium_tests.configure_build()` sets the
`chromium` one, and `pnpm run sync|build` then run with `--builder`, so that
the builder's own configuration decides what is synced and built. Its `.env`
only carries the BUILDER_DOTENV_KEYS, which stay machine-local.

Deprecated: without a builder, the `deprecated_*` config items compose the
whole `.env` instead.
"""

from __future__ import annotations

from config import (
    BadConf,
    ConfigGroup,
    Dict,
    Set,
    Static,
    config_item_context,
)

# The values the schema's platform variables take, as upstream's `chromium`
# config, less the platforms and architectures Brave does not build for.
HOST_PLATFORMS = ('linux', 'win', 'mac')
TARGET_PLATFORMS = HOST_PLATFORMS + ('ios', 'android')
HOST_TARGET_BITS = (32, 64)
HOST_ARCHS = ('intel', 'arm')
TARGET_ARCHS = HOST_ARCHS
BUILD_CONFIGS = ('Release', 'Debug')

# The only `.env` keys a build running as a builder writes: what has to stay
# machine-local. Mirrors `BUILDER_DOTENV_KEYS` in
# build/commands/lib/config.ts, which ignores any other with `--builder`.
BUILDER_DOTENV_KEYS = frozenset(
    {
        'rbe_service',
        'siso_cache_dir',
        'use_remoteexec',
    }
)

# The remote build execution service.
RBE_SERVICE = 'rbe.ba.brave.com:443'


def check(val, potentials):
    assert val in potentials, (val, potentials)
    return val


def BaseConfig(
    HOST_PLATFORM,
    DEFAULT_HOST_PLATFORM,
    HOST_ARCH,
    HOST_BITS,
    TARGET_PLATFORM,
    TARGET_ARCH,
    TARGET_BITS,
    BUILD_CONFIG,
    BUILDER,
    USE_REMOTEEXEC,
    SISO_CACHE_DIR,
    DISABLE_DEPRECATED_DOTENV,
):
    # Schema arguments come from the platform and the module's properties, see
    # `BraveCoreCheckoutApi.get_config_defaults`, and from the builder's spec,
    # see `BraveCoreCheckoutApi.configure_build`.
    return ConfigGroup(
        # A `None` value is left out of the rendered `.env`.
        dotenv=Dict(value_type=(str, type(None))),
        # The OSes whose deps every `pnpm run init`/`sync` checks out, as
        # depot_tools' `gclient` config names them; none for the host's alone.
        deprecated_target_os=Set(str),
        BUILD_CONFIG=Static(check(BUILD_CONFIG, BUILD_CONFIGS)),
        HOST_PLATFORM=Static(
            check(HOST_PLATFORM or DEFAULT_HOST_PLATFORM, HOST_PLATFORMS)
        ),
        HOST_PLATFORM_EXPLICIT=Static(bool(HOST_PLATFORM)),
        HOST_ARCH=Static(check(HOST_ARCH, HOST_ARCHS)),
        HOST_BITS=Static(check(HOST_BITS, HOST_TARGET_BITS)),
        TARGET_PLATFORM=Static(check(TARGET_PLATFORM, TARGET_PLATFORMS)),
        TARGET_ARCH=Static(check(TARGET_ARCH, TARGET_ARCHS)),
        TARGET_BITS=Static(check(TARGET_BITS, HOST_TARGET_BITS)),
        # The builder the build runs as, empty for the deprecated mode.
        BUILDER=Static(str(BUILDER)),
        USE_REMOTEEXEC=Static(bool(USE_REMOTEEXEC)),
        SISO_CACHE_DIR=Static(str(SISO_CACHE_DIR)),
        DISABLE_DEPRECATED_DOTENV=Static(bool(DISABLE_DEPRECATED_DOTENV)),
    )


def get_expected_host_platform(c):
    """Get the expected compiling platform for a config"""
    return {
        'android': 'linux',
        'ios': 'mac',
    }.get(c.TARGET_PLATFORM, c.TARGET_PLATFORM)


def validate_config(c, actual_host_platform):
    """Validate the configuration.

    This will be called after brave_core_checkout.{set,make,apply}_config to
    validate the config object. The platform checks are upstream's
    `chromium` ones, for the platforms Brave builds for.
    """
    if c.HOST_PLATFORM != actual_host_platform:
        raise BadConf(
            f'Current platform: {actual_host_platform} does not match '
            f'expected platform: {c.HOST_PLATFORM}'
        )
    host_targ_tuples = [
        (c.HOST_PLATFORM, c.HOST_ARCH, c.HOST_BITS),
        (c.TARGET_PLATFORM, c.TARGET_ARCH, c.TARGET_BITS),
    ]

    for plat, arch, bits in host_targ_tuples:
        if plat == 'mac':
            if not (
                arch == 'intel' or (arch == 'arm' and bits == 64)
            ):  # pragma: no cover
                raise BadConf(
                    '%s/%s arch is not supported on %s' % (arch, bits, plat)
                )

    # Configs have assumed platforms based on their target. Only accept
    # deviations when the host_platform is set explicitly in the builder's
    # chromium_config. This requires builders that are "cross compiling" to
    # set their host_platform
    expected_compiler_platform = get_expected_host_platform(c)
    if (
        not c.HOST_PLATFORM_EXPLICIT
        and c.HOST_PLATFORM != expected_compiler_platform
    ):
        raise BadConf(
            f'Unexpectedly attempting to compile {c.TARGET_PLATFORM} '
            f'from {c.HOST_PLATFORM}. The chromium config '
            'HOST_PLATFORM was not set explicitly'
        )

    potential_platforms = {
        # host -> potential target platforms
        'win': ('win',),
        'mac': ('mac', 'ios'),
        'linux': ('linux', 'android', 'ios', 'mac', 'win'),
    }[c.HOST_PLATFORM]

    if c.TARGET_PLATFORM not in potential_platforms:
        raise BadConf(
            'Can not compile "%s" on "%s"'
            % (c.TARGET_PLATFORM, c.HOST_PLATFORM)
        )

    if c.HOST_BITS < c.TARGET_BITS:
        raise BadConf('host bits < targ bits')  # pragma: no cover

    if c.BUILDER:
        # What a builder's own configuration sets, the deprecated items must
        # not set as well.
        not_allowed = sorted(set(c.dotenv) - BUILDER_DOTENV_KEYS)
        if not_allowed:
            raise BadConf(
                f'builder {c.BUILDER}: .env keys {not_allowed} are the '
                'builder\'s to set'
            )
        if c.deprecated_target_os:
            raise BadConf(
                f'builder {c.BUILDER}: the OSes to check out are the '
                'builder\'s to set'
            )
    elif c.DISABLE_DEPRECATED_DOTENV:
        raise BadConf(
            'the deprecated .env mode is disabled; run the build as a builder'
        )


config_ctx = config_item_context(BaseConfig)


@config_ctx(is_root=True)
def BASE(c):
    c.dotenv['use_remoteexec'] = 'false'


@config_ctx()
def rbe(c):
    c.dotenv['use_remoteexec'] = 'true'
    c.dotenv['rbe_service'] = RBE_SERVICE
    # Left out when unset, for `config.ts` to default it.
    c.dotenv['siso_cache_dir'] = c.SISO_CACHE_DIR or None


@config_ctx()
def brave(c):
    # The configuration of a build running as a builder: `configure_build`
    # sets it, with the builder from the build's properties.
    if not c.BUILDER:
        raise BadConf('the brave config needs a BUILDER, see configure_build')
    if c.USE_REMOTEEXEC:
        rbe(c)


# Deprecated: the configuration of a build without a builder, composing the
# whole `.env` its `pnpm run init`/`sync`/`build` read.


@config_ctx()
def deprecated_asan(c):
    c.dotenv['is_asan'] = 'true'


@config_ctx()
def deprecated_android(c):
    c.deprecated_target_os.add('android')


@config_ctx()
def deprecated_linux(c):
    c.deprecated_target_os.add('linux')


@config_ctx()
def deprecated_brave(c):
    # The configuration of a brave-core checkout. `BASE` applies underneath; the
    # defaults to share between builds belong here.
    #
    # Every `pnpm run init`/`sync` reads it, the recipe's and those a run makes
    # later alike.
    c.dotenv['lean_sync'] = 'true'
    # `gn gen --check` on every build, so that a header dependency missing from
    # GN fails it rather than only `gn check` later.
    c.dotenv['brave_extra_gn_gen_opts'] = '--check'
    if c.USE_REMOTEEXEC:
        rbe(c)
