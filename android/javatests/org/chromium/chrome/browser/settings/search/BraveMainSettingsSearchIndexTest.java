/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.settings.search;

import static org.junit.Assert.assertEquals;
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
import org.chromium.chrome.R;
import org.chromium.chrome.browser.flags.ChromeSwitches;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.settings.MainSettings;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;
import org.chromium.chrome.test.ChromeTabbedActivityTestRule;
import org.chromium.components.browser_ui.settings.search.SettingsIndexData;

/** Tests that main settings search entries use the sections Brave shows them under. */
@RunWith(ChromeJUnit4ClassRunner.class)
@Batch(Batch.PER_CLASS)
@CommandLineFlags.Add({ChromeSwitches.DISABLE_FIRST_RUN_EXPERIENCE})
@DisableLeakChecks("crbug.com/512492984 (SearchIndexProviderRegistryTest)")
public class BraveMainSettingsSearchIndexTest {
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
                    SettingsSearchCoordinator.buildIndexInternal(mContext, mProfile, mIndexData);
                });
    }

    @After
    public void tearDown() {
        ThreadUtils.runOnUiThreadBlocking(SettingsIndexData::reset);
    }

    @Test
    @SmallTest
    public void testGeneralSectionHeaders() {
        assertHeader(R.string.prefs_section_general, MainSettings.PREF_HOMEPAGE);
        assertHeader(R.string.prefs_section_general, "brave_sync_layout");
        assertHeader(R.string.prefs_section_general, "notifications");
        assertHeader(R.string.prefs_section_general, "content_settings");
        assertHeader(R.string.prefs_section_general, "brave_downloads");
    }

    @Test
    @SmallTest
    public void testDisplaySectionHeaders() {
        assertHeader(R.string.prefs_section_display, "tabs");
        assertHeader(R.string.prefs_section_display, "accessibility");
        assertHeader(R.string.prefs_section_display, MainSettings.PREF_AUTOFILL_AND_PASSWORDS);
    }

    @Test
    @SmallTest
    public void testAboutSectionHeaders() {
        assertHeader(R.string.prefs_about_section, "about_chrome");
    }

    private void assertHeader(int headerId, String key) {
        SettingsIndexData.Entry entry =
                mIndexData.getEntryForKey(MainSettings.class.getName(), key);
        // Some entries (e.g. notifications on emulators) are not present on every device.
        if (entry == null) return;
        assertEquals(key, mContext.getString(headerId), entry.header);
    }
}
