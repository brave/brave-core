#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Tests for knowledge_base.py."""

from __future__ import annotations

import contextlib
import tempfile
import unittest
from dataclasses import dataclass, field
from pathlib import Path
from unittest import mock

import knowledge_base
from knowledge_base import Change, KnowledgeBase, Source
from test.fake_chromium_repo import FakeChromiumRepo


def _rule(name: str, severity: str = 'should_fix', extra: str = '') -> str:
    return (
        f'---\nname: {name}\nrubric: design\nseverity: {severity}\n{extra}'
        '---\n\n**Rule.** Do the thing. Then more.\n\n**Why.** Because.\n'
    )


def _diff(path: str, *lines: str, start: int = 1, old: str = '') -> str:
    """A diff of `path` with one hunk of `lines`, each with its marker."""
    header = f'diff --git a/{old or path} b/{path}\n'
    if old:
        header += f'rename from {old}\nrename to {path}\n'
    return (
        f'{header}--- a/{old or path}\n+++ b/{path}\n'
        f'@@ -{start},1 +{start},1 @@\n' + ''.join(f'{l}\n' for l in lines)
    )


@dataclass
class FakeBrave:
    """A `BraveTree` with the words, overrides and adapters a test gives."""

    words: set[str] = field(default_factory=set)
    overridden: dict[str, set[str]] = field(default_factory=dict)
    adapted: set[str] = field(default_factory=set)


class TextProtoTest(unittest.TestCase):
    def test_fields(self):
        fields = knowledge_base._TextProto.parse(
            '# A comment.\n'
            'mixins: "//a/COMMON_METADATA"\n'
            'os: LINUX\n'
            'buganizer_public: { component_id: 12 }\n'
            'agentic_review {\n'
            '  capability_groups {\n'
            '    path_expression: "a\\\\.cc" "|b\\.cc"\n'
            '    review_capability_skills: [ "x/SKILL.md", "y/SKILL.md" ]\n'
            '    overridden_by_brave: true\n'
            '  }\n'
            '}\n',
            'f',
        )
        self.assertEqual(['//a/COMMON_METADATA'], fields['mixins'])
        self.assertEqual(['LINUX'], fields['os'])
        self.assertEqual([{'component_id': ['12']}], fields['buganizer_public'])
        (group,) = fields['agentic_review'][0]['capability_groups']
        # Adjacent strings join; an escape the format doesn't define stays.
        self.assertEqual([r'a\.cc|b\.cc'], group['path_expression'])
        self.assertEqual(
            ['x/SKILL.md', 'y/SKILL.md'], group['review_capability_skills']
        )
        self.assertEqual([True], group['overridden_by_brave'])

    def test_errors(self):
        for text in ('a { b: "x"', 'a: ?', '"x": 1'):
            with self.subTest(text=text), self.assertRaises(ValueError):
                knowledge_base._TextProto.parse(text, 'f')


class ChangeTest(unittest.TestCase):
    def test_parse(self):
        diff = (
            'diff --git a/a.cc b/a.cc\n'
            'index 1..2 100644\n'
            '--- a/a.cc\n'
            '+++ b/a.cc\n'
            '@@ -3,2 +3,3 @@ void F() {\n'
            ' context\n'
            '-old\n'
            '+new\x0cpage\n'
            '+---- not a header\n'
            '\\ No newline at end of file\n'
            'diff --git a/x/old.h b/y/new.h\n'
            'similarity index 90%\n'
            'rename from x/old.h\n'
            'rename to y/new.h\n'
            'diff --git a/gone.txt b/gone.txt\n'
            'deleted file mode 100644\n'
            '--- a/gone.txt\n'
            '+++ /dev/null\n'
            '@@ -1 +0,0 @@\n'
            '-bye\n'
        )
        change = Change.parse(diff, subject='s', body=['b'])
        self.assertEqual(['a.cc', 'y/new.h', 'gone.txt'], list(change.files))
        a = change.files['a.cc']
        self.assertEqual(
            ['-old', '+new\x0cpage', '+---- not a header'], a.lines
        )
        self.assertEqual({4: 'new\x0cpage', 5: '---- not a header'}, a.added)
        self.assertEqual('x/old.h', change.files['y/new.h'].old_path)
        self.assertEqual(['-bye'], change.files['gone.txt'].lines)
        self.assertEqual(('s', ['b']), (change.subject, change.body))
        self.assertIsNone(change.read('a.cc'))

    def test_source(self):
        with tempfile.TemporaryDirectory() as tmp:
            (Path(tmp) / 'a.txt').write_text('x', encoding='utf-8')
            source = Source(Path(tmp))
            self.assertEqual('x', source.read('a.txt'))
            self.assertIsNone(source.read('missing.txt'))


