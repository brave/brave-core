/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.day_zero;

import org.jni_zero.CalledByNative;

import org.chromium.base.BravePreferenceKeys;
import org.chromium.chrome.browser.preferences.ChromeSharedPreferences;

public class DayZeroHelper {
    @CalledByNative
    private static void setDayZeroVariant(String variant) {
        ChromeSharedPreferences.getInstance()
                .writeString(BravePreferenceKeys.DAY_ZERO_EXPT_VARIANT, variant);
    }

    public static String getDayZeroVariant() {
        return ChromeSharedPreferences.getInstance()
                .readString(BravePreferenceKeys.DAY_ZERO_EXPT_VARIANT, "");
    }
}
