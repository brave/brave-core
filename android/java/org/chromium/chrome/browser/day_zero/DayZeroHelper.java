/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.day_zero;

import org.jni_zero.CalledByNative;

import org.chromium.base.BravePreferenceKeys;
import org.chromium.chrome.browser.preferences.ChromeSharedPreferences;

public class DayZeroHelper {
    // Day Zero experiment variants, as written by native. Any other value means no variant is
    // active, which callers treat as DAY_ZERO_DEFAULT_VARIANT.
    public static final String DAY_ZERO_VARIANT_A = "A";
    public static final String DAY_ZERO_VARIANT_B = "B";
    public static final String DAY_ZERO_DEFAULT_VARIANT = "Default";

    // Returned when no variant has been stored yet, as opposed to DAY_ZERO_DEFAULT_VARIANT which
    // is an actual variant value. Change it for testing purposes to manually override Day Zero
    // study options.
    private static final String DEFAULT_DAY_ZERO_VALUE = "";

    @CalledByNative
    private static void setDayZeroVariant(String variant) {
        ChromeSharedPreferences.getInstance()
                .writeString(BravePreferenceKeys.DAY_ZERO_EXPT_VARIANT, variant);
    }

    public static String getDayZeroVariant() {
        return ChromeSharedPreferences.getInstance()
                .readString(BravePreferenceKeys.DAY_ZERO_EXPT_VARIANT, DEFAULT_DAY_ZERO_VALUE);
    }
}
