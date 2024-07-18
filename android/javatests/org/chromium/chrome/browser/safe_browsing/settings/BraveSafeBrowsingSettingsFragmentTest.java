/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.safe_browsing.settings;

import android.view.View;

import androidx.test.filters.SmallTest;

import org.junit.Assert;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.ThreadUtils;
import org.chromium.base.test.BaseJUnit4ClassRunner;
import org.chromium.chrome.browser.settings.SettingsActivityTestRule;

@RunWith(BaseJUnit4ClassRunner.class)
public class BraveSafeBrowsingSettingsFragmentTest {
    @Rule
    public SettingsActivityTestRule<SafeBrowsingSettingsFragment> mTestRule =
            new SettingsActivityTestRule<>(SafeBrowsingSettingsFragment.class);

    private SafeBrowsingSettingsFragment mSafeBrowsingSettingsFragment;
    private RadioButtonGroupSafeBrowsingPreference mSafeBrowsingPreference;

    @Before
    public void setUp() {
        mTestRule.startSettingsActivity();
        mSafeBrowsingSettingsFragment = mTestRule.getFragment();
        mSafeBrowsingPreference = mSafeBrowsingSettingsFragment.findPreference(
                SafeBrowsingSettingsFragment.PREF_SAFE_BROWSING);
    }

    @Test
    @SmallTest
    public void testEnhancedProtectionNotVisible() {
        ThreadUtils.runOnUiThreadBlocking(
                () -> {
                    Assert.assertEquals(
                            mSafeBrowsingPreference
                                    .getEnhancedProtectionButtonForTesting()
                                    .getVisibility(),
                            View.GONE);
                });
    }
}
