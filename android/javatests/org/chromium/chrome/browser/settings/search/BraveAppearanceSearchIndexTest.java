/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.settings.search;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertNull;

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
import org.chromium.base.test.util.Features.EnableFeatures;
import org.chromium.base.test.util.Restriction;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.appearance.settings.AppearanceSettingsFragment;
import org.chromium.chrome.browser.flags.ChromeFeatureList;
import org.chromium.chrome.browser.flags.ChromeSwitches;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.settings.AppearancePreferences;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;
import org.chromium.chrome.test.ChromeTabbedActivityTestRule;
import org.chromium.components.browser_ui.settings.search.SettingsIndexData;
import org.chromium.ui.base.DeviceFormFactor;

/** Tests the Appearance settings search index against the rows the screen actually shows. */
@RunWith(ChromeJUnit4ClassRunner.class)
@Batch(Batch.PER_CLASS)
@CommandLineFlags.Add({ChromeSwitches.DISABLE_FIRST_RUN_EXPERIENCE})
@DisableLeakChecks("crbug.com/512492984 (SearchIndexProviderRegistryTest)")
public class BraveAppearanceSearchIndexTest {
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
                    mIndexData = SettingsIndexData.createInstance();
                });
    }

    @After
    public void tearDown() {
        ThreadUtils.runOnUiThreadBlocking(SettingsIndexData::reset);
    }

    // Vertical tabs are tablet only, so the row is not on the screen on a phone.
    @Test
    @SmallTest
    @Restriction(DeviceFormFactor.PHONE)
    public void testTabPositionNotIndexed() {
        buildIndex();

        assertNull(getEntry(AppearanceSettingsFragment.PREF_TAB_POSITION));
    }

    // The bookmark bar is not available on a phone, and the screen inflates two rows for it.
    @Test
    @SmallTest
    @Restriction(DeviceFormFactor.PHONE)
    public void testBookmarkBarNotIndexed() {
        buildIndex();

        assertNull(getEntry(AppearanceSettingsFragment.PREF_BOOKMARK_BAR));
        assertNull(getEntry(AppearanceSettingsFragment.PREF_BOOKMARK_BAR_SWITCH));
    }

    // The row is retitled on the screen when the bottom bar moves the address bar settings here.
    @Test
    @SmallTest
    @Restriction(DeviceFormFactor.PHONE)
    @EnableFeatures(ChromeFeatureList.ANDROID_BOTTOM_BAR)
    public void testToolbarShortcutRetitledWithAndroidBottomBar() {
        buildIndex();

        SettingsIndexData.Entry entry = getEntry(AppearanceSettingsFragment.PREF_TOOLBAR_SHORTCUT);
        assertNotNull(entry);
        assertEquals(mContext.getString(R.string.address_bar_shortcut), entry.title);
    }

    private void buildIndex() {
        ThreadUtils.runOnUiThreadBlocking(
                () -> SettingsSearchCoordinator.buildIndexInternal(mContext, mProfile, mIndexData));
    }

    private SettingsIndexData.Entry getEntry(String key) {
        return mIndexData.getEntryForKey(AppearancePreferences.class.getName(), key);
    }
}
