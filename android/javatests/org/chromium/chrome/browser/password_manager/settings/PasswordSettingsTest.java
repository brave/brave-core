// Copyright 2014 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

package org.chromium.chrome.browser.password_manager.settings;

import static androidx.test.espresso.matcher.ViewMatchers.withText;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertTrue;

import static org.chromium.ui.test.util.ViewUtils.onViewWaiting;

import android.os.Bundle;

import androidx.test.filters.SmallTest;

import org.junit.After;
import org.junit.Assert;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.ThreadUtils;
import org.chromium.base.test.util.Batch;
import org.chromium.base.test.util.CommandLineFlags;
import org.chromium.base.test.util.Feature;
import org.chromium.chrome.browser.flags.ChromeSwitches;
import org.chromium.chrome.browser.password_manager.BravePasswordManagerHelper;
import org.chromium.chrome.browser.password_manager.ManagePasswordsReferrer;
import org.chromium.chrome.browser.preferences.Pref;
import org.chromium.chrome.browser.profiles.ProfileManager;
import org.chromium.chrome.browser.settings.SettingsActivityTestRule;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;
import org.chromium.chrome.test.OverrideContextWrapperTestRule;
import org.chromium.chrome.test.R;
import org.chromium.components.browser_ui.settings.ChromeSwitchPreference;
import org.chromium.components.prefs.PrefService;
import org.chromium.components.user_prefs.UserPrefs;

/**
 * Tests for the "Passwords" settings screen. These tests are not batchable (without significant
 * effort), so consider splitting large new suites into separate classes.
 */
@RunWith(ChromeJUnit4ClassRunner.class)
@CommandLineFlags.Add({ChromeSwitches.DISABLE_FIRST_RUN_EXPERIENCE})
@Batch(Batch.PER_CLASS)
public class PasswordSettingsTest {
    private static final String PREF_CHECK_PASSWORDS = "check_passwords";

    @Rule
    public SettingsActivityTestRule<PasswordSettings> mPasswordSettingsActivityTestRule =
            new SettingsActivityTestRule<>(PasswordSettings.class);

    @Rule
    public OverrideContextWrapperTestRule mAutomotiveContextWrapperTestRule =
            new OverrideContextWrapperTestRule();

    private final PasswordSettingsTestHelper mTestHelper = new PasswordSettingsTestHelper();

    @Before
    public void setUp() {
        // This initializes the browser, so some tests can do setup before PasswordSettings is
        // launched. ChromeTabbedActivityTestRule.startMainActivityOnBlankPage() is more commonly
        // used for this end, but using another settings activity instead makes these tests more
        // isolated, i.e. avoids exercising unnecessary logic. BlankUiTestActivityTestCase also
        // won't fit here, it doesn't initialize enough of the browser.
        Bundle fragmentArgs = new Bundle();
        fragmentArgs.putInt(
                BravePasswordManagerHelper.MANAGE_PASSWORDS_REFERRER,
                ManagePasswordsReferrer.CHROME_SETTINGS);
        mPasswordSettingsActivityTestRule.startSettingsActivity(fragmentArgs);
        mPasswordSettingsActivityTestRule.finishActivity();
    }

    @After
    public void tearDown() {
        mTestHelper.tearDown();
    }

    /** Ensure that resetting of empty passwords list works. */
    @Test
    @SmallTest
    @Feature({"Preferences"})
    public void testResetListEmpty() {
        // Load the preferences, they should show the empty list.
        mTestHelper.startPasswordSettingsFromMainSettings(mPasswordSettingsActivityTestRule);
        onViewWaiting(withText(R.string.password_manager_settings_title));

        ThreadUtils.runOnUiThreadBlocking(
                () -> {
                    PasswordSettings savePasswordPreferences =
                            mPasswordSettingsActivityTestRule.getFragment();
                    // Emulate an update from PasswordStore. This should not crash.
                    savePasswordPreferences.passwordListAvailable(0);
                });
    }

    /**
     * Ensure that the on/off switch in "Save Passwords" settings actually enables and disables
     * password saving.
     */
    @Test
    @SmallTest
    @Feature({"Preferences"})
    public void testSavePasswordsSwitch() {
        ThreadUtils.runOnUiThreadBlocking(
                () -> {
                    getPrefService().setBoolean(Pref.CREDENTIALS_ENABLE_SERVICE, true);
                });

        mTestHelper.startPasswordSettingsFromMainSettings(mPasswordSettingsActivityTestRule);
        onViewWaiting(withText(R.string.password_manager_settings_title));

        ThreadUtils.runOnUiThreadBlocking(
                () -> {
                    PasswordSettings savedPasswordPrefs =
                            mPasswordSettingsActivityTestRule.getFragment();
                    ChromeSwitchPreference onOffSwitch =
                            (ChromeSwitchPreference)
                                    savedPasswordPrefs.findPreference(
                                            PasswordSettings.PREF_SAVE_PASSWORDS_SWITCH);
                    assertTrue(onOffSwitch.isChecked());

                    onOffSwitch.performClick();
                    assertFalse(getPrefService().getBoolean(Pref.CREDENTIALS_ENABLE_SERVICE));

                    onOffSwitch.performClick();
                    assertTrue(getPrefService().getBoolean(Pref.CREDENTIALS_ENABLE_SERVICE));
                });

        mPasswordSettingsActivityTestRule.finishActivity();

        ThreadUtils.runOnUiThreadBlocking(
                () -> {
                    getPrefService().setBoolean(Pref.CREDENTIALS_ENABLE_SERVICE, false);
                });

        mTestHelper.startPasswordSettingsFromMainSettings(mPasswordSettingsActivityTestRule);
        onViewWaiting(withText(R.string.password_manager_settings_title));
        ThreadUtils.runOnUiThreadBlocking(
                () -> {
                    PasswordSettings savedPasswordPrefs =
                            mPasswordSettingsActivityTestRule.getFragment();
                    ChromeSwitchPreference onOffSwitch =
                            (ChromeSwitchPreference)
                                    savedPasswordPrefs.findPreference(
                                            PasswordSettings.PREF_SAVE_PASSWORDS_SWITCH);
                    assertFalse(onOffSwitch.isChecked());
                });
    }

