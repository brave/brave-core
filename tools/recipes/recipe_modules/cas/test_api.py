# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Test API for `cas`: seed `remotetool`'s upload statistics."""

from __future__ import annotations

from recipe_test_api import RecipeTestApi, StepTestData, TestData

# What a simulated upload's tree digest is when a test does not pick one.
DEFAULT_HASH = '0' * 64


class CasTestApi(RecipeTestApi):
    """Seeds what `remotetool upload_dir` reports for `archive()`."""

    def uploaded(
        self, digest_hash: str = DEFAULT_HASH, size: int = 100, files: int = 1
    ) -> StepTestData:
        """`remotetool upload_dir`'s statistics for a tree of *files*."""
        return self.m.json.output(
            {
                'InputFiles': files,
                'RootDigest': {'Hash': digest_hash, 'Size': size},
                'BytesTransferred': files * 1000,
                'Error': '',
            }
        )

    def archived(
        self, step_name: str, digest_hash: str, size: int = 100
    ) -> TestData:
        """Seeds the archive *step_name* with the tree *digest_hash*."""
        return self.step_data(
            f'{step_name}.upload', self.uploaded(digest_hash, size)
        )

    def configured(self, service: str = 'rbe.example.com:443') -> TestData:
        """Configures the module for *service*, with a client certificate."""
        return self.properties(**{'$cas': {'service': service}}) + (
            self.properties.environ(
                RBE_tls_client_auth_cert='/certs/client.crt',
                RBE_tls_client_auth_key='/certs/client.key',
            )
        )
