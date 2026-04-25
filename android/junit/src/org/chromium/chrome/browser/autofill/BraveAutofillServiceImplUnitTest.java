/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.autofill;

import static org.junit.Assert.assertArrayEquals;
import static org.junit.Assert.assertEquals;

import androidx.test.filters.SmallTest;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.annotation.Config;

import org.chromium.base.test.BaseRobolectricTestRunner;

/** Unit tests for pure utility methods in {@link BraveAutofillServiceImpl}. */
@RunWith(BaseRobolectricTestRunner.class)
@Config(manifest = Config.NONE)
public class BraveAutofillServiceImplUnitTest {

    // --- splitName ---

    @Test
    @SmallTest
    public void splitName_fullNameOnly_splitsIntoFirstAndLast() {
        assertArrayEquals(
                new String[] {"Jane", "Doe"},
                BraveAutofillServiceImpl.splitName("Jane Doe", null, null));
    }

    @Test
    @SmallTest
    public void splitName_singleWordFullName_firstNameOnly() {
        assertArrayEquals(
                new String[] {"Madonna", ""},
                BraveAutofillServiceImpl.splitName("Madonna", null, null));
    }

    @Test
    @SmallTest
    public void splitName_multiWordLastName_preservesRemainder() {
        assertArrayEquals(
                new String[] {"Anna", "Maria Van Der Berg"},
                BraveAutofillServiceImpl.splitName("Anna Maria Van Der Berg", null, null));
    }

    @Test
    @SmallTest
    public void splitName_existingPartsNotOverridden() {
        assertArrayEquals(
                new String[] {"Jonathan", "Smithson"},
                BraveAutofillServiceImpl.splitName("John Smith", "Jonathan", "Smithson"));
    }

    @Test
    @SmallTest
    public void splitName_allNull_returnsEmptyStrings() {
        assertArrayEquals(
                new String[] {"", ""}, BraveAutofillServiceImpl.splitName(null, null, null));
    }

    @Test
    @SmallTest
    public void splitName_emptyFullName_returnsEmptyStrings() {
        assertArrayEquals(
                new String[] {"", ""}, BraveAutofillServiceImpl.splitName("", null, null));
    }

    @Test
    @SmallTest
    public void splitName_fullNameWithExtraWhitespace_trims() {
        assertArrayEquals(
                new String[] {"Jane", "Doe"},
                BraveAutofillServiceImpl.splitName("  Jane   Doe  ", null, null));
    }

    @Test
    @SmallTest
    public void splitName_firstNameEmpty_lastNameProvided_derivesFirstOnly() {
        assertArrayEquals(
                new String[] {"Jane", "Smith"},
                BraveAutofillServiceImpl.splitName("Jane Doe", "", "Smith"));
    }

    // --- countryCodeToDisplayName ---

    @Test
    @SmallTest
    public void countryCodeToDisplayName_us_returnsUnitedStates() {
        assertEquals("United States", BraveAutofillServiceImpl.countryCodeToDisplayName("US"));
    }

    @Test
    @SmallTest
    public void countryCodeToDisplayName_gb_returnsUnitedKingdom() {
        assertEquals("United Kingdom", BraveAutofillServiceImpl.countryCodeToDisplayName("GB"));
    }

    @Test
    @SmallTest
    public void countryCodeToDisplayName_de_returnsGermany() {
        assertEquals("Germany", BraveAutofillServiceImpl.countryCodeToDisplayName("DE"));
    }

    @Test
    @SmallTest
    public void countryCodeToDisplayName_null_returnsEmptyString() {
        assertEquals("", BraveAutofillServiceImpl.countryCodeToDisplayName(null));
    }

    @Test
    @SmallTest
    public void countryCodeToDisplayName_emptyString_returnsEmpty() {
        assertEquals("", BraveAutofillServiceImpl.countryCodeToDisplayName(""));
    }

    @Test
    @SmallTest
    public void countryCodeToDisplayName_threeLetterCode_returnsUnchanged() {
        assertEquals("USA", BraveAutofillServiceImpl.countryCodeToDisplayName("USA"));
    }

    @Test
    @SmallTest
    public void countryCodeToDisplayName_lowercaseCode_returnsDisplayName() {
        // java.util.Locale handles lowercase country codes.
        assertEquals("United States", BraveAutofillServiceImpl.countryCodeToDisplayName("us"));
    }

    // --- matchListOption ---

    @Test
    @SmallTest
    public void matchListOption_exactMatch_returnsIndex() {
        CharSequence[] options = {"Alabama", "California", "Texas"};
        assertEquals(1, BraveAutofillServiceImpl.matchListOption(options, "California"));
    }

    @Test
    @SmallTest
    public void matchListOption_exactMatchCaseInsensitive_returnsIndex() {
        CharSequence[] options = {"Alabama", "California", "Texas"};
        assertEquals(1, BraveAutofillServiceImpl.matchListOption(options, "california"));
    }

    @Test
    @SmallTest
    public void matchListOption_optionStartsWithValue_returnsIndex() {
        // Pass 2: value "CA" matches option "California".
        CharSequence[] options = {"Alabama", "California", "Texas"};
        assertEquals(1, BraveAutofillServiceImpl.matchListOption(options, "CA"));
    }

    @Test
    @SmallTest
    public void matchListOption_valueStartsWithOption_returnsIndex() {
        // Pass 3: value "California" matches option "CA".
        CharSequence[] options = {"AL", "CA", "TX"};
        assertEquals(1, BraveAutofillServiceImpl.matchListOption(options, "California"));
    }

    @Test
    @SmallTest
    public void matchListOption_noMatch_returnsNegativeOne() {
        CharSequence[] options = {"Alabama", "California", "Texas"};
        assertEquals(-1, BraveAutofillServiceImpl.matchListOption(options, "New York"));
    }

    @Test
    @SmallTest
    public void matchListOption_nullOptions_returnsNegativeOne() {
        assertEquals(-1, BraveAutofillServiceImpl.matchListOption(null, "California"));
    }

    @Test
    @SmallTest
    public void matchListOption_emptyOptions_returnsNegativeOne() {
        assertEquals(
                -1, BraveAutofillServiceImpl.matchListOption(new CharSequence[0], "California"));
    }

    @Test
    @SmallTest
    public void matchListOption_singleCharValue_skipsPass2() {
        // Single character value should not match via prefix (pass 2 requires length >= 2).
        CharSequence[] options = {"Cat", "Car", "Cap"};
        assertEquals(-1, BraveAutofillServiceImpl.matchListOption(options, "C"));
    }

    @Test
    @SmallTest
    public void matchListOption_nullOptionElement_skipped() {
        // Null elements in the options array should be safely skipped without NPE.
        CharSequence[] options = {null, "California", "Texas"};
        assertEquals(1, BraveAutofillServiceImpl.matchListOption(options, "California"));
    }

    @Test
    @SmallTest
    public void matchListOption_exactMatchPreferredOverPrefix() {
        // "CA" should match index 0 (exact) not index 1 (prefix).
        CharSequence[] options = {"CA", "California"};
        assertEquals(0, BraveAutofillServiceImpl.matchListOption(options, "CA"));
    }

    @Test
    @SmallTest
    public void matchListOption_singleCharOption_skipsPass3() {
        // Pass 3 requires option length >= 2, so "C" is skipped.
        CharSequence[] options = {"C", "CALIF"};
        // "California" does not exact-match either, does not prefix-match either,
        // but starts with "CALIF" (pass 3).
        assertEquals(1, BraveAutofillServiceImpl.matchListOption(options, "California"));
    }
}