class RuleTest(unittest.TestCase):
    def test_read(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / 'r.md').write_text(_rule('kb/r'), encoding='utf-8')
            rule = knowledge_base.Rule.read(root, 'r.md')
            self.assertEqual(
                ('kb/r', 'design', 'should_fix'),
                (rule.name, rule.rubric, rule.severity),
            )
            self.assertEqual('Do the thing.', rule.summary)
            self.assertTrue(rule.body.startswith('\n**Rule.**'))
            (root / 'bad.md').write_text('# No frontmatter\n', encoding='utf-8')
            with self.assertRaisesRegex(ValueError, 'frontmatter'):
                knowledge_base.Rule.read(root, 'bad.md')

    def test_summary_prefers_the_description(self):
        with tempfile.TemporaryDirectory() as tmp:
            (Path(tmp) / 'r.md').write_text(
                _rule('kb/r', extra='description: Short.\n'), encoding='utf-8'
            )
            rule = knowledge_base.Rule.read(Path(tmp), 'r.md')
            self.assertEqual('Short.', rule.summary)


class KnowledgeBaseTest(unittest.TestCase):
    """Matching, on a knowledge base of its own repository."""

    def setUp(self):
        stack = contextlib.ExitStack()
        self.addCleanup(stack.close)
        self.root = Path(stack.enter_context(tempfile.TemporaryDirectory()))
        (self.root / '.git').mkdir()
        self.kb = self.root / 'kb'
        self.kb.mkdir()
        knowledge_base._RULE_CACHE.clear()
        self.addCleanup(knowledge_base._RULE_CACHE.clear)
        self.brave = FakeBrave()
        stack.enter_context(
            mock.patch.object(knowledge_base, 'brave_tree', lambda: self.brave)
        )

    def _base(self, groups: str, *rules: str) -> KnowledgeBase:
        for name in rules:
            (self.kb / f'{name}.md').write_text(
                _rule(f'kb/{name}'), encoding='utf-8'
            )
        (self.kb / 'COMMON_METADATA').write_text(
            f'agentic_review {{\n{groups}}}\n', encoding='utf-8'
        )
        return KnowledgeBase(self.kb)

    def _match(self, base: KnowledgeBase, change: Change) -> dict:
        return {m.rule.name: m.files for m in base.match(change)}

    def test_paths_and_added_content(self):
        base = self._base(
            'capability_groups {\n'
            '  path_expression: ".*\\\\.cc"\n'
            '  path_expression_exclusion: ".*_test\\\\.cc"\n'
            '  content_patterns: "\\\\bFoo\\\\("\n'
            '  rules: "kb/a.md"\n'
            '}\n',
            'a',
        )
        change = Change.parse(
            _diff('x.cc', '-Foo(1);', '+Foo(2);')
            + _diff('x_test.cc', '+Foo(3);')
            + _diff('x.h', '+Foo(4);')
            + _diff('y.cc', '-Foo(5);')
        )
        # Only added lines count, without their marker, and only in files the
        # paths keep.
        self.assertEqual(
            {'kb/a': {'x.cc': ['+Foo(2);']}}, self._match(base, change)
        )
        self.assertTrue(base.match(change)[0].targeted)

    def test_diff_patterns_see_the_marker(self):
        base = self._base(
            'capability_groups {\n'
            '  diff_patterns: "^-.*ENABLED"\n'
            '  required_diff_patterns: "^\\\\+.*DISABLED"\n'
            '  rules: "kb/a.md"\n'
            '}\n',
            'a',
        )
        change = Change.parse(
            _diff('flip.cc', '-F(kA, ENABLED);', '+F(kA, DISABLED);')
            + _diff('removal.cc', '-F(kB, ENABLED);')
        )
        self.assertEqual(
            {'kb/a': {'flip.cc': ['-F(kA, ENABLED);']}},
            self._match(base, change),
        )

    def test_without_patterns_every_changed_line(self):
        base = self._base(
            'capability_groups {\n'
            '  path_expression: "ios/.*"\n'
            '  rules: "kb/a.md"\n'
            '}\n',
            'a',
        )
        change = Change.parse(_diff('ios/a.mm', '+x', '-y'))
        self.assertEqual(
            {'kb/a': {'ios/a.mm': ['+x', '-y']}}, self._match(base, change)
        )
        self.assertFalse(base.match(change)[0].targeted)

    def test_renamed_files_also_match_by_old_path(self):
        base = self._base(
            'capability_groups {\n'
            '  path_expression: "old/.*"\n'
            '  rules: "kb/a.md"\n'
            '}\n',
            'a',
        )
        change = Change.parse(_diff('new/a.cc', '-x', old='old/a.cc'))
        self.assertEqual(
            {'kb/a': {'new/a.cc': ['-x']}}, self._match(base, change)
        )

    def test_brave_predicates(self):
        settings = 'java/src/org/chromium/chrome/browser/settings'
        self.brave = FakeBrave(
            words={'used'},
            overridden={'ios/b.mm': {'patch'}},
            adapted={'org/chromium/chrome/browser/settings/Main'},
        )
        base = self._base(
            'capability_groups {\n'
            '  overridden_by_brave: true\n'
            '  rules: "kb/overridden.md"\n'
            '}\n'
            'capability_groups {\n'
            '  bytecode_adapted: true\n'
            '  rules: "kb/adapted.md"\n'
            '}\n'
            'capability_groups {\n'
            '  path_expression: ".*\\\\.java"\n'
            '  brave_mentions: "(\\\\w+)\\\\s*\\\\("\n'
            '  rules: "kb/mentions.md"\n'
            '}\n',
            'overridden',
            'adapted',
            'mentions',
        )
        change = Change.parse(
            _diff('ios/a.mm', '+x')
            + _diff('ios/b.mm', '+x')
            + _diff(f'{settings}/Main.java', '-void used();', '-void other();')
            + _diff(f'{settings}/Other.java', '-void other();')
        )
        self.assertEqual(
            {
                'kb/overridden': {'ios/b.mm': ['+x']},
                'kb/adapted': {
                    f'{settings}/Main.java': ['-void used();', '-void other();']
                },
                'kb/mentions': {f'{settings}/Main.java': ['-void used();']},
            },
            self._match(base, change),
        )

    def test_message_patterns(self):
        base = self._base(
            'capability_groups {\n'
            '  subject_patterns: "(?i)deprecat"\n'
            '  body_patterns: "^Fixed: \\\\d+"\n'
            '  subject_pattern_exclusion: "^\\\\[ash\\\\]"\n'
            '  rules: "kb/a.md"\n'
            '}\n',
            'a',
        )
        cases = {
            ('Deprecate Foo', ()): [],
            ('Other', ('x', 'Fixed: 12')): ['Fixed: 12'],
            ('[ash] Deprecate Bar', ()): None,
            ('Other', ('Bug: 1',)): None,
        }
        for (subject, body), message in cases.items():
            with self.subTest(subject=subject):
                matches = base.match(Change(subject=subject, body=list(body)))
                self.assertEqual(
                    message, matches[0].message if matches else None
                )
                if matches:
                    self.assertEqual({}, matches[0].files)

    def test_unconditional_and_tool_groups(self):
        base = self._base(
            'capability_groups {\n'
            '  rules: "kb/always.md"\n'
            '}\n'
            'capability_groups {\n'
            '  tool: "echo {old}"\n'
            '  rules: "kb/tool.md"\n'
            '}\n',
            'always',
            'tool',
        )
        self.assertEqual({'kb/always': {}}, self._match(base, Change()))
        always, tool = base.groups
        self.assertTrue(always.unconditional)
        self.assertFalse(tool.unconditional)

    def test_a_rule_merges_its_groups(self):
        base = self._base(
            'capability_groups {\n'
            '  path_expression: "a\\\\.cc"\n'
            '  rules: "kb/a.md"\n'
            '}\n'
            'capability_groups {\n'
            '  path_expression: "b\\\\.cc"\n'
            '  content_patterns: "y"\n'
            '  rules: "kb/a.md"\n'
            '}\n',
            'a',
        )
        change = Change.parse(_diff('a.cc', '+x') + _diff('b.cc', '+y', '+z'))
        matches = base.match(change)
        self.assertEqual(1, len(matches))
        match = matches[0]
        self.assertEqual({'a.cc': ['+x'], 'b.cc': ['+y']}, match.files)
        self.assertTrue(match.targeted)

    def test_a_bad_pattern_disables_its_group(self):
        base = self._base(
            'capability_groups {\n'
            '  content_patterns: "("\n'
            '  bogus: "x"\n'
            '  rules: "kb/a.md"\n'
            '}\n',
            'a',
        )
        (group,) = base.groups
        self.assertEqual(('bogus',), group.unknown)
        self.assertEqual(1, len(group.problems))
        self.assertEqual([], base.match(Change.parse(_diff('a.cc', '+('))))

    def test_ast_patterns_span_added_lines(self):
        base = self._base(
            'capability_groups {\n'
            '  path_expression: ".*\\\\.h"\n'
            '  ast_patterns: "class $C final {}"\n'
            '  ast_patterns: "{pattern: {context: '
            '\'int x = base::Bind($$$A);\', selector: call_expression}}"\n'
            '  rules: "kb/a.md"\n'
            '}\n'
            'capability_groups {\n'
            '  path_expression: ".*\\\\.gn"\n'
            '  ast_patterns: "\\"//base\\""\n'
            '  rules: "kb/a.md"\n'
            '}\n',
            'a',
        )
        tree = self.root / 'tree'
        tree.mkdir()
        (tree / 'a.h').write_text(
            'namespace foo {\n'
            'class BASE_EXPORT Bar final {};\n'
            'int y = base::Bind(\n'
            '    1);\n'
            '}\n',
            encoding='utf-8',
        )
        (tree / 'BUILD.gn').write_text(
            'source_set("x") {\n  deps = [ "//base" ]\n}\n', encoding='utf-8'
        )
        source = Source(tree)
        cases = {
            # The class, read as C++ through the export macro's blanking.
            2: ['+class BASE_EXPORT Bar final {};'],
            # The call, from its context.
            4: ['+    1);'],
            # Code no pattern matches.
            1: None,
        }
        for line, lines in cases.items():
            with self.subTest(line=line):
                text = (tree / 'a.h').read_text().splitlines()[line - 1]
                change = Change.parse(
                    _diff('a.h', f'+{text}', start=line), source=source
                )
                matches = self._match(base, change)
                self.assertEqual(
                    lines, matches['kb/a']['a.h'] if matches else None
                )
        change = Change.parse(
            _diff('BUILD.gn', '+  deps = [ "//base" ]', start=2), source=source
        )
        self.assertIn('kb/a', self._match(base, change))


