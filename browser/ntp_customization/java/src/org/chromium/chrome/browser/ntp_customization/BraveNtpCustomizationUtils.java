/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.ntp_customization;

import org.chromium.build.annotations.NullMarked;
import org.chromium.components.browser_ui.widget.displaystyle.UiConfig;
import org.chromium.ui.base.WindowAndroid;

/**
 * Brave-specific NTP customization utilities. Used via bytecode to replace methods in
 * NtpCustomizationUtils.
 */
@NullMarked
public class BraveNtpCustomizationUtils {
    private static final String TAG = "BraveNtpCustomizationUtils";

    /**
     * Replace NtpCustomizationUtils.isInNarrowWindowOnTablet. Brave returns true for any tablet to
     * avoid MVT layout misalignment.
     */
    public static boolean isInNarrowWindowOnTablet(boolean isTablet, UiConfig uiConfig) {
        return isTablet;
    }

    /**
     * Replace NtpCustomizationUtils.isNtpThemeCustomizationEnabled (no-arg). Brave uses its own NTP
     * background system and does not participate in Chromium's NTP theme customization.
     */
    public static boolean isNtpThemeCustomizationEnabled() {
        return false;
    }

    /**
     * Replace NtpCustomizationUtils.isNtpThemeCustomizationEnabled (WindowAndroid, boolean). Same
     * rationale as the no-arg overload above.
     */
    public static boolean isNtpThemeCustomizationEnabled(
            WindowAndroid windowAndroid, boolean isTablet) {
        return false;
    }
}
