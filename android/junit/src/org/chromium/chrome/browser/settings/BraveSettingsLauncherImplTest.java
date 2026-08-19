/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.settings;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertTrue;
import static org.robolectric.Shadows.shadowOf;

import android.app.Application;
import android.content.Intent;
import android.os.Bundle;

import androidx.test.core.app.ApplicationProvider;
import androidx.test.filters.SmallTest;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.annotation.Config;

import org.chromium.base.test.BaseRobolectricTestRunner;
import org.chromium.base.test.util.Features.DisableFeatures;
import org.chromium.chrome.browser.browsing_data.BraveClearBrowsingDataFragment;
import org.chromium.chrome.browser.browsing_data.ClearBrowsingDataFragment;
import org.chromium.chrome.browser.download.settings.BraveDownloadSettings;
import org.chromium.chrome.browser.download.settings.DownloadSettings;
import org.chromium.chrome.browser.flags.ChromeFeatureList;
import org.chromium.chrome.browser.safe_browsing.settings.BraveStandardProtectionSettingsFragment;
import org.chromium.chrome.browser.safe_browsing.settings.StandardProtectionSettingsFragment;
import org.chromium.components.browser_ui.settings.SettingsNavigation.SettingsFragment;

/** Tests for {@link BraveSettingsLauncherImpl}. */
@RunWith(BaseRobolectricTestRunner.class)
@Config(manifest = Config.NONE)
// Settings fragments are EmbeddableSettingsPages, so SettingsIntentUtil reads this flag while
// building the intent. Pin it so these tests don't depend on the field trial state.
@DisableFeatures(ChromeFeatureList.SETTINGS_SINGLE_ACTIVITY)
public class BraveSettingsLauncherImplTest {
    private Application mContext;
    private BraveSettingsLauncherImpl mSettingsNavigation;

    @Before
    public void setUp() {
        mContext = ApplicationProvider.getApplicationContext();
        mSettingsNavigation = new BraveSettingsLauncherImpl();
    }

    /**
     * Entry points that pass a {@link SettingsFragment} value instead of a fragment class must
     * still get the Brave version of the fragment.
     *
     * <p>Regression test for https://github.com/brave/brave-browser/issues/57217: Quick Delete's
     * "More options" (reachable from the tab switcher menu) and the History page's "Delete browsing
     * data" use this overload, and used to land on the upstream screen, which has no Leo AI option
     * and no "Clear Brave Ads data" entry.
     */
    @Test
    @SmallTest
    public void testStartSettingsWithEnumSubstitutesFragment() {
        mSettingsNavigation.startSettings(mContext, SettingsFragment.CLEAR_BROWSING_DATA);

        Intent intent = shadowOf(mContext).getNextStartedActivity();
        assertNotNull(intent);
        assertShowsBraveClearBrowsingData(intent);
    }

    @Test
    @SmallTest
    public void testCreateSettingsIntentWithEnumSubstitutesFragment() {
        Intent intent =
                mSettingsNavigation.createSettingsIntent(
                        mContext, SettingsFragment.CLEAR_BROWSING_DATA, /* fragmentArgs= */ null);

        assertShowsBraveClearBrowsingData(intent);
    }

    @Test
    @SmallTest
    public void testStartSettingsWithFragmentClassSubstitutesFragment() {
        mSettingsNavigation.startSettings(
                mContext, ClearBrowsingDataFragment.class, /* fragmentArgs= */ null);

        Intent intent = shadowOf(mContext).getNextStartedActivity();
        assertNotNull(intent);
        assertShowsBraveClearBrowsingData(intent);
    }

    /**
     * Covers the way {@link org.chromium.chrome.browser.site_settings.BraveSiteSettingsDelegate}
     * opens the screen: fragment args plus addToBackStack, which travel through overloads that used
     * to bypass the substitution.
     */
    @Test
    @SmallTest
    public void testStartSettingsPreservesArgsAndBackStack() {
        Bundle fragmentArgs = ClearBrowsingDataFragment.createFragmentArgs("referrer");

        mSettingsNavigation.startSettings(
                mContext,
                ClearBrowsingDataFragment.class,
                fragmentArgs,
                /* addToBackStack= */ true);

        Intent intent = shadowOf(mContext).getNextStartedActivity();
        assertNotNull(intent);
        assertShowsBraveClearBrowsingData(intent);
        assertNotNull(intent.getBundleExtra(SettingsIntentUtil.EXTRA_SHOW_FRAGMENT_ARGUMENTS));
        assertTrue(intent.getBooleanExtra(SettingsIntentUtil.EXTRA_ADD_TO_BACK_STACK, false));
    }

    @Test
    @SmallTest
    public void testOtherFragmentsAreStillSubstituted() {
        assertEquals(
                BraveDownloadSettings.class.getName(),
                showFragmentName(
                        mSettingsNavigation.createSettingsIntent(
                                mContext, DownloadSettings.class)));
        assertEquals(
                BraveStandardProtectionSettingsFragment.class.getName(),
                showFragmentName(
                        mSettingsNavigation.createSettingsIntent(
                                mContext, StandardProtectionSettingsFragment.class)));
    }

    @Test
    @SmallTest
    public void testMainSettingsStillOpensBraveSettingsActivity() {
        Intent intent =
                mSettingsNavigation.createSettingsIntent(
                        mContext, SettingsFragment.MAIN, /* fragmentArgs= */ null);

        // MAIN maps to a null fragment, so there is nothing to substitute, but the intent must
        // still target the Brave settings activity.
        assertNull(showFragmentName(intent));
        assertEquals(BraveSettingsActivity.class.getName(), intent.getComponent().getClassName());
    }

    private void assertShowsBraveClearBrowsingData(Intent intent) {
        assertEquals(BraveClearBrowsingDataFragment.class.getName(), showFragmentName(intent));
        // BraveSettingsActivity is what provides the Brave overflow menu, so the substituted
        // fragment has to be hosted by it rather than by the upstream SettingsActivity.
        assertEquals(BraveSettingsActivity.class.getName(), intent.getComponent().getClassName());
    }

    private static String showFragmentName(Intent intent) {
        return intent.getStringExtra(SettingsIntentUtil.EXTRA_SHOW_FRAGMENT);
    }
}
