/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.autofill;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertTrue;

import android.view.View;
import android.view.autofill.AutofillId;
import android.view.inputmethod.EditorInfo;

import androidx.test.filters.SmallTest;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.annotation.Config;

import org.chromium.base.test.BaseRobolectricTestRunner;

import java.util.HashMap;
import java.util.Map;

/** Unit tests for pure methods in {@link BraveAutofillViewStructureParser}. */
@RunWith(BaseRobolectricTestRunner.class)
@Config(manifest = Config.NONE)
public class BraveAutofillViewStructureParserUnitTest {

    private static Map<String, AutofillId> mapWithKeys(String... keys) {
        Map<String, AutofillId> map = new HashMap<>();
        for (int i = 0; i < keys.length; i++) {
            map.put(keys[i], new AutofillId(i + 1));
        }
        return map;
    }

    // --- hasAddressFields ---

    @Test
    @SmallTest
    public void hasAddressFields_withPostalAddress_returnsTrue() {
        assertTrue(
                BraveAutofillViewStructureParser.hasAddressFields(
                        mapWithKeys(View.AUTOFILL_HINT_POSTAL_ADDRESS)));
    }

    @Test
    @SmallTest
    public void hasAddressFields_withName_returnsTrue() {
        assertTrue(
                BraveAutofillViewStructureParser.hasAddressFields(
                        mapWithKeys(View.AUTOFILL_HINT_NAME)));
    }

    @Test
    @SmallTest
    public void hasAddressFields_withFirstName_returnsTrue() {
        assertTrue(
                BraveAutofillViewStructureParser.hasAddressFields(
                        mapWithKeys(BraveAutofillViewStructureParser.HINT_FIRST_NAME)));
    }

    @Test
    @SmallTest
    public void hasAddressFields_withEmail_returnsTrue() {
        assertTrue(
                BraveAutofillViewStructureParser.hasAddressFields(
                        mapWithKeys(View.AUTOFILL_HINT_EMAIL_ADDRESS)));
    }

    @Test
    @SmallTest
    public void hasAddressFields_withPhone_returnsTrue() {
        assertTrue(
                BraveAutofillViewStructureParser.hasAddressFields(
                        mapWithKeys(View.AUTOFILL_HINT_PHONE)));
    }

    @Test
    @SmallTest
    public void hasAddressFields_withPostalCode_returnsTrue() {
        assertTrue(
                BraveAutofillViewStructureParser.hasAddressFields(
                        mapWithKeys(View.AUTOFILL_HINT_POSTAL_CODE)));
    }

    @Test
    @SmallTest
    public void hasAddressFields_withCity_returnsTrue() {
        assertTrue(
                BraveAutofillViewStructureParser.hasAddressFields(
                        mapWithKeys(BraveAutofillViewStructureParser.HINT_CITY)));
    }

    @Test
    @SmallTest
    public void hasAddressFields_withState_returnsTrue() {
        assertTrue(
                BraveAutofillViewStructureParser.hasAddressFields(
                        mapWithKeys(BraveAutofillViewStructureParser.HINT_STATE)));
    }

    @Test
    @SmallTest
    public void hasAddressFields_withCountry_returnsTrue() {
        assertTrue(
                BraveAutofillViewStructureParser.hasAddressFields(
                        mapWithKeys(BraveAutofillViewStructureParser.HINT_COUNTRY)));
    }

    @Test
    @SmallTest
    public void hasAddressFields_withCompany_returnsTrue() {
        assertTrue(
                BraveAutofillViewStructureParser.hasAddressFields(
                        mapWithKeys(BraveAutofillViewStructureParser.HINT_COMPANY)));
    }

    @Test
    @SmallTest
    public void hasAddressFields_emptyMap_returnsFalse() {
        assertFalse(BraveAutofillViewStructureParser.hasAddressFields(new HashMap<>()));
    }

    @Test
    @SmallTest
    public void hasAddressFields_withPasswordOnly_returnsFalse() {
        assertFalse(
                BraveAutofillViewStructureParser.hasAddressFields(
                        mapWithKeys(View.AUTOFILL_HINT_PASSWORD)));
    }

