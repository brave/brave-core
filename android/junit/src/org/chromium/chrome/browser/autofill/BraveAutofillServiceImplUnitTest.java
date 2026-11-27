/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.autofill;

import static org.junit.Assert.assertArrayEquals;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertSame;
import static org.junit.Assert.assertTrue;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.when;

import android.app.assist.AssistStructure.ViewNode;
import android.view.View;
import android.view.autofill.AutofillId;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.annotation.Config;

import org.chromium.base.test.BaseRobolectricTestRunner;

import java.util.Arrays;
import java.util.Collections;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;
import java.util.Set;

/** Unit tests for pure utility methods in {@link BraveAutofillServiceImpl}. */
@RunWith(BaseRobolectricTestRunner.class)
@Config(manifest = Config.NONE)
public class BraveAutofillServiceImplUnitTest {

    // --- splitName ---

    @Test
    public void splitName_fullNameOnly_splitsIntoFirstAndLast() {
        assertArrayEquals(
                new String[] {"Jane", "Doe"},
                BraveAutofillServiceImpl.splitName("Jane Doe", null, null));
    }

    @Test
    public void splitName_singleWordFullName_firstNameOnly() {
        assertArrayEquals(
                new String[] {"Madonna", ""},
                BraveAutofillServiceImpl.splitName("Madonna", null, null));
    }

    @Test
    public void splitName_multiWordLastName_preservesRemainder() {
        assertArrayEquals(
                new String[] {"Anna", "Maria Van Der Berg"},
                BraveAutofillServiceImpl.splitName("Anna Maria Van Der Berg", null, null));
    }

    @Test
    public void splitName_existingPartsNotOverridden() {
        assertArrayEquals(
                new String[] {"Jonathan", "Smithson"},
                BraveAutofillServiceImpl.splitName("John Smith", "Jonathan", "Smithson"));
    }

    @Test
    public void splitName_allNull_returnsEmptyStrings() {
        assertArrayEquals(
                new String[] {"", ""}, BraveAutofillServiceImpl.splitName(null, null, null));
    }

    @Test
    public void splitName_emptyFullName_returnsEmptyStrings() {
        assertArrayEquals(
                new String[] {"", ""}, BraveAutofillServiceImpl.splitName("", null, null));
    }

    @Test
    public void splitName_fullNameWithExtraWhitespace_trims() {
        assertArrayEquals(
                new String[] {"Jane", "Doe"},
                BraveAutofillServiceImpl.splitName("  Jane   Doe  ", null, null));
    }

    @Test
    public void splitName_firstNameEmpty_lastNameProvided_derivesFirstOnly() {
        assertArrayEquals(
                new String[] {"Jane", "Smith"},
                BraveAutofillServiceImpl.splitName("Jane Doe", "", "Smith"));
    }

    // --- countryCodeToDisplayName ---

    @Test
    public void countryCodeToDisplayName_us_returnsUnitedStates() {
        assertEquals("United States", BraveAutofillServiceImpl.countryCodeToDisplayName("US"));
    }

    @Test
    public void countryCodeToDisplayName_gb_returnsUnitedKingdom() {
        assertEquals("United Kingdom", BraveAutofillServiceImpl.countryCodeToDisplayName("GB"));
    }

    @Test
    public void countryCodeToDisplayName_de_returnsGermany() {
        assertEquals("Germany", BraveAutofillServiceImpl.countryCodeToDisplayName("DE"));
    }

    @Test
    public void countryCodeToDisplayName_null_returnsEmptyString() {
        assertEquals("", BraveAutofillServiceImpl.countryCodeToDisplayName(null));
    }

    @Test
    public void countryCodeToDisplayName_emptyString_returnsEmpty() {
        assertEquals("", BraveAutofillServiceImpl.countryCodeToDisplayName(""));
    }

    @Test
    public void countryCodeToDisplayName_threeLetterCode_returnsUnchanged() {
        assertEquals("USA", BraveAutofillServiceImpl.countryCodeToDisplayName("USA"));
    }

    @Test
    public void countryCodeToDisplayName_lowercaseCode_returnsDisplayName() {
        // java.util.Locale handles lowercase country codes.
        assertEquals("United States", BraveAutofillServiceImpl.countryCodeToDisplayName("us"));
    }

    // --- matchListOption ---

    @Test
    public void matchListOption_exactMatch_returnsIndex() {
        CharSequence[] options = {"Alabama", "California", "Texas"};
        assertEquals(1, BraveAutofillServiceImpl.matchListOption(options, "California"));
    }

