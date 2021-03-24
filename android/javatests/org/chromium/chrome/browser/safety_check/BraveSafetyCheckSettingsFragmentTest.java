/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.safety_check;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotEquals;

import android.support.test.filters.SmallTest;

import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.chrome.browser.flags.ChromeFeatureList;
import org.chromium.chrome.browser.settings.SettingsActivityTestRule;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;
import org.chromium.chrome.test.util.browser.Features;

// Currently safety check setion is not used in Brave.
// Main purpose of these tests id to detect that something new, that we may consider valuable, is
// appeared in this section .
@RunWith(ChromeJUnit4ClassRunner.class)
@Features.EnableFeatures(ChromeFeatureList.SAFETY_CHECK_ANDROID)
public class BraveSafetyCheckSettingsFragmentTest {
    private static final String PASSWORDS = "passwords";
    private static final String SAFE_BROWSING = "safe_browsing";
    private static final String UPDATES = "updates";
    // Text description plus 3 items above.
    private static final int NUMBER_OF_ITEMS = 4;

    @Rule
    public SettingsActivityTestRule<SafetyCheckSettingsFragment> mSettingsActivityTestRule =
            new SettingsActivityTestRule<>(SafetyCheckSettingsFragment.class);
    private SafetyCheckSettingsFragment mFragment;

    @Before
    public void setUp() {
        mSettingsActivityTestRule.startSettingsActivity();
        mFragment = (SafetyCheckSettingsFragment) mSettingsActivityTestRule.getFragment();
    }

    @Test
    @SmallTest
    public void testNumberOfItemsNotChanged() {
        assertEquals(NUMBER_OF_ITEMS, mFragment.getPreferenceScreen().getPreferenceCount());
    }

    @Test
    @SmallTest
    public void testExactSameItemsAreThere() {
        assertNotEquals(null, mFragment.findPreference(PASSWORDS));
        assertNotEquals(null, mFragment.findPreference(SAFE_BROWSING));
        assertNotEquals(null, mFragment.findPreference(UPDATES));
    }
}
