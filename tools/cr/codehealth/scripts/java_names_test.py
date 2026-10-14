#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Tests for java_names.py."""

from __future__ import annotations

from pathlib import Path
import unittest

import java_names
from java_names import JavaSource, Literal, LiteralCollector, Member, Result

_SOURCE = """\
package org.chromium.chrome.browser;

import java.util.List;

/** A class, with "strings" and comments that look like code: void x(); */
public class Foo<T extends Bar> {
    // void commented(int a);
    @VisibleForTesting
    @Nullable
    private static final int sCount = 1, sOther = 2;
    private Runnable mRunnable = () -> { int notAField; };
    public MediaNotificationInfo.@Nullable Builder mBuilder;

    public Foo(@NonNull Context context, List<String> items) {}

    protected <U> void generic(final U u, T t, int... values) {}

    static boolean isReady(Tab tab, @Nullable Tab parent) { return true; }

    void anonymous() {
        new Runnable() {
            public void run() {}
        };
    }

    enum State {
        A,
        B(1);
        State() {}
        State(int value) {}
        int value() { return 0; }
    }

    static class Inner {
        boolean done() { return false; }
    }
}
"""


class JavaSourceTest(unittest.TestCase):
    """Real ast-grep runs, on one file holding the cases that matter."""

    @classmethod
    def setUpClass(cls) -> None:
        cls.source = JavaSource(_SOURCE)

    def _members(self, *nested: str) -> dict[str, Member]:
        return {m.name: m for m in self.source.members(list(nested))}

    def test_methods_with_simple_parameter_types(self) -> None:
        members = self._members('Foo')
        self.assertEqual(
            members['isReady'],
            Member('method', 'isReady', ('Tab', 'Tab'), 'boolean', True),
        )

    def test_type_variables_and_varargs(self) -> None:
        generic = self._members('Foo')['generic']
        self.assertEqual(generic.params, ('?', '?', 'int[]'))
        self.assertFalse(generic.static)

    def test_constructors_drop_annotations_and_generics(self) -> None:
        self.assertEqual(
            self._members('Foo')['Foo'],
            Member('constructor', 'Foo', ('Context', 'List')),
        )

    def test_fields_including_lambda_and_annotated_types(self) -> None:
        members = self._members('Foo')
        self.assertEqual(members['sCount'].returns, 'int')
        self.assertTrue(members['sOther'].static)
        self.assertEqual(members['mRunnable'].returns, 'Runnable')
        self.assertEqual(members['mBuilder'].returns, 'Builder')
        self.assertNotIn('notAField', members)

    def test_comments_strings_and_anonymous_classes_are_left_out(self) -> None:
        members = self._members('Foo')
        self.assertNotIn('x', members)
        self.assertNotIn('commented', members)
        self.assertNotIn('run', members)

    def test_nested_types_and_enums(self) -> None:
        self.assertEqual(set(self._members('Foo', 'Inner')), {'done'})
        state = self.source.members(['Foo', 'State'])
        self.assertEqual(
            sorted(m.params for m in state if m.kind == 'constructor'),
            [(), ('int',)],
        )
        self.assertIn('value', {m.name for m in state})

    def test_unknown_type(self) -> None:
        self.assertIsNone(self.source.members(['Missing']))


class LiteralCheckTest(unittest.TestCase):
    _MEMBERS = [
        Member('method', 'go', ('Tab', 'int'), 'void', False),
        Member('method', 'make', ('?',), 'Tab', True),
        Member('constructor', 'Foo', ('Context',)),
        Member('field', 'mTab', returns='Tab'),
    ]

    def _check(self, **kwargs: object) -> str:
        defaults = {'family': 'bytecode', 'owner': 'a.Foo', 'where': 'x:1'}
        return Literal(**{**defaults, **kwargs}).check(self._MEMBERS)

    def test_matching_method(self) -> None:
        self.assertEqual(
            self._check(kind='method', member='go', params=('Tab', 'int')),
            'ok',
        )

    def test_mismatches_are_named(self) -> None:
        cases = {
            'member missing': {'member': 'gone', 'params': ()},
            'arity differs': {'member': 'go', 'params': ('Tab',)},
            'parameters differ': {'member': 'go', 'params': ('Tab', 'long')},
            'modifier or return type differs': {
                'member': 'go',
                'params': ('Tab', 'int'),
                'static': True,
            },
        }
        for status, kwargs in cases.items():
            with self.subTest(status):
                self.assertEqual(self._check(kind='method', **kwargs), status)

    def test_type_variables_and_computed_classes_need_review(self) -> None:
        self.assertEqual(
            self._check(kind='method', member='make', params=('Object',)),
            'review',
        )
        self.assertEqual(
            self._check(kind='method', member='go', params=('?', 'int')),
            'review',
        )

    def test_constructors_and_the_implicit_default(self) -> None:
        self.assertEqual(
            self._check(kind='constructor', params=('Context',)), 'ok'
        )
        self.assertEqual(
            Literal('bytecode', 'constructor', 'a.Bar', 'x:1').check([]),
            'ok',
        )

    def test_fields(self) -> None:
        self.assertEqual(self._check(kind='field', member='mTab'), 'ok')
        self.assertEqual(
            self._check(kind='field', member='mTab', returns='View'),
            'type differs',
        )