    // --- mapAutofillHint: Android standard hints ---

    @Test
    @SmallTest
    public void mapAutofillHint_androidNameHint_passesThrough() {
        assertEquals(
                View.AUTOFILL_HINT_NAME,
                BraveAutofillViewStructureParser.mapAutofillHint(View.AUTOFILL_HINT_NAME));
    }

    @Test
    @SmallTest
    public void mapAutofillHint_androidEmailHint_passesThrough() {
        assertEquals(
                View.AUTOFILL_HINT_EMAIL_ADDRESS,
                BraveAutofillViewStructureParser.mapAutofillHint(View.AUTOFILL_HINT_EMAIL_ADDRESS));
    }

    @Test
    @SmallTest
    public void mapAutofillHint_androidPhoneHint_passesThrough() {
        assertEquals(
                View.AUTOFILL_HINT_PHONE,
                BraveAutofillViewStructureParser.mapAutofillHint(View.AUTOFILL_HINT_PHONE));
    }

    @Test
    @SmallTest
    public void mapAutofillHint_androidPasswordHint_passesThrough() {
        assertEquals(
                View.AUTOFILL_HINT_PASSWORD,
                BraveAutofillViewStructureParser.mapAutofillHint(View.AUTOFILL_HINT_PASSWORD));
    }

    // --- mapAutofillHint: W3C tokens ---

    @Test
    @SmallTest
    public void mapAutofillHint_w3cGivenName_returnsFirstName() {
        assertEquals(
                BraveAutofillViewStructureParser.HINT_FIRST_NAME,
                BraveAutofillViewStructureParser.mapAutofillHint(
                        BraveAutofillViewStructureParser.W3C_GIVEN_NAME));
    }

    @Test
    @SmallTest
    public void mapAutofillHint_w3cFamilyName_returnsLastName() {
        assertEquals(
                BraveAutofillViewStructureParser.HINT_LAST_NAME,
                BraveAutofillViewStructureParser.mapAutofillHint(
                        BraveAutofillViewStructureParser.W3C_FAMILY_NAME));
    }

    @Test
    @SmallTest
    public void mapAutofillHint_w3cAdditionalName_returnsMiddleName() {
        assertEquals(
                BraveAutofillViewStructureParser.HINT_MIDDLE_NAME,
                BraveAutofillViewStructureParser.mapAutofillHint(
                        BraveAutofillViewStructureParser.W3C_ADDITIONAL_NAME));
    }

    @Test
    @SmallTest
    public void mapAutofillHint_w3cStreetAddress_returnsPostalAddress() {
        assertEquals(
                View.AUTOFILL_HINT_POSTAL_ADDRESS,
                BraveAutofillViewStructureParser.mapAutofillHint(
                        BraveAutofillViewStructureParser.W3C_STREET_ADDRESS));
    }

    @Test
    @SmallTest
    public void mapAutofillHint_w3cAddressLine1_returnsPostalAddress() {
        assertEquals(
                View.AUTOFILL_HINT_POSTAL_ADDRESS,
                BraveAutofillViewStructureParser.mapAutofillHint(
                        BraveAutofillViewStructureParser.W3C_ADDRESS_LINE1));
    }

    @Test
    @SmallTest
    public void mapAutofillHint_w3cAddressLine2_returnsAddressLine2() {
        assertEquals(
                BraveAutofillViewStructureParser.HINT_ADDRESS_LINE2,
                BraveAutofillViewStructureParser.mapAutofillHint(
                        BraveAutofillViewStructureParser.W3C_ADDRESS_LINE2));
    }

    @Test
    @SmallTest
    public void mapAutofillHint_w3cPostalCode_returnsPostalCode() {
        assertEquals(
                View.AUTOFILL_HINT_POSTAL_CODE,
                BraveAutofillViewStructureParser.mapAutofillHint(
                        BraveAutofillViewStructureParser.W3C_POSTAL_CODE));
    }

