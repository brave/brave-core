# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Configuration schema and items composing brave-core's `.env` file."""

from __future__ import annotations

from config import ConfigGroup, Dict, Set, Static, config_item_context

# The remote build execution service.
RBE_SERVICE = 'rbe.ba.brave.com:443'


def BaseConfig(USE_REMOTEEXEC=False, SISO_CACHE_DIR=''):
    # Schema arguments come from the module's properties, see
    # `BraveCoreCheckoutApi.get_config_defaults`.
    return ConfigGroup(
        # A `None` value is left out of the rendered `.env`.
        dotenv=Dict(value_type=(str, type(None))),
        # The OSes whose deps every `pnpm run init`/`sync` checks out, as
        # depot_tools' `gclient` config names them; none for the host's alone.
        target_os=Set(str),
        USE_REMOTEEXEC=Static(bool(USE_REMOTEEXEC)),
        SISO_CACHE_DIR=Static(str(SISO_CACHE_DIR)),
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
def asan(c):
    c.dotenv['is_asan'] = 'true'


@config_ctx()
def android(c):
    c.target_os.add('android')


@config_ctx()
def linux(c):
    c.target_os.add('linux')


@config_ctx()
def brave(c):
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