    // These upstream tests were removed:
    //  - testManageAccountLinkNotSignedIn
    //  - testManageAccountLinkSignedInNotSyncing
    //  - testManageAccountLinkSyncing
    //  ^ we don't have an account to manage
    //
    //  - testDestroysPasswordCheckIfFirstInSettingsStack
    //  - testDoesNotDestroyPasswordCheckIfNotFirstInSettingsStack
    //  ^ we don't have password check feature

    /**
     * Ensure that the "Auto Sign-in" switch in "Save Passwords" settings actually enables and
     * disables auto sign-in.
     */
    @Test
    @SmallTest
    @Feature({"Preferences"})
    public void testAutoSignInCheckbox() {
        mAutomotiveContextWrapperTestRule.setIsAutomotive(false);
        ThreadUtils.runOnUiThreadBlocking(
                () -> {
                    getPrefService().setBoolean(Pref.CREDENTIALS_ENABLE_AUTOSIGNIN, true);
                });

        mTestHelper.startPasswordSettingsFromMainSettings(mPasswordSettingsActivityTestRule);

        ThreadUtils.runOnUiThreadBlocking(
                () -> {
                    PasswordSettings passwordPrefs =
                            mPasswordSettingsActivityTestRule.getFragment();
                    ChromeSwitchPreference onOffSwitch =
                            (ChromeSwitchPreference)
                                    passwordPrefs.findPreference(
                                            PasswordSettings.PREF_AUTOSIGNIN_SWITCH);
                    assertTrue(onOffSwitch.isChecked());

                    onOffSwitch.performClick();
                    assertFalse(getPrefService().getBoolean(Pref.CREDENTIALS_ENABLE_AUTOSIGNIN));

                    onOffSwitch.performClick();
                    assertTrue(getPrefService().getBoolean(Pref.CREDENTIALS_ENABLE_AUTOSIGNIN));
                });

        mPasswordSettingsActivityTestRule.finishActivity();

        ThreadUtils.runOnUiThreadBlocking(
                () -> {
                    getPrefService().setBoolean(Pref.CREDENTIALS_ENABLE_AUTOSIGNIN, false);
                });

        mTestHelper.startPasswordSettingsFromMainSettings(mPasswordSettingsActivityTestRule);
        ThreadUtils.runOnUiThreadBlocking(
                () -> {
                    PasswordSettings passwordPrefs =
                            mPasswordSettingsActivityTestRule.getFragment();
                    ChromeSwitchPreference onOffSwitch =
                            (ChromeSwitchPreference)
                                    passwordPrefs.findPreference(
                                            PasswordSettings.PREF_AUTOSIGNIN_SWITCH);
                    assertFalse(onOffSwitch.isChecked());
                });
    }

    /**
     * Ensure that the "Auto Sign-in" switch in "Save Passwords" settings is not present on
     * automotive.
     */
    @Test
    @SmallTest
    @Feature({"Preferences"})
    public void testAutoSignInCheckboxIsNotPresentOnAutomotive() {
        mAutomotiveContextWrapperTestRule.setIsAutomotive(true);

        mTestHelper.startPasswordSettingsFromMainSettings(mPasswordSettingsActivityTestRule);

        ThreadUtils.runOnUiThreadBlocking(
                () -> {
                    PasswordSettings passwordPrefs =
                            mPasswordSettingsActivityTestRule.getFragment();
                    ChromeSwitchPreference onOffSwitch =
                            (ChromeSwitchPreference)
                                    passwordPrefs.findPreference(
                                            PasswordSettings.PREF_AUTOSIGNIN_SWITCH);
                    assertNull("There should be no autosignin switch.", onOffSwitch);
                });
    }

    /** Check that the check passwords preference is not shown. */
    @Test
    @SmallTest
    @Feature({"Preferences"})
    public void testCheckPasswordsNotEnabled() {
        mTestHelper.startPasswordSettingsFromMainSettings(mPasswordSettingsActivityTestRule);
        ThreadUtils.runOnUiThreadBlocking(
                () -> {
                    PasswordSettings passwordPrefs =
                            mPasswordSettingsActivityTestRule.getFragment();
                    Assert.assertNull(passwordPrefs.findPreference(PREF_CHECK_PASSWORDS));
                });
    }

    private static PrefService getPrefService() {
        return UserPrefs.get(ProfileManager.getLastUsedRegularProfile());
    }
}
