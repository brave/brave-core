#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import textwrap
import unittest

import java_names

from java_names import (
    BROKEN,
    FIXED,
    OK,
    PRE_EXISTING,
    UNKNOWN_TYPE,
    UNRESOLVABLE,
    Literal,
    Verdict,
    bucket_of,
    check,
    class_key_of,
    collect_bytecode_test,
    collect_reflection,
    failing_buckets,
    parameters_match,
)
from java_source import JavaSource


class FakeRevision:
    """Stands in for a `Revision`, which `check` only asks to look up a class.

    The real one indexes two git trees; the only thing `check` needs from it
    is the parsed source for a class key.
    """

    def __init__(self, sources: dict[str, str]) -> None:
        self._sources = {
            key: JavaSource.parse(textwrap.dedent(text))
            for key, text in sources.items()
        }

    def lookup(self, class_key: str):
        outer = class_key.split('$', 1)[0]
        source = self._sources.get(outer)
        if source is None:
            return None, 'class not found'
        return source, f'{outer}.java'


class ClassKeyTest(unittest.TestCase):
    def test_dots_become_slashes(self):
        self.assertEqual(
            class_key_of('org/chromium/foo/Bar'), 'org/chromium/foo/Bar'
        )
        # `getClassForPath` accepts either separator, and one literal in
        # `BytecodeTest` mixes them.
        self.assertEqual(
            class_key_of('org.chromium/foo/Bar'), 'org/chromium/foo/Bar'
        )

    def test_a_nested_name_keeps_its_dollar(self):
        self.assertEqual(
            class_key_of('org.chromium/foo/Bar$Baz'), 'org/chromium/foo/Bar$Baz'
        )


class ParametersMatchTest(unittest.TestCase):
    def test_arity_must_agree(self):
        self.assertFalse(parameters_match(('int',), ('int', 'int')))

    def test_unknown_type_matches_anything(self):
        self.assertTrue(parameters_match(('Foo',), (UNKNOWN_TYPE,)))

    def test_types_must_agree_when_known(self):
        self.assertFalse(parameters_match(('Foo',), ('Bar',)))


class BucketTest(unittest.TestCase):
    PRESENT = Verdict(found=True, detail='present')
    ABSENT = Verdict(found=False, detail='no method x')
    UNKNOWN = Verdict(found=None, detail='class not found')

    def test_present_at_both_is_ok(self):
        self.assertEqual(bucket_of(self.PRESENT, self.PRESENT), OK)

    def test_lost_between_the_tags(self):
        self.assertEqual(bucket_of(self.PRESENT, self.ABSENT), BROKEN)

    def test_absent_at_both_is_pre_existing(self):
        self.assertEqual(bucket_of(self.ABSENT, self.ABSENT), PRE_EXISTING)

    def test_gained_between_the_tags(self):
        self.assertEqual(bucket_of(self.ABSENT, self.PRESENT), FIXED)

    def test_unknown_at_both_is_unresolvable(self):
        self.assertEqual(bucket_of(self.UNKNOWN, self.UNKNOWN), UNRESOLVABLE)

    def test_an_unreadable_old_tag_does_not_hide_a_break(self):
        self.assertEqual(bucket_of(self.UNKNOWN, self.ABSENT), PRE_EXISTING)

    def test_a_class_deleted_upstream_is_a_break_not_a_quiet_one(self):
        # The whole file going away is the most drastic rename there is, so it
        # must not be filed with the prebuilt literals nobody can resolve.
        self.assertEqual(bucket_of(self.PRESENT, self.UNKNOWN), BROKEN)

    def test_a_class_deleted_upstream_while_already_broken(self):
        self.assertEqual(bucket_of(self.ABSENT, self.UNKNOWN), PRE_EXISTING)