class LiteralCollectorTest(unittest.TestCase):
    def setUp(self) -> None:
        self.collector = LiteralCollector(Path('.'))

    def test_reflection_resolves_the_owner_through_imports(self) -> None:
        text = (
            'package org.chromium.chrome.browser.tabmodel;\n'
            'import org.chromium.chrome.browser.tab.Tab;\n'
            'import org.chromium.chrome.browser.tabmodel.TabModelImpl;\n'
            'class BraveFoo {\n'
            '  void f() {\n'
            '    BraveReflectionUtil.invokeMethod(\n'
            '        TabModelImpl.class, this, "shouldGroup",\n'
            '        Tab.class, tab, int.class, 1);\n'
            '    // BraveReflectionUtil.getField(Foo.class, "mGone", this);\n'
            '    BraveReflectionUtil.getField(Local.class, "mField", this);\n'
            '  }\n'
            '}\n'
        )
        literals = self.collector._reflections(Path('BraveFoo.java'), text)
        self.assertEqual(
            literals,
            [
                Literal(
                    'reflection',
                    'method',
                    'org.chromium.chrome.browser.tabmodel.TabModelImpl',
                    'BraveFoo.java:6',
                    'shouldGroup',
                    ('Tab', 'int'),
                ),
                Literal(
                    'reflection',
                    'field',
                    'org.chromium.chrome.browser.tabmodel.Local',
                    'BraveFoo.java:10',
                    'mField',
                ),
            ],
        )

    def test_bytecode_assertions(self) -> None:
        text = (
            'class BytecodeTest {\n'
            '  void test() {\n'
            '    Assert.assertTrue(methodExists("a/b/Foo", "go",\n'
            '        MethodModifier.STATIC, void.class, Tab.class,\n'
            '        BraveFoo.getHiddenClass()));\n'
            '    Assert.assertFalse(fieldExists("a/b/Foo$Inner", "mGone"));\n'
            '    Assert.assertTrue(constructorsMatch("a/Up", "a/BraveUp",\n'
            '        Context.class));\n'
            '  }\n'
            '  private boolean methodExists(String className) {}\n'
            '}\n'
        )
        literals = self.collector._assertions(Path('T.java'), text)
        self.assertEqual(
            [(lit.kind, lit.owner, lit.member, lit.params) for lit in literals],
            [
                ('method', 'a.b.Foo', 'go', ('Tab', '?')),
                ('field', 'a.b.Foo$Inner', 'mGone', ()),
                ('constructor', 'a.Up', '', ('Context',)),
                ('constructor', 'a.BraveUp', '', ('Context',)),
            ],
        )
        self.assertTrue(literals[0].static)
        self.assertEqual(literals[0].returns, 'void')
        self.assertTrue(literals[1].deleted_by_bytecode)
        self.assertTrue(literals[3].brave)


class ResultTest(unittest.TestCase):
    _LITERAL = Literal('bytecode', 'class', 'a.Foo', 'x:1')

    def test_buckets(self) -> None:
        cases = {
            ('ok', 'ok'): 'fine',
            ('ok', 'member missing'): 'broke',
            ('review', 'arity differs'): 'broke',
            ('member missing', 'member missing'): 'debt',
            ('arity differs', 'ok'): 'fixed',
            ('review', 'review'): 'review',
            ('class missing', 'class missing'): 'unresolvable',
            ('ok', 'class missing'): 'broke',
        }
        for (old, new), bucket in cases.items():
            with self.subTest(old=old, new=new):
                self.assertEqual(Result(self._LITERAL, old, new).bucket, bucket)

    def test_review_of_an_unchanged_class_is_counted(self) -> None:
        unchanged = Result(self._LITERAL, 'review', 'review', changed=False)
        self.assertEqual(unchanged.bucket, 'reviewed')
        self.assertNotIn('reviewed', java_names.NameCheck.LISTED)

    def test_only_breakage_fails(self) -> None:
        self.assertEqual(java_names.NameCheck.LISTED[0], 'broke')


if __name__ == '__main__':
    unittest.main()
