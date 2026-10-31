#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import textwrap
import unittest

import repository

from java_source import (
    ClassIndex,
    ConstructorDeclaration,
    FieldDeclaration,
    JavaSource,
    MethodDeclaration,
    blank_comments_and_strings,
    simple_type_name,
    split_top_level,
)
from test.fake_chromium_repo import FakeChromiumRepo


def _source(text: str) -> JavaSource:
    return JavaSource.parse(textwrap.dedent(text))


class BlankCommentsAndStringsTest(unittest.TestCase):
    def test_offsets_are_preserved(self):
        text = 'int a; // {{{\nint b;\n'
        blanked = blank_comments_and_strings(text)
        self.assertEqual(len(blanked), len(text))
        self.assertEqual(blanked.count('\n'), text.count('\n'))
        self.assertNotIn('{', blanked)

    def test_block_comment_contents_go(self):
        blanked = blank_comments_and_strings('a /* }; */ b')
        self.assertEqual(blanked, 'a          b')

    def test_string_contents_go_but_quotes_stay(self):
        blanked = blank_comments_and_strings('f("a;b", \'}\')')
        self.assertEqual(blanked, 'f("' + ' ' * 3 + '", \' \')')

    def test_escaped_quote_does_not_end_the_string(self):
        text = r'f("a\"; b"); x'
        blanked = blank_comments_and_strings(text)
        # The escaped quote is content, so the `;` inside it goes too and the
        # `; x` after the call survives.
        self.assertEqual(blanked, 'f("' + ' ' * 6 + '"); x')

    def test_keep_strings_leaves_literals_alone(self):
        text = 'f("a;b") // c\n'
        kept = blank_comments_and_strings(text, keep_strings=True)
        self.assertTrue(kept.startswith('f("a;b")'))
        self.assertEqual(kept.strip(), 'f("a;b")')
        # Offsets have to line up with the fully blanked copy, since the
        # collectors slice values out of one using indexes found in the other.
        self.assertEqual(len(kept), len(blank_comments_and_strings(text)))

    def test_text_block_does_not_desynchronise_the_scan(self):
        # A text block holds a lone `"` and a `}`; both have to be blanked, or
        # everything after it is read as being inside a string.
        source = _source(
            '''
            class Foo {
                private static final String TEMPLATE =
                        """
                        { "a": "b" }\\
                        """;
                private static final String AFTER = "x";
            }
            '''
        )
        self.assertEqual(
            source.fields,
            (
                FieldDeclaration(owner='Foo', name='TEMPLATE'),
                FieldDeclaration(owner='Foo', name='AFTER'),
            ),
        )


class SplitTopLevelTest(unittest.TestCase):
    def test_generic_arguments_are_not_separators(self):
        self.assertEqual(
            split_top_level('Map<String, Integer> a, int b'),
            ['Map<String, Integer> a', ' int b'],
        )

    def test_other_separators(self):
        self.assertEqual(
            split_top_level('int a = f(1, 2)', '='), ['int a ', ' f(1, 2)']
        )


class SimpleTypeNameTest(unittest.TestCase):
    def test_generics_are_erased_and_packages_dropped(self):
        self.assertEqual(simple_type_name('java.util.List<Foo>'), 'List')

    def test_varargs_become_an_array(self):
        self.assertEqual(simple_type_name('String...'), 'String[]')

    def test_type_use_annotation_inside_a_qualified_name(self):
        # `ActionMode.@Nullable Callback` reaches here with the annotation
        # already stripped, leaving a gap around the dot.
        self.assertEqual(simple_type_name('ActionMode. Callback'), 'Callback')


