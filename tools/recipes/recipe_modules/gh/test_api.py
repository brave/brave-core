# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Test API for `gh`: what GitHub answers, in its REST API's form."""

from __future__ import annotations

import json
from typing import Any

from recipe_test_api import RecipeTestApi, StepTestData

_REPO_URL = 'https://github.com/brave/test'


class GhTestApi(RecipeTestApi):
    @staticmethod
    def issue(
        number: int = 1,
        *,
        title: str = 'An issue',
        body: str = '',
        state: str = 'open',
        state_reason: str | None = None,
        labels: tuple[str, ...] = (),
        assignees: tuple[str, ...] = (),
    ) -> dict[str, Any]:
        """An issue, as GitHub's REST API describes one."""
        return {
            'number': number,
            'title': title,
            'body': body,
            'state': state,
            'state_reason': state_reason,
            'labels': [{'name': label} for label in labels],
            'assignees': [{'login': login} for login in assignees],
            'html_url': f'{_REPO_URL}/issues/{number}',
        }

    def issues(self, *issues: dict[str, Any]) -> StepTestData:
        """What listing issues answers: *issues*, one per line."""
        return self.m.raw_io.stream_output_text(
            ''.join(json.dumps(issue) + '\n' for issue in issues)
        )

    def created_issue(self, number: int = 1, **fields) -> StepTestData:
        """What creating or updating an issue answers: the issue."""
        return self.m.json.output_stream(self.issue(number, **fields))

    def comment(self, number: int) -> StepTestData:
        """What commenting on issue *number* answers: the comment."""
        return self.m.json.output_stream(
            {'html_url': f'{_REPO_URL}/issues/{number}#issuecomment-1'}
        )

    def labelled(self, names) -> StepTestData:
        """What labelling an issue answers: its labels."""
        return self.m.json.output_stream([{'name': name} for name in names])

    def created_label(self, name: str) -> StepTestData:
        """What creating label *name* answers: the label."""
        return self.m.json.output_stream({'name': name})

    def labels(self, *names: str) -> StepTestData:
        """What listing labels answers: labels *names*, one per line."""
        return self.m.raw_io.stream_output_text(
            ''.join(json.dumps({'name': name}) + '\n' for name in names)
        )
