# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""The `gh` module API."""

from __future__ import annotations

from collections.abc import Iterable, Sequence
import json
from typing import TYPE_CHECKING, Any
import urllib.parse

from recipe_api import RecipeApi

from .types import Comment, Issue, IssueState, Label, StateReason

if TYPE_CHECKING:
    from recipe_modules import gh


class GhApi(RecipeApi):
    """GitHub, through the `gh` CLI's REST API calls.

    Every call is a step of its own, so that what a build did on GitHub is
    in its steps. `gh` authenticates with the `GH_TOKEN` the CI environment
    gives the build; nothing here reads or passes it.

    Issues are listed through the repository's issues endpoint rather than
    GitHub's search, which is eventually consistent: an issue created
    moments before is always listed.
    """

    m: gh.DEPS

    def _call(
        self,
        name: str,
        path: str,
        *,
        method: str = 'GET',
        body: dict[str, Any] | None = None,
        step_test_data=None,
    ) -> Any:
        """The JSON GitHub answers a REST call with."""
        cmd = ['gh', 'api', '--method', method, path]
        if body is not None:
            cmd += ['--input', self.m.json.input(body)]
        return self.m.step(
            name,
            cmd,
            stdout=self.m.json.output(),
            step_test_data=step_test_data,
        ).stdout

    def _list(self, name: str, path: str, *, step_test_data=None) -> list[Any]:
        """Every item of a paginated REST list, a page at a time."""
        result = self.m.step(
            name,
            ['gh', 'api', '--paginate', '--jq', '.[]', path],
            stdout=self.m.raw_io.output_text(),
            step_test_data=step_test_data,
        )
        return [json.loads(line) for line in result.stdout.splitlines() if line]

    def list_issues(
        self, repo: str, *, labels: Sequence[str]
    ) -> tuple[Issue, ...]:
        """The issues of `repo`, open and closed, that have every one of
        `labels`, pull requests aside."""
        query = urllib.parse.urlencode(
            {'state': 'all', 'labels': ','.join(labels), 'per_page': 100}
        )
        items = self._list(
            f'list {",".join(labels)} issues ({repo})',
            f'repos/{repo}/issues?{query}',
            step_test_data=self.test_api.issues,
        )
        return tuple(
            Issue.from_api(item) for item in items if 'pull_request' not in item
        )

    def create_issue(
        self,
        repo: str,
        *,
        title: str,
        body: str,
        labels: Iterable[str] = (),
        assignees: Iterable[str] = (),
    ) -> Issue:
        """Creates an issue in `repo`."""
        data = self._call(
            f'create issue ({repo})',
            f'repos/{repo}/issues',
            method='POST',
            body={
                'title': title,
                'body': body,
                'labels': list(labels),
                'assignees': list(assignees),
            },
            step_test_data=lambda: self.test_api.created_issue(title=title),
        )
        return Issue.from_api(data)

    def comment(self, repo: str, number: int, body: str) -> Comment:
        """Comments `body` on issue `number` of `repo`."""
        data = self._call(
            f'comment on issue #{number} ({repo})',
            f'repos/{repo}/issues/{number}/comments',
            method='POST',
            body={'body': body},
            step_test_data=lambda: self.test_api.comment(number),
        )
        return Comment.from_api(data)

    def reopen_issue(self, repo: str, number: int) -> Issue:
        """Reopens issue `number` of `repo`."""
        data = self._call(
            f'reopen issue #{number} ({repo})',
            f'repos/{repo}/issues/{number}',
            method='PATCH',
            body={
                'state': IssueState.OPEN,
                'state_reason': StateReason.REOPENED,
            },
            step_test_data=lambda: self.test_api.created_issue(number=number),
        )
        return Issue.from_api(data)

    def add_labels(self, repo: str, number: int, labels: Iterable[str]) -> None:
        """Adds `labels` to issue `number` of `repo`."""
        self._call(
            f'label issue #{number} ({repo})',
            f'repos/{repo}/issues/{number}/labels',
            method='POST',
            body={'labels': list(labels)},
            step_test_data=lambda: self.test_api.labelled(labels),
        )

    def list_labels(self, repo: str) -> tuple[Label, ...]:
        """The labels `repo` defines."""
        items = self._list(
            f'list labels ({repo})',
            f'repos/{repo}/labels?per_page=100',
            step_test_data=self.test_api.labels,
        )
        return tuple(Label.from_api(item) for item in items)

    def ensure_labels(self, repo: str, labels: Iterable[Label]) -> None:
        """Creates those of `labels` that `repo` does not define yet."""
        existing = {label.name for label in self.list_labels(repo)}
        for label in labels:
            if label.name in existing:
                continue
            self._call(
                f'create label {label.name} ({repo})',
                f'repos/{repo}/labels',
                method='POST',
                body=label.to_api(),
                step_test_data=lambda label=label: self.test_api.created_label(
                    label.name
                ),
            )
            existing.add(label.name)
