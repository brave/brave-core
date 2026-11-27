#!/usr/bin/env python
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import io
import os
import sys
import tempfile
import unittest
from contextlib import redirect_stdout
from unittest.mock import patch

dirname = os.path.dirname(os.path.realpath(__file__))
sys.path.insert(0, os.path.join(dirname, '..'))

import check_pr_template  # noqa: E402

REPO = 'brave/brave-core'

TEMPLATE = '''<!-- Add brave-browser issue below that this PR will resolve -->
- Resolves

<!-- CI-related labels comment block -->

## Checklist:

- [ ] Reviewed the relevant design docs
- [ ] Tests are included or updated
- [ ] Comments explaining the "why" and relevant docs updated
- [ ] Security or other review requested if applicable
- [ ] A ticket exists and auto-closing keywords are used
- [ ] A good PR/commit description is written
- [ ] Review feedback and "fixup" commits are squashed
- [ ] Appropriate labels added to the associated issue
- [ ] Checked the PR locally
- [ ] Ran `git rebase master` (if needed)
- [ ] I did not use AI to create this PR, or I have disclosed the AI tooling used
'''


def filled_body(ticked=False, count=11):
    body = ['- Resolves', '', '## Checklist:', '']
    for i in range(1, 12):
        if i > count:
            continue
        marker = '- [x] ' if ticked else '- [ ] '
        body.append(marker + TEMPLATE.splitlines()[6 + i][6:])
    body.append('')
    return '\n'.join(body) + '\n'


class CheckPullRequestTemplateTest(unittest.TestCase):

    def test_complete_when_template_preserved_and_ai_disclosed(self):
        self.assertTrue(
            check_pr_template.is_complete(filled_body(), TEMPLATE, '', ''))

    def test_complete_when_checkboxes_ticked(self):
        self.assertTrue(
            check_pr_template.is_complete(filled_body(ticked=True), TEMPLATE,
                                          '', ''))

    def test_incomplete_when_body_stripped(self):
        self.assertFalse(
            check_pr_template.is_complete(
                'Fixes a bug in the network stack.\n\n- Resolves brave/brave-browser#1\n',
                TEMPLATE, '', ''))

    def test_incomplete_when_body_empty(self):
        self.assertFalse(check_pr_template.is_complete('', TEMPLATE, '', ''))

    def test_incomplete_below_threshold_preservation(self):
        # 8 of 12 items (7 checkboxes + '## Checklist:') = 67% < 75%, AI disclosed
        body = filled_body(count=7) + 'Used an LLM to fix typos\n'
        self.assertFalse(check_pr_template.is_complete(body, TEMPLATE, '', ''))

    def test_complete_at_threshold_preservation(self):
        # 9 of 12 items (8 checkboxes + '## Checklist:') = exactly 75%, AI disclosed
        body = filled_body(count=8) + 'Used an LLM to fix typos\n'
        self.assertTrue(check_pr_template.is_complete(body, TEMPLATE, '', ''))

    def test_incomplete_when_ai_not_disclosed(self):
        body = filled_body().replace(
            '- [ ] I did not use AI to create this PR, or I have disclosed the AI tooling used\n',
            '')
        self.assertFalse(check_pr_template.is_complete(body, TEMPLATE, '', ''))

    def test_ai_disclosed_anywhere_in_body(self):
        body = filled_body(count=9).replace(
            '- [ ] Reviewed the relevant design docs',
            'Reviewed the relevant design docs (prompted an LLM for wording)')
        self.assertTrue(check_pr_template.is_complete(body, TEMPLATE, '', ''))

    def test_normalisation_drops_noise_lines(self):
        body = (
            '\r\n'
            '---\n'
            '<!-- a single line comment -->\n'
            '## Checklist:\n'
            '## Checklist:\n'
            '- [X] Reviewed the relevant design docs\n'
            '- [ ] Tests are included or updated\n'
            '- [ ] Comments explaining the "why" and relevant docs updated\n'
            '- [ ] Security or other review requested if applicable\n'
            '- [ ] A ticket exists and auto-closing keywords are used\n'
            '- [ ] A good PR/commit description is written\n'
            '- [ ] Review feedback and "fixup" commits are squashed\n'
            '- [ ] Appropriate labels added to the associated issue\n'
            '- [ ] Checked the PR locally\n'
            '- [ ] Ran `git rebase master` (if needed)\n'
            '- [ ] I did not use AI to create this PR, or I have disclosed the AI tooling used\n'
            '\n')
        self.assertTrue(check_pr_template.is_complete(body, TEMPLATE, '', ''))

    def test_template_items_include_headings_and_checkboxes_only(self):
        self.assertEqual(12, len(check_pr_template.template_items(TEMPLATE)))

    def test_revert_escape(self):
        body = 'Reverts brave/brave-core#123\r\n'
        self.assertTrue(
            check_pr_template.is_complete(body, TEMPLATE,
                                          'Revert "some change"', REPO))

    def test_revert_escape_requires_matching_repository(self):
        body = 'Reverts brave/brave-core#123\n'
        self.assertFalse(
            check_pr_template.is_complete(body, TEMPLATE,
                                          'Revert "some change"',
                                          'brave/brave-browser'))

    def test_revert_escape_requires_revert_title(self):
        body = 'Reverts brave/brave-core#123\n'
        self.assertFalse(
            check_pr_template.is_complete(body, TEMPLATE, 'Some change', REPO))

    def test_cli_prints_true(self):
        with tempfile.TemporaryDirectory() as tmp:
            body_path = os.path.join(tmp, 'body')
            template_path = os.path.join(tmp, 'template')
            with open(body_path, 'w', encoding='utf-8') as f:
                f.write(filled_body())
            with open(template_path, 'w', encoding='utf-8') as f:
                f.write(TEMPLATE)
            output = self.run_cli(body_path, template_path, 'Fix network bug',
                                  REPO)
            self.assertEqual('true', output)

    def test_cli_prints_false(self):
        with tempfile.TemporaryDirectory() as tmp:
            body_path = os.path.join(tmp, 'body')
            template_path = os.path.join(tmp, 'template')
            with open(body_path, 'w', encoding='utf-8') as f:
                f.write('just a description\n')
            with open(template_path, 'w', encoding='utf-8') as f:
                f.write(TEMPLATE)
            output = self.run_cli(body_path, template_path, 'Fix network bug',
                                  REPO)
            self.assertEqual('false', output)

    def test_cli_handles_invalid_utf8_without_crashing(self):
        with tempfile.TemporaryDirectory() as tmp:
            body_path = os.path.join(tmp, 'body')
            template_path = os.path.join(tmp, 'template')
            with open(body_path, 'wb') as f:
                f.write(b'\xff\xfe broken bytes\n')
            with open(template_path, 'w', encoding='utf-8') as f:
                f.write(TEMPLATE)
            output = self.run_cli(body_path, template_path, 'Fix network bug',
                                  REPO)
            self.assertEqual('false', output)

    def run_cli(self, body_path, template_path, title, repository):
        argv = [
            'check_pr_template.py', 'pull-request', body_path, template_path,
            title, repository
        ]
        out = io.StringIO()
        with redirect_stdout(out):
            check_pr_template.main(argv)
        return out.getvalue().strip()


if __name__ == '__main__':
    unittest.main()