    @Test
    @SmallTest
    public void mapAutofillHint_w3cOrganization_returnsCompany() {
        assertEquals(
                BraveAutofillViewStructureParser.HINT_COMPANY,
                BraveAutofillViewStructureParser.mapAutofillHint(
                        BraveAutofillViewStructureParser.W3C_ORGANIZATION));
    }

    @Test
    @SmallTest
    public void mapAutofillHint_w3cTel_returnsPhone() {
        assertEquals(
                View.AUTOFILL_HINT_PHONE,
                BraveAutofillViewStructureParser.mapAutofillHint(
                        BraveAutofillViewStructureParser.W3C_TEL));
    }

    @Test
    @SmallTest
    public void mapAutofillHint_w3cTelNational_returnsPhone() {
        assertEquals(
                View.AUTOFILL_HINT_PHONE,
                BraveAutofillViewStructureParser.mapAutofillHint(
                        BraveAutofillViewStructureParser.W3C_TEL_NATIONAL));
    }

    @Test
    @SmallTest
    public void mapAutofillHint_w3cAddressLevel1_returnsState() {
        assertEquals(
                BraveAutofillViewStructureParser.HINT_STATE,
                BraveAutofillViewStructureParser.mapAutofillHint(
                        BraveAutofillViewStructureParser.W3C_ADDRESS_LEVEL1));
    }

    @Test
    @SmallTest
    public void mapAutofillHint_w3cAddressLevel2_returnsCity() {
        assertEquals(
                BraveAutofillViewStructureParser.HINT_CITY,
                BraveAutofillViewStructureParser.mapAutofillHint(
                        BraveAutofillViewStructureParser.W3C_ADDRESS_LEVEL2));
    }

    @Test
    @SmallTest
    public void mapAutofillHint_w3cCountry_returnsCountry() {
        assertEquals(
                BraveAutofillViewStructureParser.HINT_COUNTRY,
                BraveAutofillViewStructureParser.mapAutofillHint(
                        BraveAutofillViewStructureParser.W3C_COUNTRY));
    }

    @Test
    @SmallTest
    public void mapAutofillHint_w3cCountryName_returnsCountry() {
        assertEquals(
                BraveAutofillViewStructureParser.HINT_COUNTRY,
                BraveAutofillViewStructureParser.mapAutofillHint(
                        BraveAutofillViewStructureParser.W3C_COUNTRY_NAME));
    }

    @Test
    @SmallTest
    public void mapAutofillHint_unknownHint_returnsNull() {
        assertNull(BraveAutofillViewStructureParser.mapAutofillHint("some-unknown-hint"));
    }

    // --- mapComputedAutofillHint ---

    @Test
    @SmallTest
    public void mapComputedAutofillHint_nameFull_returnsName() {
        assertEquals(
                View.AUTOFILL_HINT_NAME,
                BraveAutofillViewStructureParser.mapComputedAutofillHint(
                        BraveAutofillViewStructureParser.COMPUTED_NAME_FULL));
    }

    @Test
    @SmallTest
    public void mapComputedAutofillHint_nameFirst_returnsFirstName() {
        assertEquals(
                BraveAutofillViewStructureParser.HINT_FIRST_NAME,
                BraveAutofillViewStructureParser.mapComputedAutofillHint(
                        BraveAutofillViewStructureParser.COMPUTED_NAME_FIRST));
    }

    @Test
    @SmallTest
    public void mapComputedAutofillHint_nameMiddle_returnsMiddleName() {
        assertEquals(
                BraveAutofillViewStructureParser.HINT_MIDDLE_NAME,
                BraveAutofillViewStructureParser.mapComputedAutofillHint(
                        BraveAutofillViewStructureParser.COMPUTED_NAME_MIDDLE));
    }

    @Test
    @SmallTest
    public void mapComputedAutofillHint_nameMiddleInitial_returnsMiddleName() {
        assertEquals(
                BraveAutofillViewStructureParser.HINT_MIDDLE_NAME,
                BraveAutofillViewStructureParser.mapComputedAutofillHint(
                        BraveAutofillViewStructureParser.COMPUTED_NAME_MIDDLE_INITIAL));
    }

