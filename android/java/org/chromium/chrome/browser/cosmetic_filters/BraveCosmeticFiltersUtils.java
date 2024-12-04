/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.cosmetic_filters;

import org.jni_zero.CalledByNative;
import org.jni_zero.JNINamespace;
import org.jni_zero.NativeMethods;

import org.chromium.base.Log;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.tab.Tab;

@JNINamespace("cosmetic_filters")
public class BraveCosmeticFiltersUtils {
    private static final String TAG = "CosmeticFiltersUtils";

    public static boolean launchContentPickerForWebContent(Tab tab) {
        return BraveCosmeticFiltersUtilsJni.get().launchContentPickerForWebContent(tab);
    }

    @CalledByNative
    public static void showCustomFilterSettings() {
        try {
            BraveActivity.getBraveActivity().openBraveCreateCustomFiltersSettings();
        } catch (BraveActivity.BraveActivityNotFoundException e) {
            Log.e(TAG, "open create custom filter settings" + e);
        }
    }

    @CalledByNative
    public static int getThemeBackgroundColor() {
        int backgroundColor = 0;
        try {
            backgroundColor = BraveActivity.getBraveActivity().getBraveThemeBackgroundColor();
        } catch (BraveActivity.BraveActivityNotFoundException e) {
            Log.e(TAG, "Get theme background color" + e);
        }
        return backgroundColor;
    }

    @CalledByNative
    public static boolean isNightlyModeEnabled() {
        boolean isDarkModeEnabled = false;
        try {
            isDarkModeEnabled = BraveActivity.getBraveActivity().isNightlyModeEnabled();
        } catch (BraveActivity.BraveActivityNotFoundException e) {
            Log.e(TAG, "Get nightly mode status" + e);
        }
        return isDarkModeEnabled;
    }

    @NativeMethods
    interface Natives {
        boolean launchContentPickerForWebContent(Tab tab);
    }
}
