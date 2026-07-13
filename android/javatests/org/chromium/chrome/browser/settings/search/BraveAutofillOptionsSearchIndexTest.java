/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.settings.search;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertTrue;

import android.content.Context;

import androidx.test.filters.SmallTest;

import org.junit.After;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.ThreadUtils;
import org.chromium.base.test.util.Batch;
import org.chromium.base.test.util.CommandLineFlags;
import org.chromium.base.test.util.DisableLeakChecks;
import org.chromium.base.test.util.Features.DisableFeatures;
import org.chromium.base.test.util.Features.EnableFeatures;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.autofill.settings.HomeOfTransactionsFragment;
import org.chromium.chrome.browser.autofill.settings.options.AutofillOptionsFragment;
import org.chromium.chrome.browser.autofill.settings.options.AutofillOptionsFragment.AutofillOptionsReferrer;
import org.chromium.chrome.browser.autofill.settings.options.BraveAutofillOptionsFragmentBase;
import org.chromium.chrome.browser.flags.ChromeFeatureList;
import org.chromium.chrome.browser.flags.ChromeSwitches;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.settings.MainSettings;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;
import org.chromium.chrome.test.ChromeTabbedActivityTestRule;
import org.chromium.components.browser_ui.settings.search.PreferenceParser;
import org.chromium.components.browser_ui.settings.search.SettingsIndexData;

import java.util.List;

/** Tests Brave additions to the Autofill options search index. */
@RunWith(ChromeJUnit4ClassRunner.class)
@Batch(Batch.PER_CLASS)
@CommandLineFlags.Add({ChromeSwitches.DISABLE_FIRST_RUN_EXPERIENCE})
@EnableFeatures({
    ChromeFeatureList.AUTOFILL_AI_WITH_DATA_SCHEMA,
    ChromeFeatureList.AUTOFILL_ENABLE_NEW_CARD_BENEFITS_TOGGLE_TEXT,
    ChromeFeatureList.AUTOFILL_ENABLE_SEPARATE_PIX_PREFERENCE_ITEM,
    ChromeFeatureList.FACILITATED_PAYMENTS_ENABLE_A2A_PAYMENT,
    ChromeFeatureList.AUTOFILL_SYNC_EWALLET_ACCOUNTS,
    ChromeFeatureList.DETAILED_LANGUAGE_SETTINGS,
    ChromeFeatureList.PLUS_ADDRESSES_ENABLED
})
@DisableLeakChecks("crbug.com/512492984 (SearchIndexProviderRegistryTest)")
public class BraveAutofillOptionsSearchIndexTest {
    @Rule
    public ChromeTabbedActivityTestRule mActivityTestRule = new ChromeTabbedActivityTestRule();

    private Context mContext;
    private Profile mProfile;
    private SettingsIndexData mIndexData;

    @Before
    public void setUp() {
        mActivityTestRule.startMainActivityOnBlankPage();
        mContext = mActivityTestRule.getActivity();

        ThreadUtils.runOnUiThreadBlocking(
                () -> {
                    mProfile = mActivityTestRule.getActivity().getActivityTab().getProfile();
                    assertNull(mProfile.getOtrProfileId());
                    mIndexData = SettingsIndexData.createInstance();
                });
    }

    @After
    public void tearDown() {
        ThreadUtils.runOnUiThreadBlocking(SettingsIndexData::reset);
    }

    @Test
    @SmallTest
    @DisableFeatures(ChromeFeatureList.YOUR_SAVED_INFO_SETTINGS_PAGE_ANDROID)
    public void testPrivateWindowEntrySurvivesLegacyRoute() {
        buildIndex();

        SettingsIndexData.Entry entry = getPrivateWindowEntry();
        assertSettingsReferrerExtras(entry);
        assertSearchFindsPrivateWindowEntry(entry);

        SettingsIndexData.Entry legacyAutofillOptionsEntry =
                mIndexData.getEntryForKey(
                        MainSettings.class.getName(), MainSettings.PREF_AUTOFILL_OPTIONS);
        assertNotNull(legacyAutofillOptionsEntry);
        assertEquals(AutofillOptionsFragment.class.getName(), legacyAutofillOptionsEntry.fragment);
        assertSettingsReferrerExtras(legacyAutofillOptionsEntry);
    }