    @Test
    public void matchListOption_exactMatchCaseInsensitive_returnsIndex() {
        CharSequence[] options = {"Alabama", "California", "Texas"};
        assertEquals(1, BraveAutofillServiceImpl.matchListOption(options, "california"));
    }

    @Test
    public void matchListOption_optionStartsWithValue_returnsIndex() {
        // Pass 2: value "CA" matches option "California".
        CharSequence[] options = {"Alabama", "California", "Texas"};
        assertEquals(1, BraveAutofillServiceImpl.matchListOption(options, "CA"));
    }

    @Test
    public void matchListOption_valueStartsWithOption_returnsIndex() {
        // Pass 3: value "California" matches option "CA".
        CharSequence[] options = {"AL", "CA", "TX"};
        assertEquals(1, BraveAutofillServiceImpl.matchListOption(options, "California"));
    }

    @Test
    public void matchListOption_noMatch_returnsNegativeOne() {
        CharSequence[] options = {"Alabama", "California", "Texas"};
        assertEquals(-1, BraveAutofillServiceImpl.matchListOption(options, "New York"));
    }

    @Test
    public void matchListOption_nullOptions_returnsNegativeOne() {
        assertEquals(-1, BraveAutofillServiceImpl.matchListOption(null, "California"));
    }

    @Test
    public void matchListOption_emptyOptions_returnsNegativeOne() {
        assertEquals(
                -1, BraveAutofillServiceImpl.matchListOption(new CharSequence[0], "California"));
    }

    @Test
    public void matchListOption_singleCharValue_skipsPass2() {
        // Single character value should not match via prefix (pass 2 requires length >= 2).
        CharSequence[] options = {"Cat", "Car", "Cap"};
        assertEquals(-1, BraveAutofillServiceImpl.matchListOption(options, "C"));
    }

    @Test
    public void matchListOption_nullOptionElement_skipped() {
        // Null elements in the options array should be safely skipped without NPE.
        CharSequence[] options = {null, "California", "Texas"};
        assertEquals(1, BraveAutofillServiceImpl.matchListOption(options, "California"));
    }

    @Test
    public void matchListOption_exactMatchPreferredOverPrefix() {
        // "CA" should match index 0 (exact) not index 1 (prefix).
        CharSequence[] options = {"CA", "California"};
        assertEquals(0, BraveAutofillServiceImpl.matchListOption(options, "CA"));
    }

    @Test
    public void matchListOption_singleCharOption_skipsPass3() {
        // Pass 3 requires option length >= 2, so "C" is skipped.
        CharSequence[] options = {"C", "CALIF"};
        // "California" does not exact-match either, does not prefix-match either,
        // but starts with "CALIF" (pass 3).
        assertEquals(1, BraveAutofillServiceImpl.matchListOption(options, "California"));
    }

    // --- pickRequiredSaveField ---

    private static Map<String, AutofillId> mapWithKeys(String... keys) {
        Map<String, AutofillId> map = new HashMap<>();
        for (int i = 0; i < keys.length; i++) {
            map.put(keys[i], new AutofillId(i + 1));
        }
        return map;
    }

    @Test
    public void pickRequiredSaveField_noAddressComponent_returnsNull() {
        // The system prompt reads "Save address to Brave?" and Android shows it when the
        // activity finishes, so a form with no address component must not arm it at all.
        assertNull(
                BraveAutofillServiceImpl.pickRequiredSaveField(
                        mapWithKeys(
                                View.AUTOFILL_HINT_NAME,
                                View.AUTOFILL_HINT_EMAIL_ADDRESS,
                                View.AUTOFILL_HINT_PHONE)));
    }

    @Test
    public void pickRequiredSaveField_prefersStreetAddress() {
        Map<String, AutofillId> fields =
                mapWithKeys(
                        View.AUTOFILL_HINT_POSTAL_ADDRESS,
                        View.AUTOFILL_HINT_POSTAL_CODE,
                        View.AUTOFILL_HINT_NAME);
        assertSame(
                fields.get(View.AUTOFILL_HINT_POSTAL_ADDRESS),
                BraveAutofillServiceImpl.pickRequiredSaveField(fields));
    }

    @Test
    public void pickRequiredSaveField_postalCodeOnly_returnsPostalCode() {
        Map<String, AutofillId> fields =
                mapWithKeys(View.AUTOFILL_HINT_POSTAL_CODE, View.AUTOFILL_HINT_NAME);
        assertSame(
                fields.get(View.AUTOFILL_HINT_POSTAL_CODE),
                BraveAutofillServiceImpl.pickRequiredSaveField(fields));
    }

    // --- restrictToFocusedOrigin ---

