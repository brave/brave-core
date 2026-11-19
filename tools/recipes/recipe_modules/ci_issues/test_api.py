# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Test API for `ci_issues`: where CI has it file."""

from __future__ import annotations

from recipe_test_api import RecipeTestApi, TestData


class CiIssuesTestApi(RecipeTestApi):
    def repositories(
        self,
        *,
        sanitizer: str = 'brave/sanitizer-findings',
        test: str = 'brave/test-failures',
        build_url: str = 'https://ci.example/build/1',
        labels: tuple[str, ...] = (),
    ) -> TestData:
        """Have CI file in *sanitizer* and *test*, linking to *build_url*."""
        return self.properties(
            **{
                '$ci_issues': {
                    'sanitizer_repository': sanitizer,
                    'test_repository': test,
                    'build_url': build_url,
                    'labels': list(labels),
                }
            }
        )
