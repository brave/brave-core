/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.toolbar.adaptive;

import static org.chromium.chrome.browser.preferences.ChromePreferenceKeys.ADAPTIVE_TOOLBAR_CUSTOMIZATION_ENABLED;
import static org.chromium.chrome.browser.preferences.ChromePreferenceKeys.ADAPTIVE_TOOLBAR_CUSTOMIZATION_SETTINGS;

import org.chromium.base.ContextUtils;
import org.chromium.build.annotations.NullMarked;
import org.chromium.chrome.browser.preferences.ChromeSharedPreferences;
import org.chromium.ui.base.DeviceFormFactor;

@NullMarked
public class BraveAdaptiveToolbarPrefs {
    public static @AdaptiveToolbarButtonVariant int getCustomizationSetting() {
        return ChromeSharedPreferences.getInstance()
                .readInt(
                        ADAPTIVE_TOOLBAR_CUSTOMIZATION_SETTINGS,
                        DeviceFormFactor.isNonMultiDisplayContextOnTablet(
                                        ContextUtils.getApplicationContext())
                                ? AdaptiveToolbarButtonVariant.SHARE
                                : AdaptiveToolbarButtonVariant.NEW_TAB);
    }

    /**
     * Returns whether the customization preference toggle is enabled. Returns false if no value has
     * been set. The value returned is orthogonal to whether the corresponding feature flag is
     * enabled.
     */
    public static boolean isCustomizationPreferenceEnabled() {
        return ChromeSharedPreferences.getInstance()
                .readBoolean(ADAPTIVE_TOOLBAR_CUSTOMIZATION_ENABLED, false);
    }
}