    @Test
    @SmallTest
    public void mapComputedAutofillHint_nameLast_returnsLastName() {
        assertEquals(
                BraveAutofillViewStructureParser.HINT_LAST_NAME,
                BraveAutofillViewStructureParser.mapComputedAutofillHint(
                        BraveAutofillViewStructureParser.COMPUTED_NAME_LAST));
    }

    @Test
    @SmallTest
    public void mapComputedAutofillHint_nameLastFirst_returnsLastName() {
        assertEquals(
                BraveAutofillViewStructureParser.HINT_LAST_NAME,
                BraveAutofillViewStructureParser.mapComputedAutofillHint(
                        BraveAutofillViewStructureParser.COMPUTED_NAME_LAST_FIRST));
    }

    @Test
    @SmallTest
    public void mapComputedAutofillHint_emailAddress_returnsEmail() {
        assertEquals(
                View.AUTOFILL_HINT_EMAIL_ADDRESS,
                BraveAutofillViewStructureParser.mapComputedAutofillHint(
                        BraveAutofillViewStructureParser.COMPUTED_EMAIL_ADDRESS));
    }

    @Test
    @SmallTest
    public void mapComputedAutofillHint_phoneHomeWholeNumber_returnsPhone() {
        assertEquals(
                View.AUTOFILL_HINT_PHONE,
                BraveAutofillViewStructureParser.mapComputedAutofillHint(
                        BraveAutofillViewStructureParser.COMPUTED_PHONE_HOME_WHOLE_NUMBER));
    }

    @Test
    @SmallTest
    public void mapComputedAutofillHint_addressHomeLine1_returnsPostalAddress() {
        assertEquals(
                View.AUTOFILL_HINT_POSTAL_ADDRESS,
                BraveAutofillViewStructureParser.mapComputedAutofillHint(
                        BraveAutofillViewStructureParser.COMPUTED_ADDRESS_HOME_LINE1));
    }

    @Test
    @SmallTest
    public void mapComputedAutofillHint_addressHomeStreetAddress_returnsPostalAddress() {
        assertEquals(
                View.AUTOFILL_HINT_POSTAL_ADDRESS,
                BraveAutofillViewStructureParser.mapComputedAutofillHint(
                        BraveAutofillViewStructureParser.COMPUTED_ADDRESS_HOME_STREET_ADDRESS));
    }

    @Test
    @SmallTest
    public void mapComputedAutofillHint_addressHomeLine2_returnsAddressLine2() {
        assertEquals(
                BraveAutofillViewStructureParser.HINT_ADDRESS_LINE2,
                BraveAutofillViewStructureParser.mapComputedAutofillHint(
                        BraveAutofillViewStructureParser.COMPUTED_ADDRESS_HOME_LINE2));
    }

    @Test
    @SmallTest
    public void mapComputedAutofillHint_addressHomeCity_returnsCity() {
        assertEquals(
                BraveAutofillViewStructureParser.HINT_CITY,
                BraveAutofillViewStructureParser.mapComputedAutofillHint(
                        BraveAutofillViewStructureParser.COMPUTED_ADDRESS_HOME_CITY));
    }

    @Test
    @SmallTest
    public void mapComputedAutofillHint_addressHomeState_returnsState() {
        assertEquals(
                BraveAutofillViewStructureParser.HINT_STATE,
                BraveAutofillViewStructureParser.mapComputedAutofillHint(
                        BraveAutofillViewStructureParser.COMPUTED_ADDRESS_HOME_STATE));
    }

    @Test
    @SmallTest
    public void mapComputedAutofillHint_addressHomeZip_returnsPostalCode() {
        assertEquals(
                View.AUTOFILL_HINT_POSTAL_CODE,
                BraveAutofillViewStructureParser.mapComputedAutofillHint(
                        BraveAutofillViewStructureParser.COMPUTED_ADDRESS_HOME_ZIP));
    }

