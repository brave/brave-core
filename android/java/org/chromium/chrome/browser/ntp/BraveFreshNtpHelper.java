/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.ntp;

import org.chromium.build.annotations.NullMarked;
import org.chromium.chrome.browser.app.flags.BraveCachedFlags;

/**
 * Helper class for managing fresh NTP after idle expiration feature. This feature shows a refreshed
 * NTP when the app has been idle for a specified duration. Uses cached feature params to safely
 * access values before native is ready.
 */
@NullMarked
public class BraveFreshNtpHelper {
    /**
     * @return Whether the fresh NTP after idle expiration feature is enabled. This uses a cached
     *     flag, so it's safe to call even before native is ready.
     */
    public static boolean isEnabled() {
        return BraveCachedFlags.sBraveFreshNtpAfterIdleExperimentEnabled.isEnabled();
    }

    /**
     * Gets the variant of the fresh NTP experiment (e.g., "A", "B", "C"). Returns empty string if
     * no variant is set. This uses a cached param, so it's safe to call even before native is
     * ready.
     *
     * @return The variant string.
     */
    public static String getVariant() {
        return BraveCachedFlags.sBraveFreshNtpAfterIdleExperimentVariant.getValue();
    }
}
