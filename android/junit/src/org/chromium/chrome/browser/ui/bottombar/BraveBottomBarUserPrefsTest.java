/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.ui.bottombar;

import static org.junit.Assert.assertEquals;

import org.junit.After;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.annotation.Config;

import org.chromium.base.BravePreferenceKeys;
import org.chromium.base.ContextUtils;
import org.chromium.base.test.BaseRobolectricTestRunner;
import org.chromium.base.test.util.Features.DisableFeatures;
import org.chromium.base.test.util.Features.EnableFeatures;
import org.chromium.chrome.browser.flags.ChromeFeatureList;
import org.chromium.chrome.browser.preferences.ChromePreferenceKeys;
import org.chromium.chrome.browser.preferences.ChromeSharedPreferences;
import org.chromium.chrome.browser.toolbar.ToolbarVariationUtils;

/**
 * Unit tests for the bottom bar setting's Brave defaults.
 *
 * <p>These fail if a rebase drops the BottomBarConfigUtils patch.
 */
@RunWith(BaseRobolectricTestRunner.class)
@Config(manifest = Config.NONE)
public class BraveBottomBarUserPrefsTest {
    @After
    public void tearDown() {
        ChromeSharedPreferences.getInstance().removeKey(ChromePreferenceKeys.BOTTOM_BAR_ENABLED);
        ChromeSharedPreferences.getInstance()
                .removeKey(BravePreferenceKeys.BRAVE_ENABLE_BOTTOM_BAR);
        ChromeSharedPreferences.getInstance()
                .removeKey(BravePreferenceKeys.BRAVE_BOTTOM_TOOLBAR_ENABLED_KEY);
    }

    private void setFormerBottomBarSetting(boolean enabled) {
        ChromeSharedPreferences.getInstance()
                .writeBoolean(BravePreferenceKeys.BRAVE_ENABLE_BOTTOM_BAR, enabled);
    }

    private void setBottomToolbarSetting(boolean enabled) {
        ChromeSharedPreferences.getInstance()
                .writeBoolean(BravePreferenceKeys.BRAVE_BOTTOM_TOOLBAR_ENABLED_KEY, enabled);
    }

    private void assertBottomBarGates(boolean expected) {
        assertEquals(
                "BottomBarConfigUtils.isBottomBarEnabled",
                expected,
                BottomBarConfigUtils.isBottomBarEnabled(ContextUtils.getApplicationContext()));
        // The refactor has to follow the bottom bar, otherwise neither bar carries the tab switcher
        // and app menu buttons.
        assertEquals(
                "ToolbarVariationUtils.isToolbarUiRefactorEnabled",
                expected,
                ToolbarVariationUtils.isToolbarUiRefactorEnabled(
                        ContextUtils.getApplicationContext()));
    }

    @Test
    @EnableFeatures(ChromeFeatureList.ANDROID_BOTTOM_BAR)
    public void testEnabledWhenNoSettingIsSet() {
        assertBottomBarGates(true);
    }

    @Test
    @EnableFeatures(ChromeFeatureList.ANDROID_BOTTOM_BAR)
    public void testUnsetSettingInheritsFormerBottomBarSettingOff() {
        setFormerBottomBarSetting(false);
        assertBottomBarGates(false);
    }

    @Test
    @EnableFeatures(ChromeFeatureList.ANDROID_BOTTOM_BAR)
    public void testUnsetSettingInheritsBottomToolbarSettingOff() {
        setBottomToolbarSetting(false);
        assertBottomBarGates(false);
    }

    @Test
    @EnableFeatures(ChromeFeatureList.ANDROID_BOTTOM_BAR)
    public void testFormerBottomBarSettingOverridesBottomToolbarSetting() {
        setBottomToolbarSetting(false);
        setFormerBottomBarSetting(true);
        assertBottomBarGates(true);
    }

    @Test
    @EnableFeatures(ChromeFeatureList.ANDROID_BOTTOM_BAR)
    public void testSettingOverridesBraveDefaults() {
        setFormerBottomBarSetting(false);
        setBottomToolbarSetting(false);
        BottomBarConfigUtils.setBottomBarUserEnabled(true);
        assertBottomBarGates(true);

        BottomBarConfigUtils.setBottomBarUserEnabled(false);
        assertBottomBarGates(false);
    }

    @Test
    @DisableFeatures(ChromeFeatureList.ANDROID_BOTTOM_BAR)
    public void testSettingOnDoesNotEnableBottomBarWithoutFlag() {
        BottomBarConfigUtils.setBottomBarUserEnabled(true);
        assertBottomBarGates(false);
    }
}