class FailingBucketsTest(unittest.TestCase):
    def test_comparing_two_revisions_ignores_older_breakage(self):
        self.assertEqual(failing_buckets('156.0.1.1', '157.0.1.1'), (BROKEN,))

    def test_auditing_one_revision_fails_on_pre_existing(self):
        # Otherwise a single-revision audit could never fail: nothing can break
        # between a revision and itself.
        self.assertIn(PRE_EXISTING, failing_buckets('157.0.1.1', '157.0.1.1'))

    def test_no_bucket_is_failing_by_accident(self):
        for buckets in (
            failing_buckets('156.0.1.1', '157.0.1.1'),
            failing_buckets('157.0.1.1', '157.0.1.1'),
        ):
            self.assertNotIn(UNRESOLVABLE, buckets)
            self.assertNotIn(FIXED, buckets)
            self.assertNotIn(OK, buckets)


class CollectBytecodeTestTest(unittest.TestCase):
    SOURCE = textwrap.dedent(
        '''
        public class BytecodeTest {
            @Test
            public void testClassesExist() {
                Assert.assertTrue(classExists("org/chromium/foo/Bar"));
                Assert.assertTrue(
                        fieldExists(
                                "org/chromium/foo/Bar",
                                "mField",
                                true,
                                String.class));
                Assert.assertTrue(
                        methodExists(
                                "org/chromium/foo/Bar",
                                "doThing",
                                MethodModifier.STATIC,
                                void.class,
                                Context.class,
                                int[].class));
                Assert.assertTrue(
                        constructorsMatch(
                                "org/chromium/foo/Bar",
                                "org/chromium/foo/BraveBar",
                                Context.class,
                                BraveBar.getDelegateClass()));
            }

            private boolean classExists(String className) {
                return getClassForPath(className) != null;
            }
        }
        '''
    )

    def setUp(self):
        self.literals = collect_bytecode_test('BytecodeTest.java', self.SOURCE)

    def test_the_helper_declarations_are_not_collected(self):
        self.assertEqual(len(self.literals), 5)

    def test_class_assertion(self):
        self.assertEqual(
            self.literals[0],
            Literal(
                collector='bytecode-test',
                kind='class',
                class_key='org/chromium/foo/Bar',
                file='BytecodeTest.java',
                line=5,
            ),
        )

    def test_field_assertion(self):
        self.assertEqual(self.literals[1].kind, 'field')
        self.assertEqual(self.literals[1].member, 'mField')

    def test_method_assertion_reads_modifier_return_type_and_parameters(self):
        method = self.literals[2]
        self.assertEqual(method.member, 'doThing')
        self.assertTrue(method.is_static)
        self.assertEqual(method.return_type, 'void')
        self.assertEqual(method.parameters, ('Context', 'int[]'))

    def test_constructors_match_yields_both_ends(self):
        upstream, brave = self.literals[3], self.literals[4]
        self.assertEqual(upstream.class_key, 'org/chromium/foo/Bar')
        self.assertEqual(brave.class_key, 'org/chromium/foo/BraveBar')
        # The second argument is a `Class<?>` from a helper, not a `.class`
        # literal, so only its position is known.
        self.assertEqual(upstream.parameters, ('Context', UNKNOWN_TYPE))


class CollectReflectionTest(unittest.TestCase):
    SOURCE = textwrap.dedent(
        '''
        package org.chromium.chrome.browser.settings;

        import org.chromium.components.browser_ui.site_settings.SiteSettings;

        class BraveSettings extends SiteSettings {
            void apply(Preference preference) {
                BraveReflectionUtil.invokeMethod(
                        SiteSettings.class,
                        this,
                        "setupPreference",
                        Preference.class,
                        preference,
                        boolean.class,
                        true);
                Object value =
                        BraveReflectionUtil.getField(
                                SiteSettings.class, "mValue", this);
                BraveReflectionUtil.invokeMethod(
                        getSomeClass(), this, "unreadable");
            }
        }
        '''
    )

    def setUp(self):
        self.literals = collect_reflection('BraveSettings.java', self.SOURCE)

    def test_a_non_literal_owner_is_skipped(self):
        self.assertEqual(len(self.literals), 2)

    def test_the_owner_resolves_through_the_files_imports(self):
        self.assertEqual(
            self.literals[0].class_key,
            'org/chromium/components/browser_ui/site_settings/SiteSettings',
        )

    def test_invoke_method_reads_the_interleaved_parameter_types(self):
        self.assertEqual(self.literals[0].member, 'setupPreference')
        self.assertEqual(self.literals[0].parameters, ('Preference', 'boolean'))

    def test_get_field(self):
        self.assertEqual(self.literals[1].kind, 'field')
        self.assertEqual(self.literals[1].member, 'mValue')

    def test_an_unimported_owner_falls_back_to_the_files_package(self):
        source = JavaSource.parse(self.SOURCE)
        self.assertEqual(
            java_names._qualify('Neighbour', source),
            'org/chromium/chrome/browser/settings/Neighbour',
        )