class _FakeRepoTest(unittest.TestCase):
    def setUp(self):
        self.fake = FakeChromiumRepo()
        self.fake.setup()
        self.addCleanup(self.fake.cleanup)
        for cache in (
            knowledge_base.brave_tree.cache_clear,
            knowledge_base._RULE_CACHE.clear,
        ):
            cache()
            self.addCleanup(cache)

    def brave(self, path: str, text: str) -> None:
        self.fake.write_file(path, text, self.fake.brave)

    def chromium(self, path: str, text: str) -> None:
        self.fake.write_file(path, text, self.fake.chromium)


class BraveTreeTest(_FakeRepoTest):
    def test_overrides(self):
        self.chromium('chrome/browser/foo-bar/a_b.cc', '')
        self.chromium('v8/src/x.cc', '')
        self.brave('chromium_src/chrome/a.cc', '')
        self.brave('chromium_src/PRESUBMIT.py', '')
        self.brave('chromium_src/ui/b.html.ts.lit_mangler.ts', '')
        self.brave('rewrite/base/c.h.yaml', '')
        self.brave(
            'patches/net-d.cc.patch', 'diff --git a/net/d.cc b/net/d.cc\n'
        )
        self.brave('patches/chrome-browser-foo-bar-a_b.cc.patch', '')
        self.brave('patches/v8/src-x.cc.patch', '')
        tree = knowledge_base.BraveTree()
        self.assertEqual(
            {
                'chrome/a.cc': {'chromium_src'},
                'ui/b.html.ts': {'lit_mangler'},
                'base/c.h': {'plaster'},
                'net/d.cc': {'patch'},
                'chrome/browser/foo-bar/a_b.cc': {'patch'},
                'v8/src/x.cc': {'patch'},
            },
            tree.overridden,
        )
        # A new patch names its file in the lines it adds.
        self.assertEqual(
            ('net/e.cc', 'patch'),
            tree.override(
                'patches/net-e.cc.patch', ['+diff --git a/net/e.cc b/net/e.cc']
            ),
        )
        self.assertIsNone(tree.override('browser/a.cc'))