    private static ViewNode mockViewNode(String webDomain) {
        ViewNode node = mock(ViewNode.class);
        when(node.getWebDomain()).thenReturn(webDomain);
        return node;
    }

    // The node BraveAutofillViewStructureParser#findFocusedNode would return — found by
    // searching the raw structure, so it may or may not also be a key in nodesMap/fields.
    private static ViewNode mockFocusedNode(String webDomain, AutofillId autofillId) {
        ViewNode node = mockViewNode(webDomain);
        when(node.getAutofillId()).thenReturn(autofillId);
        return node;
    }

    private static Set<String> setOf(String... values) {
        return new HashSet<>(Arrays.asList(values));
    }

    @Test
    public void restrictToFocusedOrigin_noFocusedNode_returnsFieldsUnchanged() {
        Map<String, AutofillId> fields = mapWithKeys(View.AUTOFILL_HINT_NAME);
        Map<AutofillId, ViewNode> nodesMap = Collections.emptyMap();

        assertSame(
                fields,
                BraveAutofillServiceImpl.restrictToFocusedOrigin(
                        fields, nodesMap, /* focusedNode= */ null, "com.example.app", setOf()));
    }

    @Test
    public void restrictToFocusedOrigin_sameOrigin_keepsAllFields() {
        // All fields in one real HTML form legitimately share the main frame's origin — must not
        // be reduced, or one-tap whole-form fill breaks.
        Map<String, AutofillId> fields =
                mapWithKeys(View.AUTOFILL_HINT_NAME, View.AUTOFILL_HINT_POSTAL_ADDRESS);
        AutofillId nameId = fields.get(View.AUTOFILL_HINT_NAME);
        AutofillId streetId = fields.get(View.AUTOFILL_HINT_POSTAL_ADDRESS);
        ViewNode focusedNode = mockFocusedNode("shop.example", nameId);
        Map<AutofillId, ViewNode> nodesMap = new HashMap<>();
        nodesMap.put(nameId, focusedNode);
        nodesMap.put(streetId, mockViewNode("shop.example"));

        Map<String, AutofillId> restricted =
                BraveAutofillServiceImpl.restrictToFocusedOrigin(
                        fields, nodesMap, focusedNode, "com.android.chrome", setOf());

        assertEquals(2, restricted.size());
    }

    @Test
    public void restrictToFocusedOrigin_crossOrigin_dropsOtherOriginFields() {
        // The scenario from the cross-origin iframe report: a legitimate field plus a
        // different-origin iframe's field in the same structure.
        Map<String, AutofillId> fields =
                mapWithKeys(View.AUTOFILL_HINT_NAME, View.AUTOFILL_HINT_PHONE);
        AutofillId nameId = fields.get(View.AUTOFILL_HINT_NAME);
        AutofillId phoneId = fields.get(View.AUTOFILL_HINT_PHONE);
        ViewNode focusedNode = mockFocusedNode("shop.example", nameId);
        Map<AutofillId, ViewNode> nodesMap = new HashMap<>();
        nodesMap.put(nameId, focusedNode);
        nodesMap.put(phoneId, mockViewNode("ad.example"));

        Map<String, AutofillId> restricted =
                BraveAutofillServiceImpl.restrictToFocusedOrigin(
                        fields, nodesMap, focusedNode, "com.android.chrome", setOf());

        assertEquals(1, restricted.size());
        assertSame(nameId, restricted.get(View.AUTOFILL_HINT_NAME));
    }

    @Test
    public void restrictToFocusedOrigin_noOriginSignal_unknownPackage_returnsFieldsUnchanged() {
        // No per-field origin info (e.g. compat mode) but the package isn't one we recognize as
        // compat-mode risk (e.g. a plain native app) — must not restrict a legitimate app.
        Map<String, AutofillId> fields =
                mapWithKeys(View.AUTOFILL_HINT_NAME, View.AUTOFILL_HINT_EMAIL_ADDRESS);
        AutofillId nameId = fields.get(View.AUTOFILL_HINT_NAME);
        AutofillId emailId = fields.get(View.AUTOFILL_HINT_EMAIL_ADDRESS);
        ViewNode focusedNode = mockFocusedNode(null, nameId);
        Map<AutofillId, ViewNode> nodesMap = new HashMap<>();
        nodesMap.put(nameId, focusedNode);
        nodesMap.put(emailId, mockViewNode(null));

        Map<String, AutofillId> restricted =
                BraveAutofillServiceImpl.restrictToFocusedOrigin(
                        fields,
                        nodesMap,
                        focusedNode,
                        "com.example.nativeapp",
                        setOf("com.opera.browser"));

        assertSame(fields, restricted);
    }

