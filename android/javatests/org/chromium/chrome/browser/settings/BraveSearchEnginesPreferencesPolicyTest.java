/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.settings;

import static org.chromium.chrome.browser.settings.BraveSearchEnginesPreferences.PREF_PRIVATE_SEARCH_ENGINE;
import static org.chromium.chrome.browser.settings.BraveSearchEnginesPreferences.PREF_STANDARD_SEARCH_ENGINE;

import androidx.test.filters.SmallTest;

import org.junit.Assert;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.ThreadUtils;
import org.chromium.base.test.util.CriteriaHelper;
import org.chromium.base.test.util.DoNotBatch;
import org.chromium.chrome.browser.incognito.IncognitoUtils;
import org.chromium.chrome.browser.init.ChromeBrowserInitializer;
import org.chromium.chrome.browser.profiles.ProfileManager;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;
import org.chromium.components.policy.test.annotations.Policies;

/**
 * Tests that the private search engine row tracks incognito availability.
 *
 * <p>Rendering that row used to create the primary OTR profile, which CHECK-fails in
 * OffTheRecordProfileImpl::Init when incognito is disabled, so the disabled case aborted the
 * browser instead of failing an assertion.
 */
@RunWith(ChromeJUnit4ClassRunner.class)
@DoNotBatch(reason = "Each test launches a Settings activity and applies policies.")
public class BraveSearchEnginesPreferencesPolicyTest {
    @Rule
    public final SettingsActivityTestRule<BraveSearchEnginesPreferences> mSettingsActivityTestRule =
            new SettingsActivityTestRule<>(BraveSearchEnginesPreferences.class);

    @Test
    @SmallTest
    public void testPrivateSearchEngineShownWhenIncognitoAllowed() {
        startBrowserAndWaitForIncognitoAvailability(true);

        BraveSearchEnginesPreferences fragment = startSettings();
        Assert.assertNotNull(fragment.findPreference(PREF_STANDARD_SEARCH_ENGINE));
        Assert.assertNotNull(
                "Private search engine row should be present by default.",
                fragment.findPreference(PREF_PRIVATE_SEARCH_ENGINE));
    }

    @Test
    @SmallTest
    @Policies.Add({@Policies.Item(key = "IncognitoModeAvailability", string = "1")})
    public void testPrivateSearchEngineHiddenWhenIncognitoDisabled() {
        startBrowserAndWaitForIncognitoAvailability(false);

        BraveSearchEnginesPreferences fragment = startSettings();
        Assert.assertNotNull(fragment.findPreference(PREF_STANDARD_SEARCH_ENGINE));
        Assert.assertNull(
                "Private search engine row must be hidden when incognito is disabled.",
                fragment.findPreference(PREF_PRIVATE_SEARCH_ENGINE));
    }

    /**
     * Starts the browser and blocks until incognito availability settles to {@code expected}.
     *
     * <p>Policies declared with {@link Policies.Add} reach native asynchronously, so the settings
     * screen must not be launched before the pref that OffTheRecordProfileImpl::Init reads has
     * actually been updated. A timeout here means the policy never applied, which is a different
     * failure from the row being wrong.
     */
    private void startBrowserAndWaitForIncognitoAvailability(boolean expected) {
        ThreadUtils.runOnUiThreadBlocking(
                () -> ChromeBrowserInitializer.getInstance().handleSynchronousStartup());
        CriteriaHelper.pollUiThread(
                () ->
                        IncognitoUtils.isIncognitoModeEnabled(
                                        ProfileManager.getLastUsedRegularProfile())
                                == expected,
                "Incognito availability never settled to " + expected + "; policy did not apply.");
    }

    private BraveSearchEnginesPreferences startSettings() {
        mSettingsActivityTestRule.startSettingsActivity();
        BraveSearchEnginesPreferences fragment = mSettingsActivityTestRule.getFragment();
        Assert.assertNotNull("SettingsActivity failed to launch.", fragment);
        return fragment;
    }
}