class ReviewTest(_FakeRepoTest):
    def setUp(self):
        super().setUp()
        self.brave(
            'docs/knowledge_base/rules/cxx/blocker.md',
            _rule('brave/blocker', 'blocking'),
        )
        self.brave(
            'docs/knowledge_base/rules/cxx/nit.md', _rule('brave/nit', 'nit')
        )
        self.brave(
            'docs/knowledge_base/rules/cxx/COMMON_METADATA',
            'agentic_review {\n'
            '  capability_groups {\n'
            '    path_expression: "(?:.*/)?[^/]*\\\\.cc"\n'
            '    rules: "docs/knowledge_base/rules/cxx/nit.md"\n'
            '  }\n'
            '  capability_groups {\n'
            '    path_expression: "chromium_src/.*"\n'
            '    content_patterns: "BRAVE_"\n'
            '    rules: "docs/knowledge_base/rules/cxx/blocker.md"\n'
            '  }\n'
            '}\n',
        )
        rules = 'agents/knowledge_base/rules'
        self.chromium(f'{rules}/general/style.md', _rule('general/style'))
        self.chromium(f'{rules}/general/sync.md', _rule('general/sync'))
        self.chromium(
            f'{rules}/general/COMMON_METADATA',
            'agentic_review {\n'
            '  capability_groups {\n'
            '    path_expression: "(?:.*/)?[^/]*\\\\.cc"\n'
            f'    rules: "{rules}/general/style.md"\n'
            '  }\n'
            '}\n',
        )
        # A directory's own knowledge base, mixed in, and a subdirectory
        # dropping the rule.
        self.chromium(
            'components/sync/COMMON_METADATA',
            'agentic_review {\n'
            '  capability_groups {\n'
            '    path_expression: "components/sync/.*"\n'
            f'    rules: "{rules}/general/sync.md"\n'
            '  }\n'
            '}\n',
        )
        self.chromium(
            'components/sync/DIR_METADATA',
            'mixins: "//components/sync/COMMON_METADATA"\nagentic_review {}\n',
        )
        self.chromium(
            'components/sync/test/DIR_METADATA',
            f'agentic_review {{ exclude_rules: "{rules}/general/sync.md" }}\n',
        )

    def _review(self, diff: str) -> dict:
        return {
            m.rule.name: m.files
            for m in knowledge_base.review(Change.parse(diff))
        }

    def test_brave_files_and_their_overrides(self):
        matches = knowledge_base.review(
            Change.parse(
                _diff('browser/a.cc', '+x')
                + _diff('chromium_src/components/sync/b.cc', '+BRAVE_B')
                + _diff('chromium_src/components/sync/test/c.cc', '+y')
            )
        )
        # Most severe first, then Brave's.
        self.assertEqual(
            ['brave/blocker', 'general/style', 'general/sync', 'brave/nit'],
            [m.rule.name for m in matches],
        )
        files = {m.rule.name: list(m.files) for m in matches}
        self.assertEqual(
            ['chromium_src/components/sync/b.cc'], files['general/sync']
        )
        self.assertEqual(3, len(files['general/style']))
        self.assertTrue(
            matches[0].rule.url.startswith(knowledge_base._BRAVE_URL)
        )
        self.assertTrue(
            matches[2].rule.url.startswith(knowledge_base._CHROMIUM_URL)
        )

    def test_a_patch_is_matched_by_the_lines_it_adds(self):
        self.chromium('components/sync/d.cc', '')
        patch = _diff(
            'patches/components-sync-d.cc.patch',
            '+diff --git a/components/sync/d.cc b/components/sync/d.cc',
            '++++ b/components/sync/d.cc',
            '++  BRAVE_D();',
            '+ context',
        )
        self.assertEqual(
            {'patches/components-sync-d.cc.patch': ['+  BRAVE_D();']},
            self._review(patch)['general/sync'],
        )


