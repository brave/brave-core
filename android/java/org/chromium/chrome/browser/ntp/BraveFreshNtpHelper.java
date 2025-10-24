/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.ntp;

import org.chromium.base.BraveFeatureList;
import org.chromium.build.annotations.NullMarked;
import org.chromium.chrome.browser.flags.ChromeFeatureList;

/**
 * Helper class for managing fresh NTP after idle expiration feature. This feature shows a refreshed
 * NTP when the app has been idle for a specified duration.
 */
@NullMarked
public class BraveFreshNtpHelper {
    private static final String PARAM_VARIANT = "variant";

    /**
     * @return Whether the fresh NTP after idle expiration feature is enabled.
     */
    public static boolean isEnabled() {
        return ChromeFeatureList.isEnabled(BraveFeatureList.BRAVE_FRESH_NTP_AFTER_IDLE_EXPIREMENT);
    }

    /**
     * Gets the variant of the fresh NTP experiment (e.g., "A", "B", "C"). Returns empty string if
     * no variant is set.
     *
     * @return The variant string.
     */
    public static String getVariant() {
        if (!isEnabled()) {
            return "";
        }

        return ChromeFeatureList.getFieldTrialParamByFeature(
                BraveFeatureList.BRAVE_FRESH_NTP_AFTER_IDLE_EXPIREMENT, PARAM_VARIANT);
    }
}