class CheckTest(unittest.TestCase):
    REVISION = FakeRevision(
        {
            'org/chromium/foo/Bar': '''
                class Bar {
                    private int mField;

                    Bar(Context context) {}

                    static void doThing(Context context, int[] ids) {}

                    void overloaded(int a) {}

                    void overloaded(int a, int b) {}
                }
                ''',
            'org/chromium/foo/NoConstructor': '''
                class NoConstructor {
                }
                ''',
        }
    )

    def _check(self, **kwargs) -> Verdict:
        literal = Literal(
            collector='bytecode-test',
            file='BytecodeTest.java',
            line=1,
            **kwargs,
        )
        return check(literal, self.REVISION)

    def test_a_class_that_is_not_in_the_tree_is_unknown(self):
        verdict = self._check(kind='class', class_key='com/google/Absent')
        self.assertIsNone(verdict.found)

    def test_field_found(self):
        self.assertTrue(
            self._check(
                kind='field',
                class_key='org/chromium/foo/Bar',
                member='mField',
            ).found
        )

    def test_field_renamed(self):
        verdict = self._check(
            kind='field', class_key='org/chromium/foo/Bar', member='mGone'
        )
        self.assertIs(verdict.found, False)
        self.assertEqual(verdict.detail, 'no field mGone')

    def test_method_matching_on_every_attribute(self):
        self.assertTrue(
            self._check(
                kind='method',
                class_key='org/chromium/foo/Bar',
                member='doThing',
                parameters=('Context', 'int[]'),
                return_type='void',
                is_static=True,
            ).found
        )

    def test_a_method_that_gained_a_parameter(self):
        verdict = self._check(
            kind='method',
            class_key='org/chromium/foo/Bar',
            member='doThing',
            parameters=('Context',),
        )
        self.assertIs(verdict.found, False)
        self.assertEqual(verdict.detail, 'takes 2 parameters, not 1')

    def test_a_method_that_stopped_being_static(self):
        verdict = self._check(
            kind='method',
            class_key='org/chromium/foo/Bar',
            member='doThing',
            parameters=('Context', 'int[]'),
            is_static=False,
        )
        self.assertIs(verdict.found, False)
        self.assertEqual(verdict.detail, 'is static')

    def test_one_matching_overload_is_enough(self):
        self.assertTrue(
            self._check(
                kind='method',
                class_key='org/chromium/foo/Bar',
                member='overloaded',
                parameters=('int', 'int'),
            ).found
        )

    def test_a_declared_constructor(self):
        self.assertTrue(
            self._check(
                kind='constructor',
                class_key='org/chromium/foo/Bar',
                parameters=('Context',),
            ).found
        )

    def test_the_implicit_no_arg_constructor_counts_as_present(self):
        # `getDeclaredConstructor()` finds it even though no source declares
        # it, so a `constructorsMatch` with no parameters has to pass.
        verdict = self._check(
            kind='constructor',
            class_key='org/chromium/foo/NoConstructor',
            parameters=(),
        )
        self.assertTrue(verdict.found)

    def test_a_class_with_no_constructor_has_no_other_one(self):
        verdict = self._check(
            kind='constructor',
            class_key='org/chromium/foo/NoConstructor',
            parameters=('Context',),
        )
        self.assertIs(verdict.found, False)


if __name__ == '__main__':
    unittest.main()
