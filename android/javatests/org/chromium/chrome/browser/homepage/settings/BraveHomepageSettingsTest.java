/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.homepage.settings;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotEquals;
import static org.junit.Assert.assertTrue;

import androidx.preference.Preference;
import androidx.test.filters.SmallTest;

import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.ThreadUtils;
import org.chromium.base.test.util.Batch;
import org.chromium.chrome.browser.homepage.settings.RadioButtonGroupHomepagePreference.HomepageOption;
import org.chromium.chrome.browser.settings.SettingsActivityTestRule;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;

/** Test for {@link BraveHomepageSettings} to check Brave related UI changes. */
@Batch(Batch.PER_CLASS)
@RunWith(ChromeJUnit4ClassRunner.class)
public class BraveHomepageSettingsTest {
    private static final String PREF_HOMEPAGE_RADIO_GROUP = "homepage_radio_group";

    @Rule
    public SettingsActivityTestRule<BraveHomepageSettings> mSettingsActivityTestRule =
            new SettingsActivityTestRule<>(BraveHomepageSettings.class);

    private BraveHomepageSettings mFragment;

    @Before
    public void setUp() {
        mSettingsActivityTestRule.startSettingsActivity();
        mFragment = (BraveHomepageSettings) mSettingsActivityTestRule.getFragment();
    }

    @Test
    @SmallTest
    public void testHomepageRadioGroupType() {
        Preference homepageRadioGroup = mFragment.findPreference(PREF_HOMEPAGE_RADIO_GROUP);
        assertNotEquals(null, homepageRadioGroup);
        assertTrue(homepageRadioGroup instanceof BraveRadioButtonGroupHomepagePreference);
    }

    @Test
    @SmallTest
    public void testMobileBookmarksOption() {
        ThreadUtils.runOnUiThreadBlocking(
                () -> {
                    BraveRadioButtonGroupHomepagePreference homepageRadioGroup =
                            mFragment.findPreference(PREF_HOMEPAGE_RADIO_GROUP);
                    // Check initial state.
                    assertTrue(homepageRadioGroup.getChromeNtpRadioButton().isChecked());
                    assertFalse(homepageRadioGroup.getMobileBookmarksRadioButton().isChecked());
                    assertFalse(homepageRadioGroup.getCustomUriRadioButton().isChecked());

                    // Check the click action on the mobile bookmarks radio button.
                    homepageRadioGroup.getMobileBookmarksRadioButton().performClick();
                    assertFalse(homepageRadioGroup.getChromeNtpRadioButton().isChecked());
                    assertTrue(homepageRadioGroup.getMobileBookmarksRadioButton().isChecked());
                    assertFalse(homepageRadioGroup.getCustomUriRadioButton().isChecked());
                    assertTrue(
                            homepageRadioGroup.getPreferenceValue().getCheckedOption()
                                    == RadioButtonGroupHomepagePreference.HomepageOption
                                            .ENTRY_CUSTOM_URI);
                    assertTrue(
                            homepageRadioGroup
                                    .getPreferenceValue()
                                    .getCustomURI()
                                    .equals(
                                            BraveRadioButtonGroupHomepagePreference
                                                    .MOBILE_BOOKMARKS_PATH));
                });
    }

    @Test
    @SmallTest
    public void testAnyBookmarksCustomUrlIsMobileBookamrks() {
        ThreadUtils.runOnUiThreadBlocking(
                () -> {
                    BraveRadioButtonGroupHomepagePreference homepageRadioGroup =
                            mFragment.findPreference(PREF_HOMEPAGE_RADIO_GROUP);

                    homepageRadioGroup.setupPreferenceValues(
                            new RadioButtonGroupHomepagePreference.PreferenceValues(
                                    /* checkedOption= */ HomepageOption.ENTRY_CUSTOM_URI,
                                    /* customizedText= */ "chrome-native://bookmarks/folder/42",
                                    /* isEnabled= */ true,
                                    /* isNtpButtonVisible= */ true,
                                    /* isCustomizedOptionVisible= */ true));

                    assertTrue(homepageRadioGroup.getMobileBookmarksRadioButton().isChecked());
                });
    }
}
