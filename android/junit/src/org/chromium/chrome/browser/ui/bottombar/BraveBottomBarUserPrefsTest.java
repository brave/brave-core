/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.ui.bottombar;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

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
import org.chromium.chrome.browser.preferences.ChromeSharedPreferences;
import org.chromium.chrome.browser.toolbar.ToolbarVariationUtils;

/**
 * Unit tests for the "Enable bottom bar" setting.
 *
 * <p>Both gates below are patched into upstream, and upstream keeps them in step with each other
 * through LINT.IfChange. These tests fail if a rebase drops either patch, or applies only one of
 * them.
 */
@RunWith(BaseRobolectricTestRunner.class)
@Config(manifest = Config.NONE)
public class BraveBottomBarUserPrefsTest {
    @After
    public void tearDown() {
        ChromeSharedPreferences.getInstance()
                .removeKey(BravePreferenceKeys.BRAVE_ENABLE_BOTTOM_BAR);
        ChromeSharedPreferences.getInstance()
                .removeKey(BravePreferenceKeys.BRAVE_BOTTOM_TOOLBAR_ENABLED_KEY);
    }

    private void setBottomBarSetting(boolean enabled) {
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
    public void testEnabledWhenNeitherSettingIsSet() {
        assertTrue(BraveBottomBarUserPrefs.isBottomBarEnabled());
        assertBottomBarGates(true);
    }

    @Test
    @EnableFeatures(ChromeFeatureList.ANDROID_BOTTOM_BAR)
    public void testSettingOnKeepsBottomBar() {
        setBottomBarSetting(true);
        assertBottomBarGates(true);
    }

    @Test
    @EnableFeatures(ChromeFeatureList.ANDROID_BOTTOM_BAR)
    public void testSettingOffDisablesBottomBar() {
        setBottomBarSetting(false);
        assertFalse(BraveBottomBarUserPrefs.isBottomBarEnabled());
        assertBottomBarGates(false);
    }

    // Until it is set the setting inherits the bottom navigation toolbar setting, so switching the
    // flag on does not hand a bottom bar back to someone who turned that one off.
    @Test
    @EnableFeatures(ChromeFeatureList.ANDROID_BOTTOM_BAR)
    public void testUnsetSettingInheritsBottomToolbarSetting() {
        setBottomToolbarSetting(false);
        assertFalse(BraveBottomBarUserPrefs.isBottomBarEnabled());
        assertBottomBarGates(false);

        setBottomToolbarSetting(true);
        assertTrue(BraveBottomBarUserPrefs.isBottomBarEnabled());
        assertBottomBarGates(true);
    }

    @Test
    @EnableFeatures(ChromeFeatureList.ANDROID_BOTTOM_BAR)
    public void testSettingOverridesBottomToolbarSetting() {
        setBottomToolbarSetting(false);
        setBottomBarSetting(true);
        assertTrue(BraveBottomBarUserPrefs.isBottomBarEnabled());
        assertBottomBarGates(true);
    }

    @Test
    @DisableFeatures(ChromeFeatureList.ANDROID_BOTTOM_BAR)
    public void testSettingOnDoesNotEnableBottomBarWithoutFlag() {
        setBottomBarSetting(true);
        assertBottomBarGates(false);
    }
}
