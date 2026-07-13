/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.autofill.settings.options;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertThrows;
import static org.junit.Assert.assertTrue;

import android.os.Bundle;

import androidx.preference.Preference;
import androidx.preference.PreferenceScreen;
import androidx.test.filters.SmallTest;

import org.junit.After;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.ThreadUtils;
import org.chromium.base.test.util.Batch;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.preferences.BravePref;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.settings.SettingsActivityTestRule;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;
import org.chromium.components.browser_ui.settings.ChromeSwitchPreference;
import org.chromium.components.user_prefs.UserPrefs;

/** Tests for {@link BraveAutofillOptionsFragmentBase}. */
@Batch(Batch.PER_CLASS)
@RunWith(ChromeJUnit4ClassRunner.class)
public class BraveAutofillOptionsFragmentTest {
    @Rule
    public final SettingsActivityTestRule<AutofillOptionsFragment> mSettingsActivityTestRule =
            new SettingsActivityTestRule<>(AutofillOptionsFragment.class);

    private Profile mProfile;
    private boolean mOriginalAutofillPrivateWindowPref;
    private boolean mHasOriginalAutofillPrivateWindowPref;

    @After
    public void tearDown() {
        if (mHasOriginalAutofillPrivateWindowPref) {
            ThreadUtils.runOnUiThreadBlocking(
                    () ->
                            UserPrefs.get(mProfile)
                                    .setBoolean(
                                            BravePref.BRAVE_AUTOFILL_PRIVATE_WINDOWS,
                                            mOriginalAutofillPrivateWindowPref));
        }
    }

    @Test
    @SmallTest
    public void testPrivateWindowSwitchAddedForSettingsReferrer() {
        AutofillOptionsFragment fragment =
                launchFragment(AutofillOptionsFragment.AutofillOptionsReferrer.SETTINGS);

        ChromeSwitchPreference privateWindowPref = getPrivateWindowPreference(fragment);
        assertEquals(
                fragment.getString(R.string.prefs_autofill_private_window_title),
                privateWindowPref.getTitle());
        assertEquals(
                fragment.getString(R.string.prefs_autofill_private_window_summary),
                privateWindowPref.getSummary());
        assertNotNull(privateWindowPref.getIcon());
        assertEquals(getAutofillPrivateWindowPref(), privateWindowPref.isChecked());
    }

    @Test
    @SmallTest
    public void testPrivateWindowSwitchWritesProfilePref() {
        AutofillOptionsFragment fragment =
                launchFragment(AutofillOptionsFragment.AutofillOptionsReferrer.SETTINGS);
        ChromeSwitchPreference privateWindowPref = getPrivateWindowPreference(fragment);

        boolean newValue = !privateWindowPref.isChecked();

        ThreadUtils.runOnUiThreadBlocking(privateWindowPref::onClick);
        assertEquals(newValue, getAutofillPrivateWindowPref());
    }

    @Test
    @SmallTest
    public void testPrivateWindowSwitchOrder() {
        AutofillOptionsFragment fragment =
                launchFragment(AutofillOptionsFragment.AutofillOptionsReferrer.SETTINGS);

        Preference thirdPartyFilling =
                fragment.findPreference(AutofillOptionsFragment.PREF_AUTOFILL_THIRD_PARTY_FILLING);
        Preference thirdPartyToggleHint =
                fragment.findPreference(AutofillOptionsFragment.PREF_THIRD_PARTY_TOGGLE_HINT);
        Preference privateWindowPref =
                fragment.findPreference(
                        BraveAutofillOptionsFragmentBase.PREF_AUTOFILL_PRIVATE_WINDOW);
        Preference autofillAiCategory =
                fragment.findPreference(AutofillOptionsFragment.PREF_AUTOFILL_AI_CATEGORY);

        assertNotNull(thirdPartyFilling);
        assertNotNull(thirdPartyToggleHint);
        assertNotNull(privateWindowPref);
        assertTrue(thirdPartyFilling.getOrder() < thirdPartyToggleHint.getOrder());
        assertTrue(thirdPartyToggleHint.getOrder() < privateWindowPref.getOrder());
        if (autofillAiCategory != null) {
            assertTrue(privateWindowPref.getOrder() < autofillAiCategory.getOrder());
        }
    }

    @Test
    @SmallTest
    public void testPrivateWindowSwitchNotDuplicatedAfterRecreate() {
        AutofillOptionsFragment fragment =
                launchFragment(AutofillOptionsFragment.AutofillOptionsReferrer.SETTINGS);
        assertEquals(1, countTopLevelPrivateWindowPreferences(fragment));

        mSettingsActivityTestRule.recreateActivity();
        fragment = mSettingsActivityTestRule.getFragment();

        assertEquals(1, countTopLevelPrivateWindowPreferences(fragment));
    }