class LintTest(_FakeRepoTest):
    def test_problems(self):
        rules = 'docs/knowledge_base/rules/x'
        self.brave(f'{rules}/good.md', _rule('brave/good'))
        self.brave(
            f'{rules}/bad.md',
            '---\nname: brave/other\nrubric: design\n---\n\nNo paragraphs.\n',
        )
        self.brave(f'{rules}/orphan.md', _rule('brave/good'))
        # A directory of rules whose metadata isn't written yet.
        self.brave('docs/knowledge_base/rules/y/new.md', _rule('brave/new'))
        self.brave(
            f'{rules}/COMMON_METADATA',
            'agentic_review {\n'
            '  capability_groups {\n'
            '    content_patterns: "("\n'
            '    bogus: 1\n'
            f'    rules: "{rules}/good.md"\n'
            f'    rules: "{rules}/bad.md"\n'
            f'    rules: "{rules}/missing.md"\n'
            '  }\n'
            '  capability_groups {}\n'
            '}\n',
        )
        problems = knowledge_base.lint()
        text = '\n'.join(problems)
        for expected in (
            'capability_groups[0]: unknown field bogus',
            "capability_groups[0]: content_patterns '('",
            f'{rules}/missing.md: no such file',
            'capability_groups[1]: names no rules or skills',
            f'{rules}/bad.md: no `severity`',
            'name brave/other does not end in the file stem',
            f'{rules}/bad.md: no **Rule.** paragraph',
            f'{rules}/orphan.md: no capability group names the rule',
            f'{rules}/orphan.md: name brave/good is also {rules}/good.md',
            'docs/knowledge_base/rules/y/new.md: no capability group names',
        ):
            self.assertIn(expected, text)
        self.assertFalse(
            [p for p in problems if p.startswith(f'{rules}/good.md')]
        )


if __name__ == '__main__':
    unittest.main()
