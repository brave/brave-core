# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""The `ci_issues` module API."""

from __future__ import annotations

from collections.abc import Iterable, Mapping, Sequence
from dataclasses import dataclass
from typing import TYPE_CHECKING, Any

from PB.recipe_modules.brave.ci_issues.properties import InputProperties
from recipe_api import RecipeApi
from recipe_modules.gh import Issue, IssueState, Label
from recipe_modules.sanitizer_findings import Findings
from test_results import SuiteOutcome

from . import policy, requests
from .policy import Action, Kind, Where
from .requests import IssueRequest

if TYPE_CHECKING:
    from recipe_modules import ci_issues

# The output property what was filed is reported in.
OUTPUT_PROPERTY = 'ci_issues'


@dataclass(frozen=True)
class Filed:
    """A request, and what was done to file it."""

    request: IssueRequest
    # What was done, and the issue or comment it is at; None when CI gave
    # no repository to file the request's kind in.
    action: Action | None = None
    url: str | None = None


@dataclass(frozen=True)
class Filing:
    """What a build had filed: each of its requests, and why it made none
    for what it withheld."""

    where: Where
    filed: tuple[Filed, ...] = ()
    withheld: tuple[str, ...] = ()

    def to_json(self) -> dict[str, Any]:
        return {
            'filed': [
                {
                    'key': entry.request.key,
                    'action': str(entry.action),
                    'url': entry.url,
                }
                for entry in self.filed
                if entry.action is not None
            ],
            'withheld': list(self.withheld),
        }


class CiIssuesApi(RecipeApi):
    """Files what a build found on GitHub: its sanitiser findings in the
    `sanitizer_repository` CI gives, and its failing tests in the
    `test_repository`, each bug on the issue it was filed on before, if
    any."""

    m: ci_issues.DEPS

    def __init__(self, properties: InputProperties) -> None:
        super().__init__()
        self._properties = properties

    def file(
        self,
        *,
        builder: str,
        findings: Findings,
        outcomes: Mapping[str, SuiteOutcome],
    ) -> Filing:
        """File the build's findings and failing tests, and report what was
        filed, and what withheld, as the `ci_issues` output property.

        Args:
            builder: The builder the build ran as.
            findings: Its sanitiser findings.
            outcomes: How each of its suites' tests did.

        Returns:
            What was done for each of the build's requests.
        """
        sanitizer_requests = requests.from_sanitizer_findings(findings)
        test_requests = requests.from_test_failures(
            outcomes,
            explained=(
                test for finding in findings.findings for test in finding.tests
            ),
        )
        where = Where(builder, self._properties.build_url)
        filed: list[Filed] = []
        with self.m.step.nest('file issues'):
            for repo, made in (
                (self._properties.sanitizer_repository, sanitizer_requests),
                (self._properties.test_repository, test_requests),
            ):
                if repo and made.requests:
                    filed.extend(self._file(repo, made.requests, where))
                else:
                    filed.extend(Filed(request) for request in made.requests)
            filing = Filing(
                where=where,
                filed=tuple(filed),
                withheld=(
                    *sanitizer_requests.withheld,
                    *test_requests.withheld,
                ),
            )
            step = self.m.step('filed', None)
            step.presentation.properties[OUTPUT_PROPERTY] = filing.to_json()
        return filing

    def _file(
        self, repo: str, made: Sequence[IssueRequest], where: Where
    ) -> list[Filed]:
        kinds = tuple(dict.fromkeys(request.kind for request in made))
        labels = [
            policy.CI_CONCERN,
            *(kind.label for kind in kinds),
            *(Label(name) for name in self._properties.labels),
        ]
        self.m.gh.ensure_labels(repo, labels)
        existing = self._existing(repo, kinds)
        return [
            self._file_one(repo, request, existing.get(request.key), where)
            for request in made
        ]

    def _file_one(
        self,
        repo: str,
        request: IssueRequest,
        issue: Issue | None,
        where: Where,
    ) -> Filed:
        labels = [
            policy.CI_CONCERN.name,
            request.kind.label.name,
            *self._properties.labels,
        ]
        if issue is None:
            created = self.m.gh.create_issue(
                repo,
                title=request.title,
                body=request.body(where),
                labels=labels,
            )
            return Filed(request, Action.CREATE, created.url)
        action = policy.ON_RECURRENCE.get(
            (issue.state, issue.state_reason), Action.COMMENT
        )
        if action is Action.REOPEN:
            self.m.gh.reopen_issue(repo, issue.number)
        if missing := [label for label in labels if label not in issue.labels]:
            self.m.gh.add_labels(repo, issue.number, missing)
        commented = self.m.gh.comment(
            repo, issue.number, request.comment(where)
        )
        return Filed(request, action, commented.url)

    def _existing(self, repo: str, kinds: Iterable[Kind]) -> dict[str, Issue]:
        """The issues of `kinds` filed in `repo` before, by key: for a key
        filed more than once, its open issue, or else its latest."""
        by_key: dict[str, Issue] = {}
        for kind in kinds:
            for issue in self.m.gh.list_issues(repo, labels=[kind.label.name]):
                key = policy.key_of(issue.body)
                if key is not None and _preferred(issue, by_key.get(key)):
                    by_key[key] = issue
        return by_key


def _preferred(issue: Issue, other: Issue | None) -> bool:
    """Whether `issue` is a key's issue rather than `other`: an open issue
    before a closed one, then the earliest open or the latest closed."""
    if other is None:
        return True
    if (issue.state is IssueState.OPEN) != (other.state is IssueState.OPEN):
        return issue.state is IssueState.OPEN
    if issue.state is IssueState.OPEN:
        return issue.number < other.number
    return issue.number > other.number
