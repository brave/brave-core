# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""The `cas` module API."""

from __future__ import annotations

import contextlib
from collections.abc import Iterator, Sequence
from typing import TYPE_CHECKING

from PB.recipe_modules.brave.cas.properties import (
    EnvProperties,
    InputProperties,
)
from recipe_api import RecipeApi

if TYPE_CHECKING:
    from pathlib import Path

    import config_types
    from recipe_modules import cas

# The remote execution client's package, which `remotetool` comes in.
REMOTETOOL_PACKAGE = 'infra/rbe/client/${platform}'
REMOTETOOL_VERSION = 're_client_version:0.187.0.ad10b508-gomaip'

# The remote execution service Brave builds use, and its instance, when the
# `service` and `instance` properties are unset.
DEFAULT_SERVICE = 'rbe.ba.brave.com:443'
DEFAULT_INSTANCE = 'default'

# How many uploads and downloads `remotetool` runs at once. Its default, 500,
# leaves each too little of a slow link to finish in time.
CAS_CONCURRENCY = 50

# How often an upload is tried. A retry sends only what CAS does not have yet.
UPLOAD_ATTEMPTS = 3


class CasApi(RecipeApi):
    """Archives trees of files in a remote execution service's CAS, and
    downloads them back by their digest.

    The service is the one builds run remotely on, and it is reached as Siso
    reaches it for a build: over mTLS, with the client certificate and key
    `RBE_tls_client_auth_cert` and `RBE_tls_client_auth_key` name.
    """

    m: cas.DEPS

    def __init__(
        self, properties: InputProperties, env_properties: EnvProperties
    ) -> None:
        super().__init__()
        self._service = properties.service or DEFAULT_SERVICE
        self._instance = properties.instance or DEFAULT_INSTANCE
        self._cert = env_properties.RBE_TLS_CLIENT_AUTH_CERT
        self._key = env_properties.RBE_TLS_CLIENT_AUTH_KEY

    @property
    def instance(self) -> str:
        """The instance trees go to and come from."""
        return self._instance

    @contextlib.contextmanager
    def with_instance(self, instance: str) -> Iterator[None]:
        """Sets the instance while in context, then reverts it."""
        previous = self._instance
        try:
            self._instance = instance
            yield
        finally:
            self._instance = previous

    def _remotetool(
        self, operation: str
    ) -> list[str | Path | config_types.Path]:
        """`remotetool`, connected to the service, to run *operation*.

        Raises:
            ValueError: no client certificate and key.
        """
        if not self._cert or not self._key:
            raise ValueError(
                'the cas module needs RBE_tls_client_auth_cert and '
                'RBE_tls_client_auth_key'
            )
        return [
            self.m.cipd.ensure_tool(
                REMOTETOOL_PACKAGE, REMOTETOOL_VERSION, 'remotetool'
            ),
            '--service',
            self._service,
            '--instance',
            self._instance,
            # The client certificate authenticates; no per-call credentials.
            '--service_no_auth',
            '--tls_client_auth_cert',
            self._cert,
            '--tls_client_auth_key',
            self._key,
            '--cas_concurrency',
            str(CAS_CONCURRENCY),
            '--operation',
            operation,
        ]

    def archive(
        self,
        step_name: str,
        root: str | Path | config_types.Path,
        paths: Sequence[str],
        cwd: str | Path | config_types.Path | None = None,
    ) -> str:
        """Archives *paths*, files or directories, as one tree.

        Args:
            step_name: The step's name.
            root: The tree's root; every path must be inside it.
            paths: The files and directories to archive, relative to *cwd*.
                Each keeps its place relative to *root* in the tree.
            cwd: What *paths* are relative to; *root* if unset.

        Returns:
            The tree's digest, as `<hash>/<size>`.

        Raises:
            StepFailure: The upload failed `UPLOAD_ATTEMPTS` times.
        """
        with self.m.step.nest(step_name):
            stage = self.m.path.mkdtemp('cas')
            self.m.step(
                'stage',
                [
                    self.m.depot_tools.vpython3(),
                    self.resource('stage.py'),
                    '--root',
                    root,
                    '--cwd',
                    cwd or root,
                    '--stage',
                    stage,
                    '--paths-json',
                    self.m.json.input(list(paths)),
                ],
            )
            for _ in range(UPLOAD_ATTEMPTS):
                result = self.m.step(
                    'upload',
                    [
                        *self._remotetool('upload_dir'),
                        '--path',
                        stage,
                        '--json',
                        self.m.json.output(),
                    ],
                    check=False,
                    step_test_data=self.test_api.uploaded,
                )
                if result.retcode == 0:
                    break
            else:
                raise self.m.step.StepFailure(
                    f'{step_name}: the upload failed {UPLOAD_ATTEMPTS} times'
                )
        stats = result.json.output
        root_digest = stats['RootDigest']
        digest = f'{root_digest["Hash"]}/{root_digest["Size"]}'
        result.presentation.step_text = (
            f'{digest}\n{stats["InputFiles"]} files, '
            f'{stats["BytesTransferred"]} bytes sent'
        )
        return digest

    def download(
        self,
        step_name: str,
        digest: str,
        output_dir: str | Path | config_types.Path,
    ) -> None:
        """Downloads the tree *digest* names into *output_dir*.

        Args:
            step_name: The step's name.
            digest: The tree's digest, as `<hash>/<size>`.
            output_dir: Where the tree goes.
        """
        self.m.step(
            step_name,
            [
                *self._remotetool('download_dir'),
                '--digest',
                digest,
                '--path',
                output_dir,
            ],
        )
