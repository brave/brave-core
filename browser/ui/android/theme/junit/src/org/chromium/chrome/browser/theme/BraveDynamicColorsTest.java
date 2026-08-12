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
    public void testIsDynamicColorsAvailable_androidS_returnsTrue() {
        assertTrue(BraveDynamicColors.isDynamicColorsAvailable());
    }

    @Test
    @Config(sdk = Build.VERSION_CODES.R)
    public void testIsDynamicColorsAvailable_belowAndroidS_returnsFalse() {
        assertFalse(BraveDynamicColors.isDynamicColorsAvailable());
    }

    @Test
    public void testIsDynamicColorsEnabled_defaultEnabled_userPreferenceUnset_returnsTrue() {
        assertTrue(BraveDynamicColors.isDynamicColorsEnabled());
    }

    @Test
    @DisableFeatures(BraveFeatureList.BRAVE_ANDROID_DYNAMIC_COLORS_BY_DEFAULT)
    public void testIsDynamicColorsEnabled_defaultDisabled_userPreferenceUnset_returnsFalse() {
        assertFalse(BraveDynamicColors.isDynamicColorsEnabled());
    }

    @Test
    @EnableFeatures(BraveFeatureList.BRAVE_ANDROID_DYNAMIC_COLORS_BY_DEFAULT)
    public void
            testIsDynamicColorsEnabled_userPreferenceDisabledOverridesDefaultEnabled_returnsFalse() {
        ChromeSharedPreferences.getInstance()
                .writeBoolean(BravePreferenceKeys.BRAVE_ANDROID_DYNAMIC_COLORS_ENABLED, false);

        assertTrue(BraveDynamicColors.isDynamicColorsAvailable());
        assertFalse(BraveDynamicColors.isDynamicColorsEnabled());
    }

    @Test
    @DisableFeatures(BraveFeatureList.BRAVE_ANDROID_DYNAMIC_COLORS_BY_DEFAULT)
    public void
            testIsDynamicColorsEnabled_userPreferenceEnabledOverridesDefaultDisabled_returnsTrue() {
        ChromeSharedPreferences.getInstance()
                .writeBoolean(BravePreferenceKeys.BRAVE_ANDROID_DYNAMIC_COLORS_ENABLED, true);

        assertTrue(BraveDynamicColors.isDynamicColorsAvailable());
        assertTrue(BraveDynamicColors.isDynamicColorsEnabled());
    }

    @Test
    @Config(sdk = Build.VERSION_CODES.R)
    public void testIsDynamicColorsEnabled_userPreferenceEnabledBelowAndroidS_returnsFalse() {
        ChromeSharedPreferences.getInstance()
                .writeBoolean(BravePreferenceKeys.BRAVE_ANDROID_DYNAMIC_COLORS_ENABLED, true);

        assertFalse(BraveDynamicColors.isDynamicColorsEnabled());
    }
}
