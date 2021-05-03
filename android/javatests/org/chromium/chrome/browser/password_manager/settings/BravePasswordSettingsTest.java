/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.password_manager.settings;

import static org.junit.Assert.assertEquals;

import android.os.Bundle;
import android.support.test.filters.SmallTest;

import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.chrome.browser.password_manager.ManagePasswordsReferrer;
import org.chromium.chrome.browser.password_manager.PasswordManagerHelper;
import org.chromium.chrome.browser.settings.SettingsActivityTestRule;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;

/**
 * Tests for the "Passwords" settings screen.
 */
@RunWith(ChromeJUnit4ClassRunner.class)
public class BravePasswordSettingsTest {
    private static final String PREF_CHECK_PASSWORDS = "check_passwords";

    @Rule
    public SettingsActivityTestRule<PasswordSettings> mSettingsActivityTestRule =
            new SettingsActivityTestRule<>(PasswordSettings.class);
    PasswordSettings mSavePasswordPreferences;

    @Before
    public void setUp() throws Exception {
        Bundle fragmentArgs = new Bundle();
        fragmentArgs.putInt(PasswordManagerHelper.MANAGE_PASSWORDS_REFERRER,
                ManagePasswordsReferrer.CHROME_SETTINGS);
        mSettingsActivityTestRule.startSettingsActivity(fragmentArgs);
        mSavePasswordPreferences = mSettingsActivityTestRule.getFragment();
    }

    @Test
    @SmallTest
    public void testCheckPasswordsIsRemoved() {
        assertEquals(null, mSavePasswordPreferences.findPreference(PREF_CHECK_PASSWORDS));
    }
}
