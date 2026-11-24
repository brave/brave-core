/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.browsing_data;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;

import androidx.preference.Preference;
import androidx.preference.PreferenceScreen;
import androidx.test.filters.SmallTest;

import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.test.util.Batch;
import org.chromium.base.test.util.Features.EnableFeatures;
import org.chromium.chrome.browser.ChromeTabbedActivity;
import org.chromium.chrome.browser.flags.ChromeFeatureList;
import org.chromium.chrome.browser.settings.SettingsActivityTestRule;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;

/** Tests for {@link BraveClearBrowsingDataFragment}. */
@Batch(Batch.PER_CLASS)
@RunWith(ChromeJUnit4ClassRunner.class)
@EnableFeatures(ChromeFeatureList.DBD_PASSWORD_REMOVAL_ON_ANDROID)
public class BraveClearBrowsingDataFragmentTest {
    @Rule
    public final SettingsActivityTestRule<BraveClearBrowsingDataFragment>
            mSettingsActivityTestRule =
                    new SettingsActivityTestRule<>(BraveClearBrowsingDataFragment.class);

    @Test
    @SmallTest
    public void testBraveRowOrder() {
        BraveClearBrowsingDataFragment fragment = launchFragment();
        PreferenceScreen screen = fragment.getPreferenceScreen();

        Preference leoAiHistory = fragment.mClearAIChatDataCheckBoxPreference;
        Preference manageOtherData =
                fragment.findPreference(
                        ClearBrowsingDataFragment.PREF_MANAGE_OTHER_GOOGLE_DATA_EXPANDABLE);
        Preference braveAdsData = findKeylessRowOtherThan(screen, leoAiHistory);

        assertNotNull(leoAiHistory);
        assertNotNull(manageOtherData);

        // The Leo AI checkbox belongs with the other checkboxes, so the sections
        // that close the screen have to follow it.
        assertTrue(leoAiHistory.getOrder() < manageOtherData.getOrder());

        Preference signOutText =
                fragment.findPreference(ClearBrowsingDataFragment.PREF_SIGN_OUT_OF_CHROME_TEXT);
        if (signOutText != null) {
            assertTrue(manageOtherData.getOrder() < signOutText.getOrder());
        }

        // The Brave Ads row is a text link, so nothing may follow it.
        for (int i = 0; i < screen.getPreferenceCount(); i++) {
            Preference preference = screen.getPreference(i);
            if (preference != braveAdsData) {
                assertTrue(preference.getOrder() < braveAdsData.getOrder());
            }
        }
    }

    private BraveClearBrowsingDataFragment launchFragment() {
        mSettingsActivityTestRule.startSettingsActivity(
                ClearBrowsingDataFragment.createFragmentArgs(ChromeTabbedActivity.class.getName()));
        BraveClearBrowsingDataFragment fragment = mSettingsActivityTestRule.getFragment();
        assertNotNull(fragment);
        return fragment;
    }

    // Brave adds its own rows in code with no key, so the Rewards/Ads one, which
    // has no title either, can only be told apart from the Leo AI checkbox.
    private static Preference findKeylessRowOtherThan(PreferenceScreen screen, Preference other) {
        Preference found = null;
        int count = 0;
        for (int i = 0; i < screen.getPreferenceCount(); i++) {
            Preference preference = screen.getPreference(i);
            if (!preference.hasKey() && preference != other) {
                found = preference;
                count++;
            }
        }
        assertEquals(1, count);
        return found;
    }
}
