/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.theme;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import android.os.Build;

import org.junit.After;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.annotation.Config;

import org.chromium.base.BraveFeatureList;
import org.chromium.base.BravePreferenceKeys;
import org.chromium.base.test.BaseRobolectricTestRunner;
import org.chromium.base.test.util.Features.DisableFeatures;
import org.chromium.base.test.util.Features.EnableFeatures;
import org.chromium.chrome.browser.preferences.ChromeSharedPreferences;

/** Unit tests for {@link BraveDynamicColors}. */
@RunWith(BaseRobolectricTestRunner.class)
@Config(manifest = Config.NONE, sdk = Build.VERSION_CODES.S)
public class BraveDynamicColorsTest {
    @After
    public void tearDown() {
        ChromeSharedPreferences.getInstance()
                .removeKey(BravePreferenceKeys.BRAVE_ANDROID_DYNAMIC_COLORS_ENABLED);
    }

    @Test
    @DisableFeatures(BraveFeatureList.BRAVE_ANDROID_DYNAMIC_COLORS)
    public void testIsDynamicColorsAvailable_featureDisabled_returnsFalse() {
        assertFalse(BraveDynamicColors.isDynamicColorsAvailable());
    }

    @Test
    @EnableFeatures(BraveFeatureList.BRAVE_ANDROID_DYNAMIC_COLORS)
    @Config(sdk = Build.VERSION_CODES.R)
    public void testIsDynamicColorsAvailable_belowAndroidS_returnsFalse() {
        assertFalse(BraveDynamicColors.isDynamicColorsAvailable());
    }

    @Test
    @EnableFeatures(BraveFeatureList.BRAVE_ANDROID_DYNAMIC_COLORS)
    public void testIsDynamicColorsAvailable_androidSAndFeatureEnabled_returnsTrue() {
        assertTrue(BraveDynamicColors.isDynamicColorsAvailable());
    }

    @Test
    @EnableFeatures(BraveFeatureList.BRAVE_ANDROID_DYNAMIC_COLORS)
    public void testIsDynamicColorsEnabled_userPreferenceUnset_returnsTrue() {
        assertTrue(BraveDynamicColors.isDynamicColorsEnabled());
    }

    @Test
    @EnableFeatures(BraveFeatureList.BRAVE_ANDROID_DYNAMIC_COLORS)
    public void testIsDynamicColorsEnabled_userPreferenceDisabled_returnsFalse() {
        ChromeSharedPreferences.getInstance()
                .writeBoolean(BravePreferenceKeys.BRAVE_ANDROID_DYNAMIC_COLORS_ENABLED, false);

        assertTrue(BraveDynamicColors.isDynamicColorsAvailable());
        assertFalse(BraveDynamicColors.isDynamicColorsEnabled());
    }

    @Test
    @DisableFeatures(BraveFeatureList.BRAVE_ANDROID_DYNAMIC_COLORS)
    public void testIsDynamicColorsEnabled_featureDisabled_returnsFalse() {
        assertFalse(BraveDynamicColors.isDynamicColorsEnabled());
    }

    @Test
    @EnableFeatures(BraveFeatureList.BRAVE_ANDROID_DYNAMIC_COLORS)
    @Config(sdk = Build.VERSION_CODES.R)
    public void testIsDynamicColorsEnabled_belowAndroidS_returnsFalse() {
        assertFalse(BraveDynamicColors.isDynamicColorsEnabled());
    }
}
