/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.download;

import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;

import androidx.test.filters.SmallTest;

import org.junit.After;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.ThreadUtils;
import org.chromium.base.test.util.Batch;
import org.chromium.base.test.util.CommandLineFlags;
import org.chromium.chrome.browser.download.settings.BraveDownloadSettings;
import org.chromium.chrome.browser.flags.ChromeSwitches;
import org.chromium.chrome.browser.settings.SettingsActivityTestRule;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;
import org.chromium.components.browser_ui.settings.ChromeSwitchPreference;

/** Test for Brave download settings. */
@RunWith(ChromeJUnit4ClassRunner.class)
@CommandLineFlags.Add({ChromeSwitches.DISABLE_FIRST_RUN_EXPERIENCE})
@Batch(Batch.PER_CLASS)
public class BraveDownloadSettingsTest {
    @Rule
    public final SettingsActivityTestRule<BraveDownloadSettings> mSettingsActivityTestRule =
            new SettingsActivityTestRule<>(BraveDownloadSettings.class);

    private BraveDownloadSettings mFragment;

    @Before
    public void setUp() {
        mSettingsActivityTestRule.startSettingsActivity();
        mFragment = (BraveDownloadSettings) mSettingsActivityTestRule.getFragment();
    }

    @After
    public void tearDown() {
        mSettingsActivityTestRule.getActivity().finish();
    }

    @Test
    @SmallTest
    public void testDefaultDownloadNotificationBubbleState() throws Exception {
        ThreadUtils.runOnUiThreadBlocking(
                () -> {
                    ChromeSwitchPreference downloadNotificationBubble =
                            mFragment.findPreference(
                                    BraveDownloadSettings
                                            .PREF_DOWNLOAD_PROGRESS_NOTIFICATION_BUBBLE);
                    assertNotNull(downloadNotificationBubble);
                    assertTrue(downloadNotificationBubble.isChecked());
                });
    }
}
