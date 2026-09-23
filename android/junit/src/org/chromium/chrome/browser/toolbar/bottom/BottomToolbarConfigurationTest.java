/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.toolbar.bottom;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.annotation.Config;

import org.chromium.base.BravePreferenceKeys;
import org.chromium.base.test.BaseRobolectricTestRunner;
import org.chromium.base.test.util.Features.DisableFeatures;
import org.chromium.base.test.util.Features.EnableFeatures;
import org.chromium.chrome.browser.flags.ChromeFeatureList;
import org.chromium.chrome.browser.preferences.ChromePreferenceKeys;
import org.chromium.chrome.browser.preferences.ChromeSharedPreferences;
import org.chromium.chrome.browser.toolbar.ToolbarPositionController.ToolbarPositionAndSource;

/** Unit tests for {@link BottomToolbarConfiguration}. */
@RunWith(BaseRobolectricTestRunner.class)
@Config(manifest = Config.NONE)
public class BottomToolbarConfigurationTest {
    @Before
    public void setUp() {
        // Pre-set the "bottom toolbar initialized" flags to bypass the isSmallScreen() call, which
        // requires a running Activity, and anchor the address bar on top, which is the only
        // configuration where Brave's bottom controls are used.
        ChromeSharedPreferences.getInstance()
                .writeBoolean(BravePreferenceKeys.BRAVE_BOTTOM_TOOLBAR_SET_KEY, true);
        ChromeSharedPreferences.getInstance()
                .writeBoolean(BravePreferenceKeys.BRAVE_BOTTOM_TOOLBAR_ENABLED_KEY, true);
        ChromeSharedPreferences.getInstance()
                .writeInt(
                        ChromePreferenceKeys.TOOLBAR_TOP_ANCHORED,
                        ToolbarPositionAndSource.TOP_SETTINGS);
    }

    @After
    public void tearDown() {
        ChromeSharedPreferences.getInstance()
                .removeKey(BravePreferenceKeys.BRAVE_BOTTOM_TOOLBAR_SET_KEY);
        ChromeSharedPreferences.getInstance()
                .removeKey(BravePreferenceKeys.BRAVE_BOTTOM_TOOLBAR_ENABLED_KEY);
        ChromeSharedPreferences.getInstance().removeKey(ChromePreferenceKeys.TOOLBAR_TOP_ANCHORED);
    }

    @Test
    @DisableFeatures(ChromeFeatureList.ANDROID_BOTTOM_BAR)
    public void testBraveBottomControlsEnabledWithoutAndroidBottomBar() {
        assertFalse(BottomToolbarConfiguration.isAndroidBottomBarEnabled());
        assertTrue(BottomToolbarConfiguration.isBraveBottomControlsEnabled());
    }

    @Test
    @EnableFeatures(ChromeFeatureList.ANDROID_BOTTOM_BAR)
    public void testBraveBottomControlsDisabledByAndroidBottomBar() {
        assertTrue(BottomToolbarConfiguration.isAndroidBottomBarEnabled());
        assertFalse(BottomToolbarConfiguration.isBraveBottomControlsEnabled());
    }
}
