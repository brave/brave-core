/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.cosmetic_filters;

import org.jni_zero.JNINamespace;
import org.jni_zero.NativeMethods;

import org.chromium.chrome.browser.tab.Tab;
import org.chromium.content_public.browser.WebContents;
import org.jni_zero.CalledByNative;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.settings.BraveLeoPreferences;
import org.chromium.chrome.browser.settings.SettingsNavigationFactory;
import org.chromium.components.browser_ui.settings.SettingsNavigation;

import android.app.Activity;
import android.content.Context;

import org.chromium.base.Log;

@JNINamespace("cosmetic_filters")
public class BraveCosmeticFiltersUtils {
    private static final String TAG = "BraveCosmeticFiltersUtils";

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

    @NativeMethods
    interface Natives {
        boolean launchContentPickerForWebContent(Tab tab);
    }
}
