/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.language.settings;

import static org.junit.Assert.assertTrue;

import androidx.preference.PreferenceCategory;
import androidx.test.filters.SmallTest;

import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.test.BaseJUnit4ClassRunner;
import org.chromium.chrome.browser.settings.SettingsActivityTestRule;

@RunWith(BaseJUnit4ClassRunner.class)
public class BraveLanguageSettingsTest {
    static final String TRANSLATION_SETTINGS_SECTION = "translation_settings_section";

    @Rule
    public final SettingsActivityTestRule<LanguageSettings> mSettingsActivityTestRule =
            new SettingsActivityTestRule<>(LanguageSettings.class);
    private LanguageSettings mLanguageSettings;

    @Before
    public void setUp() throws Exception {
        mSettingsActivityTestRule.startSettingsActivity();
        mLanguageSettings = mSettingsActivityTestRule.getFragment();
    }

    @Test
    @SmallTest
    public void testRemovedSettings() {
        PreferenceCategory pref = mLanguageSettings.findPreference(TRANSLATION_SETTINGS_SECTION);
        assertTrue(pref != null);
        assertTrue(!pref.isEnabled());
    }
}