class JavaSourceTest(unittest.TestCase):
    def test_package_and_imports(self):
        source = _source(
            '''
            package org.chromium.foo;

            import org.chromium.bar.Baz;
            import static org.chromium.bar.Qux.ALL;

            class Foo {}
            '''
        )
        self.assertEqual(source.package, 'org.chromium.foo')
        self.assertEqual(source.imports['Baz'], 'org.chromium.bar.Baz')
        self.assertEqual(source.imports['ALL'], 'org.chromium.bar.Qux.ALL')

    def test_nested_types_are_named_with_a_dollar(self):
        source = _source(
            '''
            class Outer {
                interface Listener {}

                static class Inner {
                    enum Mode { A, B }
                }
            }
            '''
        )
        self.assertEqual(
            source.type_names,
            frozenset(
                (
                    'Outer',
                    'Outer$Listener',
                    'Outer$Inner',
                    'Outer$Inner$Mode',
                )
            ),
        )

    def test_annotation_type_is_a_type(self):
        # `@interface` must not be mistaken for an annotation and stripped,
        # which would also lose every member declared after it.
        source = _source(
            '''
            class Foo {
                @IntDef({Mode.A, Mode.B})
                @Retention(RetentionPolicy.SOURCE)
                public @interface Mode {
                    int A = 0;
                    int B = 1;
                }

                private static final String AFTER = "x";
            }
            '''
        )
        self.assertIn('Foo$Mode', source.type_names)
        self.assertIn(
            FieldDeclaration(owner='Foo', name='AFTER'), source.fields
        )

    def test_methods_carry_parameters_return_type_and_staticness(self):
        source = _source(
            '''
            class Foo {
                public static Foo create(Context context, int[] ids) {
                    return null;
                }

                @Nullable
                private List<String> names(Map<String, Integer> m)
                        throws IOException {
                    return null;
                }

                <T> void generic(T value) {}
            }
            '''
        )
        self.assertEqual(
            source.methods,
            (
                MethodDeclaration(
                    owner='Foo',
                    name='create',
                    parameters=('Context', 'int[]'),
                    return_type='Foo',
                    is_static=True,
                ),
                MethodDeclaration(
                    owner='Foo',
                    name='names',
                    parameters=('Map',),
                    return_type='List',
                    is_static=False,
                ),
                MethodDeclaration(
                    owner='Foo',
                    name='generic',
                    parameters=('T',),
                    return_type='void',
                    is_static=False,
                ),
            ),
        )

    def test_constructors_are_separate_from_methods(self):
        source = _source(
            '''
            class Foo {
                Foo() {}

                public Foo(Context context) {}
            }
            '''
        )
        self.assertEqual(source.methods, ())
        self.assertEqual(
            source.constructors,
            (
                ConstructorDeclaration(owner='Foo', parameters=()),
                ConstructorDeclaration(owner='Foo', parameters=('Context',)),
            ),
        )

    def test_an_initialiser_is_a_field_not_an_anonymous_class(self):
        source = _source(
            '''
            class Foo {
                private final Runnable mTask =
                        new Runnable() {
                            @Override
                            public void run() {}
                        };
            }
            '''
        )
        self.assertEqual(
            source.fields, (FieldDeclaration(owner='Foo', name='mTask'),)
        )
        self.assertNotIn('run', [method.name for method in source.methods])

    def test_several_declarators_on_one_field(self):
        source = _source('class Foo { private int a, b; }')
        self.assertEqual(
            source.fields,
            (
                FieldDeclaration(owner='Foo', name='a'),
                FieldDeclaration(owner='Foo', name='b'),
            ),
        )

    def test_locals_inside_a_method_are_not_members(self):
        source = _source(
            '''
            class Foo {
                void run() {
                    int local = 0;
                    Runnable r = () -> {};
                }
            }
            '''
        )
        self.assertEqual(source.fields, ())
        self.assertEqual([method.name for method in source.methods], ['run'])


class ClassIndexTest(unittest.TestCase):
    """Covers resolution against a commit, which needs a real git repo."""

    JAVA_PATH = 'chrome/android/java/src/org/chromium/chrome/Foo.java'
    JAVA_TEXT = textwrap.dedent(
        '''
        package org.chromium.chrome;

        class Foo {
            void bar() {}
        }
        '''
    )

    def setUp(self):
        self.fake_chromium_src = FakeChromiumRepo()
        self.fake_chromium_src.setup()
        self.addCleanup(self.fake_chromium_src.cleanup)
        self.fake_chromium_src.write_and_stage_file(
            self.JAVA_PATH, self.JAVA_TEXT, self.fake_chromium_src.chromium
        )
        self.commit = self.fake_chromium_src.commit(
            'Add Foo.java', self.fake_chromium_src.chromium
        )

    def test_resolves_a_class_key_at_a_commit(self):
        index = ClassIndex(repository.chromium, self.commit)
        resolution = index.resolve('org/chromium/chrome/Foo')
        self.assertEqual(resolution.path, self.JAVA_PATH)
        source = index.source(resolution.path)
        self.assertEqual([method.name for method in source.methods], ['bar'])

    def test_a_nested_key_resolves_to_the_outer_file(self):
        index = ClassIndex(repository.chromium, self.commit)
        self.assertEqual(
            index.resolve('org/chromium/chrome/Foo$Inner').path,
            self.JAVA_PATH,
        )

    def test_a_package_that_does_not_match_the_path_is_not_found(self):
        index = ClassIndex(repository.chromium, self.commit)
        self.assertFalse(index.resolve('org/chromium/other/Foo').found)


if __name__ == '__main__':
    unittest.main()