    @Test
    @SmallTest
    public void mapComputedAutofillHint_addressHomeCountry_returnsCountry() {
        assertEquals(
                BraveAutofillViewStructureParser.HINT_COUNTRY,
                BraveAutofillViewStructureParser.mapComputedAutofillHint(
                        BraveAutofillViewStructureParser.COMPUTED_ADDRESS_HOME_COUNTRY));
    }

    @Test
    @SmallTest
    public void mapComputedAutofillHint_companyName_returnsCompany() {
        assertEquals(
                BraveAutofillViewStructureParser.HINT_COMPANY,
                BraveAutofillViewStructureParser.mapComputedAutofillHint(
                        BraveAutofillViewStructureParser.COMPUTED_COMPANY_NAME));
    }

    @Test
    @SmallTest
    public void mapComputedAutofillHint_null_returnsNull() {
        assertNull(BraveAutofillViewStructureParser.mapComputedAutofillHint(null));
    }

    @Test
    @SmallTest
    public void mapComputedAutofillHint_unknownValue_returnsNull() {
        assertNull(BraveAutofillViewStructureParser.mapComputedAutofillHint("UNKNOWN_FIELD_TYPE"));
    }

    // --- isOmnibox ---

    @Test
    @SmallTest
    public void isOmnibox_uriInputType_returnsTrue() {
        assertTrue(
                BraveAutofillViewStructureParser.isOmnibox(
                        EditorInfo.TYPE_CLASS_TEXT | EditorInfo.TYPE_TEXT_VARIATION_URI));
    }

    @Test
    @SmallTest
    public void isOmnibox_postalAddressInputType_returnsFalse() {
        // Variation values are not flags: TYPE_TEXT_VARIATION_POSTAL_ADDRESS (0x70) contains the
        // bits of TYPE_TEXT_VARIATION_URI (0x10), so they must be compared after masking.
        assertFalse(
                BraveAutofillViewStructureParser.isOmnibox(
                        EditorInfo.TYPE_CLASS_TEXT
                                | EditorInfo.TYPE_TEXT_VARIATION_POSTAL_ADDRESS));
    }

    @Test
    @SmallTest
    public void isOmnibox_webEmailInputType_returnsFalse() {
        assertFalse(
                BraveAutofillViewStructureParser.isOmnibox(
                        EditorInfo.TYPE_CLASS_TEXT
                                | EditorInfo.TYPE_TEXT_VARIATION_WEB_EMAIL_ADDRESS));
    }

    @Test
    @SmallTest
    public void isOmnibox_plainTextInputType_returnsFalse() {
        assertFalse(BraveAutofillViewStructureParser.isOmnibox(EditorInfo.TYPE_CLASS_TEXT));
    }

    // --- inferHint ---

    @Test
    @SmallTest
    public void inferHint_postalAddressInputType_returnsPostalAddress() {
        // The postal-address input type must not be mistaken for the URL bar — see isOmnibox.
        assertEquals(
                View.AUTOFILL_HINT_POSTAL_ADDRESS,
                BraveAutofillViewStructureParser.inferHint(
                        EditorInfo.TYPE_CLASS_TEXT | EditorInfo.TYPE_TEXT_VARIATION_POSTAL_ADDRESS,
                        "address_line_1"));
    }

    @Test
    @SmallTest
    public void inferHint_uriInputType_returnsNull() {
        assertNull(
                BraveAutofillViewStructureParser.inferHint(
                        EditorInfo.TYPE_CLASS_TEXT | EditorInfo.TYPE_TEXT_VARIATION_URI,
                        "address"));
    }

    @Test
    @SmallTest
    public void inferHint_unrecognizedText_returnsNull() {
        // Null lets getHint() fall through to the HTML/computed-hint layer. Any non-null value
        // here would end classification for the node before that layer runs.
        assertNull(BraveAutofillViewStructureParser.inferHint(EditorInfo.TYPE_CLASS_TEXT, "notes"));
    }

    @Test
    @SmallTest
    public void inferHint_null_returnsNull() {
        assertNull(BraveAutofillViewStructureParser.inferHint(EditorInfo.TYPE_CLASS_TEXT, null));
    }

