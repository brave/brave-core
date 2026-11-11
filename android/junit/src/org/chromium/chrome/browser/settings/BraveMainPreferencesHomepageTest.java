/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.settings;

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
import org.chromium.ui.base.DeviceFormFactor;

/** Tests when the main settings show the homepage setting. */
@RunWith(BaseRobolectricTestRunner.class)
@Config(manifest = Config.NONE)
public class BraveMainPreferencesHomepageTest {
    @Before
    public void setUp() {
        DeviceFormFactor.setIsTabletForTesting(false);
        // Bypasses the first-run isSmallScreen() call, which requires a running Activity.
        ChromeSharedPreferences.getInstance()
                .writeBoolean(BravePreferenceKeys.BRAVE_BOTTOM_TOOLBAR_SET_KEY, true);
        setAddressBarOnTop(true);
    }

    @After
    public void tearDown() {
        ChromeSharedPreferences.getInstance()
                .removeKey(BravePreferenceKeys.BRAVE_BOTTOM_TOOLBAR_SET_KEY);
        ChromeSharedPreferences.getInstance()
                .removeKey(BravePreferenceKeys.BRAVE_BOTTOM_TOOLBAR_ENABLED_KEY);
        ChromeSharedPreferences.getInstance()
                .removeKey(BravePreferenceKeys.BRAVE_ENABLE_BOTTOM_BAR);
        ChromeSharedPreferences.getInstance().removeKey(ChromePreferenceKeys.TOOLBAR_TOP_ANCHORED);
    }

    private void setAddressBarOnTop(boolean onTop) {
        ChromeSharedPreferences.getInstance()
                .writeInt(
                        ChromePreferenceKeys.TOOLBAR_TOP_ANCHORED,
                        onTop
                                ? ToolbarPositionAndSource.TOP_SETTINGS
                                : ToolbarPositionAndSource.BOTTOM_SETTINGS);
    }

    private void setBottomToolbarSetting(boolean enabled) {
        ChromeSharedPreferences.getInstance()
                .writeBoolean(BravePreferenceKeys.BRAVE_BOTTOM_TOOLBAR_ENABLED_KEY, enabled);
    }

    private void setBottomBarSetting(boolean enabled) {
        ChromeSharedPreferences.getInstance()
                .writeBoolean(BravePreferenceKeys.BRAVE_ENABLE_BOTTOM_BAR, enabled);
    }

    @Test
    @DisableFeatures(ChromeFeatureList.ANDROID_BOTTOM_BAR)
    public void testShownWithBraveBottomControls() {
        setBottomToolbarSetting(true);
        assertTrue(BraveMainPreferencesBase.shouldShowHomepagePreference());
    }

    @Test
    @DisableFeatures(ChromeFeatureList.ANDROID_BOTTOM_BAR)
    public void testHiddenWithoutBottomControlsAndAddressBarOnTop() {
        setBottomToolbarSetting(false);
        assertFalse(BraveMainPreferencesBase.shouldShowHomepagePreference());
    }

    @Test
    @DisableFeatures(ChromeFeatureList.ANDROID_BOTTOM_BAR)
    public void testShownWithAddressBarOnBottom() {
        setBottomToolbarSetting(false);
        setAddressBarOnTop(false);
        assertTrue(BraveMainPreferencesBase.shouldShowHomepagePreference());
    }

    @Test
    @DisableFeatures(ChromeFeatureList.ANDROID_BOTTOM_BAR)
    public void testShownOnTablet() {
        DeviceFormFactor.setIsTabletForTesting(true);
        setBottomToolbarSetting(false);
        assertTrue(BraveMainPreferencesBase.shouldShowHomepagePreference());
    }

    // The bottom bar replaces Brave's bottom controls but still carries a home button.
    @Test
    @EnableFeatures(ChromeFeatureList.ANDROID_BOTTOM_BAR)
    public void testShownWithBottomBarAndAddressBarOnTop() {
        setBottomBarSetting(true);
        assertTrue(BraveMainPreferencesBase.shouldShowHomepagePreference());
    }

    @Test
    @EnableFeatures(ChromeFeatureList.ANDROID_BOTTOM_BAR)
    public void testHiddenWithBottomBarSwitchedOffAndAddressBarOnTop() {
        setBottomBarSetting(false);
        assertFalse(BraveMainPreferencesBase.shouldShowHomepagePreference());
    }
}