    @Test
    @SmallTest
    public void testPrivateWindowSwitchAddedForDeepLinkReferrer() {
        AutofillOptionsFragment fragment =
                launchFragment(
                        AutofillOptionsFragment.AutofillOptionsReferrer.DEEP_LINK_TO_SETTINGS);

        assertNotNull(getPrivateWindowPreference(fragment));
    }

    @Test
    @SmallTest
    public void testSupportedAutofillOptionsReferrersAreExplicit() {
        assertSupportedAutofillOptionsReferrer(
                AutofillOptionsFragment.AutofillOptionsReferrer.SETTINGS);
        assertSupportedAutofillOptionsReferrer(
                AutofillOptionsFragment.AutofillOptionsReferrer.PAYMENT_METHODS_FRAGMENT);
        assertSupportedAutofillOptionsReferrer(
                AutofillOptionsFragment.AutofillOptionsReferrer.AUTOFILL_PROFILES_FRAGMENT);
        assertSupportedAutofillOptionsReferrer(
                AutofillOptionsFragment.AutofillOptionsReferrer.AUTOFILL_AND_PASSWORDS_FRAGMENT);
        assertSupportedAutofillOptionsReferrer(
                AutofillOptionsFragment.AutofillOptionsReferrer.AUTOFILL_IDENTITY_DOCS_FRAGMENT);
        assertSupportedAutofillOptionsReferrer(
                AutofillOptionsFragment.AutofillOptionsReferrer.AUTOFILL_TRAVEL_FRAGMENT);
        assertSupportedAutofillOptionsReferrer(
                AutofillOptionsFragment.AutofillOptionsReferrer.AUTOFILL_SHOPPING_FRAGMENT);
        assertSupportedAutofillOptionsReferrer(
                AutofillOptionsFragment.AutofillOptionsReferrer.DEEP_LINK_TO_SETTINGS);
        assertUnsupportedAutofillOptionsReferrer(
                AutofillOptionsFragment.AutofillOptionsReferrer.COUNT);
        assertUnsupportedAutofillOptionsReferrer(
                AutofillOptionsFragment.AutofillOptionsReferrer.COUNT + 1);
    }

    @Test
    @SmallTest
    public void testMissingLaunchReferrerIsRejected() {
        assertUnsupportedAutofillOptionsReferrer(
                BraveAutofillOptionsFragmentBase.getReferrerFromInstanceStateOrLaunchBundle(
                        null, new Bundle()));
    }

    private AutofillOptionsFragment launchFragment(
            @AutofillOptionsFragment.AutofillOptionsReferrer int referrer) {
        mSettingsActivityTestRule.startSettingsActivity(
                AutofillOptionsFragment.createRequiredArgs(referrer));
        AutofillOptionsFragment fragment = mSettingsActivityTestRule.getFragment();
        cacheOriginalAutofillPrivateWindowPref(fragment.getProfile());
        return fragment;
    }

    private void cacheOriginalAutofillPrivateWindowPref(Profile profile) {
        if (mHasOriginalAutofillPrivateWindowPref) {
            return;
        }

        mProfile = profile;
        mOriginalAutofillPrivateWindowPref = getAutofillPrivateWindowPref();
        mHasOriginalAutofillPrivateWindowPref = true;
    }

    private boolean getAutofillPrivateWindowPref() {
        return ThreadUtils.runOnUiThreadBlocking(
                () -> UserPrefs.get(mProfile).getBoolean(BravePref.BRAVE_AUTOFILL_PRIVATE_WINDOWS));
    }

    private ChromeSwitchPreference getPrivateWindowPreference(AutofillOptionsFragment fragment) {
        Preference preference =
                fragment.findPreference(
                        BraveAutofillOptionsFragmentBase.PREF_AUTOFILL_PRIVATE_WINDOW);
        assertNotNull(preference);
        assertTrue(preference instanceof ChromeSwitchPreference);
        return (ChromeSwitchPreference) preference;
    }

    private int countTopLevelPrivateWindowPreferences(AutofillOptionsFragment fragment) {
        PreferenceScreen screen = fragment.getPreferenceScreen();
        int count = 0;
        for (int index = 0; index < screen.getPreferenceCount(); index++) {
            Preference preference = screen.getPreference(index);
            if (BraveAutofillOptionsFragmentBase.PREF_AUTOFILL_PRIVATE_WINDOW.equals(
                    preference.getKey())) {
                count++;
            }
        }
        return count;
    }

    @SuppressWarnings("WrongConstant")
    private void assertSupportedAutofillOptionsReferrer(int referrer) {
        BraveAutofillOptionsFragmentBase.validateAutofillOptionsReferrer(referrer);
    }

    @SuppressWarnings("WrongConstant")
    private void assertUnsupportedAutofillOptionsReferrer(int referrer) {
        assertThrows(
                IllegalArgumentException.class,
                () -> BraveAutofillOptionsFragmentBase.validateAutofillOptionsReferrer(referrer));
    }
}
