/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.autofill.settings;

import static org.junit.Assert.assertNull;

import androidx.test.filters.SmallTest;

import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.test.util.Batch;
import org.chromium.chrome.browser.settings.SettingsActivityTestRule;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;

/** Tests for {@link BraveAutofillPaymentMethodsFragmentBase}. */
@Batch(Batch.PER_CLASS)
@RunWith(ChromeJUnit4ClassRunner.class)
public class BraveAutofillPaymentMethodsFragmentTest {
    @Rule
    public final SettingsActivityTestRule<AutofillPaymentMethodsFragment>
            mSettingsActivityTestRule =
                    new SettingsActivityTestRule<>(AutofillPaymentMethodsFragment.class);

    private AutofillPaymentMethodsFragment mFragment;

    @Before
    public void setUp() throws Exception {
        mSettingsActivityTestRule.startSettingsActivity();
        mFragment = mSettingsActivityTestRule.getFragment();
    }

    @Test
    @SmallTest
    public void testLoyaltyCardsPrefRemoved() {
        assertNull(
                "loyalty_cards pref should be removed by BraveAutofillPaymentMethodsFragmentBase",
                mFragment.findPreference(AutofillPaymentMethodsFragment.PREF_LOYALTY_CARDS));
    }
}
