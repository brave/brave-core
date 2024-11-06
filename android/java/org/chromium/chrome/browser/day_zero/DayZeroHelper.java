/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.day_zero;

import org.jni_zero.CalledByNative;

import org.chromium.base.BravePreferenceKeys;
import org.chromium.chrome.browser.preferences.ChromeSharedPreferences;

public class DayZeroHelper {
    private static final String TAG = "DayZeroHelper";

    @CalledByNative
    private static void setDayZeroExptAndroid(boolean shouldShowFeatures) {
        ChromeSharedPreferences.getInstance()
                .writeBoolean(BravePreferenceKeys.DAY_ZERO_EXPT_FLAG, shouldShowFeatures);
    }

    public static boolean getDayZeroExptFlag() {
        return ChromeSharedPreferences.getInstance()
                .readBoolean(BravePreferenceKeys.DAY_ZERO_EXPT_FLAG, true);
    }
}