    @Test
    @SmallTest
    @EnableFeatures(ChromeFeatureList.YOUR_SAVED_INFO_SETTINGS_PAGE_ANDROID)
    public void testPrivateWindowEntrySurvivesHomeOfTransactionsRoute() {
        buildIndex();

        SettingsIndexData.Entry entry = getPrivateWindowEntry();
        assertSettingsReferrerExtras(entry);
        assertSearchFindsPrivateWindowEntry(entry);

        assertNull(
                mIndexData.getEntryForKey(
                        MainSettings.class.getName(), MainSettings.PREF_AUTOFILL_OPTIONS));

        SettingsIndexData.Entry mainAutofillAndPasswordsEntry =
                mIndexData.getEntryForKey(
                        MainSettings.class.getName(), MainSettings.PREF_AUTOFILL_AND_PASSWORDS);
        assertNotNull(mainAutofillAndPasswordsEntry);
        assertEquals(
                HomeOfTransactionsFragment.class.getName(), mainAutofillAndPasswordsEntry.fragment);
        assertHomeOfTransactionsReferrerExtras(mainAutofillAndPasswordsEntry);

        SettingsIndexData.Entry homeAutofillOptionsEntry =
                mIndexData.getEntryForKey(
                        HomeOfTransactionsFragment.class.getName(),
                        HomeOfTransactionsFragment.PREF_AUTOFILL_SETTINGS);
        assertNotNull(homeAutofillOptionsEntry);
        assertEquals(AutofillOptionsFragment.class.getName(), homeAutofillOptionsEntry.fragment);
        assertSettingsReferrerExtras(homeAutofillOptionsEntry);

        List<SettingsIndexData.Entry> breadcrumbEntries =
                mIndexData.getBreadcrumbEntries(
                        AutofillOptionsFragment.class.getName(),
                        AutofillOptionsFragment.createRequiredArgs(
                                AutofillOptionsReferrer.SETTINGS),
                        MainSettings.class.getName());
        assertNotNull(breadcrumbEntries);
        assertEquals(2, breadcrumbEntries.size());
        assertEquals(mainAutofillAndPasswordsEntry.id, breadcrumbEntries.get(0).id);
        assertEquals(homeAutofillOptionsEntry.id, breadcrumbEntries.get(1).id);
    }

    private void buildIndex() {
        ThreadUtils.runOnUiThreadBlocking(
                () -> SettingsSearchCoordinator.buildIndexInternal(mContext, mProfile, mIndexData));
    }

    private SettingsIndexData.Entry getPrivateWindowEntry() {
        SettingsIndexData.Entry entry =
                mIndexData.getEntry(
                        PreferenceParser.createUniqueId(
                                AutofillOptionsFragment.class.getName(),
                                BraveAutofillOptionsFragmentBase.PREF_AUTOFILL_PRIVATE_WINDOW));
        assertNotNull(entry);
        return entry;
    }

    private void assertSettingsReferrerExtras(SettingsIndexData.Entry entry) {
        assertTrue(entry.extras.containsKey(AutofillOptionsFragment.AUTOFILL_OPTIONS_REFERRER));
        assertEquals(
                AutofillOptionsReferrer.SETTINGS,
                entry.extras.getInt(AutofillOptionsFragment.AUTOFILL_OPTIONS_REFERRER));
    }

    private void assertHomeOfTransactionsReferrerExtras(SettingsIndexData.Entry entry) {
        assertTrue(entry.extras.containsKey(HomeOfTransactionsFragment.EXTRA_REFERRER));
        assertEquals(
                HomeOfTransactionsFragment.AutofillSettingsReferrer.SETTINGS_MENU,
                entry.extras.getInt(HomeOfTransactionsFragment.EXTRA_REFERRER));
    }

    private void assertSearchFindsPrivateWindowEntry(SettingsIndexData.Entry entry) {
        SettingsIndexData.SearchResults searchResults =
                mIndexData.search(mContext.getString(R.string.prefs_autofill_private_window_title));

        assertTrue(
                searchResults.getItems().stream()
                        .anyMatch(searchEntry -> entry.id.equals(searchEntry.id)));
    }
}