    @Test
    @SmallTest
    public void inferHint_labelText_returnsNull() {
        assertNull(
                BraveAutofillViewStructureParser.inferHint(
                        EditorInfo.TYPE_CLASS_TEXT, "address_label"));
    }

    @Test
    @SmallTest
    public void inferHint_emailKeyword_returnsEmail() {
        assertEquals(
                View.AUTOFILL_HINT_EMAIL_ADDRESS,
                BraveAutofillViewStructureParser.inferHint(
                        EditorInfo.TYPE_CLASS_TEXT, "Enter your email"));
    }

    @Test
    @SmallTest
    public void inferHint_cityKeyword_returnsCity() {
        assertEquals(
                BraveAutofillViewStructureParser.HINT_CITY,
                BraveAutofillViewStructureParser.inferHint(EditorInfo.TYPE_CLASS_TEXT, "City"));
    }

    // --- isSaveableAddressForm ---

    @Test
    @SmallTest
    public void isSaveableAddressForm_emailAndPassword_returnsFalse() {
        assertFalse(
                BraveAutofillViewStructureParser.isSaveableAddressForm(
                        mapWithKeys(
                                View.AUTOFILL_HINT_EMAIL_ADDRESS, View.AUTOFILL_HINT_PASSWORD)));
    }

    @Test
    @SmallTest
    public void isSaveableAddressForm_nameCompanyAndPhone_returnsFalse() {
        // A contact editor: personal details, no address component anywhere.
        assertFalse(
                BraveAutofillViewStructureParser.isSaveableAddressForm(
                        mapWithKeys(
                                View.AUTOFILL_HINT_NAME,
                                BraveAutofillViewStructureParser.HINT_COMPANY,
                                View.AUTOFILL_HINT_PHONE)));
    }

    @Test
    @SmallTest
    public void isSaveableAddressForm_noFields_returnsFalse() {
        assertFalse(BraveAutofillViewStructureParser.isSaveableAddressForm(mapWithKeys()));
    }

    @Test
    @SmallTest
    public void isSaveableAddressForm_streetAndCity_returnsTrue() {
        assertTrue(
                BraveAutofillViewStructureParser.isSaveableAddressForm(
                        mapWithKeys(
                                View.AUTOFILL_HINT_POSTAL_ADDRESS,
                                BraveAutofillViewStructureParser.HINT_CITY)));
    }

    @Test
    @SmallTest
    public void isSaveableAddressForm_streetAndCityWithPassword_returnsTrue() {
        // Two components is a real address form even on a registration screen.
        assertTrue(
                BraveAutofillViewStructureParser.isSaveableAddressForm(
                        mapWithKeys(
                                View.AUTOFILL_HINT_POSTAL_ADDRESS,
                                BraveAutofillViewStructureParser.HINT_CITY,
                                View.AUTOFILL_HINT_PASSWORD)));
    }

    @Test
    @SmallTest
    public void isSaveableAddressForm_postalCodeOnly_returnsFalse() {
        assertFalse(
                BraveAutofillViewStructureParser.isSaveableAddressForm(
                        mapWithKeys(View.AUTOFILL_HINT_POSTAL_CODE)));
    }

    @Test
    @SmallTest
    public void isSaveableAddressForm_postalCodeAndName_returnsTrue() {
        assertTrue(
                BraveAutofillViewStructureParser.isSaveableAddressForm(
                        mapWithKeys(View.AUTOFILL_HINT_POSTAL_CODE, View.AUTOFILL_HINT_NAME)));
    }

    @Test
    @SmallTest
    public void isSaveableAddressForm_postalCodeAndNameWithPassword_returnsFalse() {
        // One component is not enough to override a sign-in screen.
        assertFalse(
                BraveAutofillViewStructureParser.isSaveableAddressForm(
                        mapWithKeys(
                                View.AUTOFILL_HINT_POSTAL_CODE,
                                View.AUTOFILL_HINT_NAME,
                                View.AUTOFILL_HINT_PASSWORD)));
    }
}