    @Test
    public void restrictToFocusedOrigin_noOriginSignal_riskyPackage_keepsOnlyFocusedField() {
        // The compat-mode fallback: no per-field origin info at all, and the package is one that
        // could be running in Android's accessibility-based compat mode.
        Map<String, AutofillId> fields =
                mapWithKeys(View.AUTOFILL_HINT_NAME, View.AUTOFILL_HINT_PHONE);
        AutofillId nameId = fields.get(View.AUTOFILL_HINT_NAME);
        AutofillId phoneId = fields.get(View.AUTOFILL_HINT_PHONE);
        ViewNode focusedNode = mockFocusedNode(null, nameId);
        Map<AutofillId, ViewNode> nodesMap = new HashMap<>();
        nodesMap.put(nameId, focusedNode);
        nodesMap.put(phoneId, mockViewNode(null));

        Map<String, AutofillId> restricted =
                BraveAutofillServiceImpl.restrictToFocusedOrigin(
                        fields,
                        nodesMap,
                        focusedNode,
                        "com.opera.browser",
                        setOf("com.opera.browser"));

        assertEquals(1, restricted.size());
        assertSame(nameId, restricted.get(View.AUTOFILL_HINT_NAME));
    }

    @Test
    public void restrictToFocusedOrigin_sameOrigin_orphanFieldNotInNodesMap_isDropped() {
        // A field whose AutofillId has no corresponding ViewNode can't be verified same-origin,
        // so it must be excluded rather than assumed safe.
        Map<String, AutofillId> fields =
                mapWithKeys(View.AUTOFILL_HINT_NAME, View.AUTOFILL_HINT_POSTAL_ADDRESS);
        AutofillId nameId = fields.get(View.AUTOFILL_HINT_NAME);
        ViewNode focusedNode = mockFocusedNode("shop.example", nameId);
        Map<AutofillId, ViewNode> nodesMap = Collections.singletonMap(nameId, focusedNode);

        Map<String, AutofillId> restricted =
                BraveAutofillServiceImpl.restrictToFocusedOrigin(
                        fields, nodesMap, focusedNode, "com.android.chrome", setOf());

        assertEquals(1, restricted.size());
        assertTrue(restricted.containsKey(View.AUTOFILL_HINT_NAME));
    }

    @Test
    public void restrictToFocusedOrigin_focusedFieldNotClassified_riskyPackage_failsClosed() {
        // Regression test: a cross-origin field can share a hint with an earlier-seen node and
        // lose classification entirely (see BraveAutofillViewStructureParser#getHint), so the
        // node findFocusedNode() actually finds may never appear in fields/nodesMap. Must fail
        // closed (empty), not fall back to the unrestricted fields — that would defeat the whole
        // point of the compat-mode fallback for exactly the case it exists to catch.
        Map<String, AutofillId> fields =
                mapWithKeys(View.AUTOFILL_HINT_NAME, View.AUTOFILL_HINT_PHONE);
        AutofillId nameId = fields.get(View.AUTOFILL_HINT_NAME);
        AutofillId phoneId = fields.get(View.AUTOFILL_HINT_PHONE);
        Map<AutofillId, ViewNode> nodesMap = new HashMap<>();
        nodesMap.put(nameId, mockViewNode(null));
        nodesMap.put(phoneId, mockViewNode(null));
        // The real focused node has its own AutofillId, distinct from anything in `fields`.
        ViewNode focusedNode = mockFocusedNode(null, new AutofillId(999));

        Map<String, AutofillId> restricted =
                BraveAutofillServiceImpl.restrictToFocusedOrigin(
                        fields,
                        nodesMap,
                        focusedNode,
                        "com.opera.browser",
                        setOf("com.opera.browser"));

        assertTrue(restricted.isEmpty());
    }

    @Test
    public void restrictToFocusedOrigin_focusedNodeHasNoAutofillId_riskyPackage_failsClosed() {
        Map<String, AutofillId> fields = mapWithKeys(View.AUTOFILL_HINT_NAME);
        AutofillId nameId = fields.get(View.AUTOFILL_HINT_NAME);
        Map<AutofillId, ViewNode> nodesMap = Collections.singletonMap(nameId, mockViewNode(null));
        ViewNode focusedNode = mockFocusedNode(null, null);

        Map<String, AutofillId> restricted =
                BraveAutofillServiceImpl.restrictToFocusedOrigin(
                        fields,
                        nodesMap,
                        focusedNode,
                        "com.opera.browser",
                        setOf("com.opera.browser"));

        assertTrue(restricted.isEmpty());
    }
}
