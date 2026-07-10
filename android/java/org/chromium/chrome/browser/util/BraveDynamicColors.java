/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.util;

import android.app.Activity;
import android.os.Build;

import com.google.android.material.color.DynamicColors;
import com.google.android.material.color.DynamicColorsOptions;

import org.chromium.base.BraveFeatureList;
import org.chromium.base.BravePreferenceKeys;
import org.chromium.chrome.browser.flags.ChromeFeatureMap;
import org.chromium.chrome.browser.preferences.ChromeSharedPreferences;
import org.chromium.components.cached_flags.CachedFlag;

public final class BraveDynamicColors {
    private static final class LazyHolder {
        // BraveCachedFlags extends ChromeCachedFlags. ChromeCachedFlags.<clinit> creates a
        // singleton whose constructor is bytecode-redirected to BraveCachedFlags. This can run
        // before BraveCachedFlags static fields are ready, so this CachedFlag must not live
        // directly in BraveCachedFlags. It is obtained from this separately initialized holder.
        private static final CachedFlag sDynamicColorsFlag =
                new CachedFlag(
                        ChromeFeatureMap.getInstance(),
                        BraveFeatureList.BRAVE_ANDROID_DYNAMIC_COLORS,
                        true);
    }

    private BraveDynamicColors() {}

    /**
     * Returns the lazily initialized feature flag used for cached-flag registration and
     * early-startup reads.
     */
    public static CachedFlag getCachedFlag() {
        return LazyHolder.sDynamicColorsFlag;
    }

    /**
     * Returns whether dynamic colors are available for this app session.
     *
     * <p>Availability requires the cached feature flag to be enabled and Android 12 or later. It
     * does not include the user's preference; {@link #isDynamicColorsEnabled()} is the preferred
     * method for runtime behavior checks.
     */
    public static boolean isDynamicColorsAvailable() {
        return getCachedFlag().isEnabled() && Build.VERSION.SDK_INT >= Build.VERSION_CODES.S;
    }

    /**
     * Returns whether dynamic colors should be used at runtime.
     *
     * <p>This requires dynamic colors to be available and the user preference to be enabled. The
     * preference defaults to enabled when it has not been set.
     */
    public static boolean isDynamicColorsEnabled() {
        return isDynamicColorsAvailable() && isDynamicColorsUserEnabled();
    }

    /** Returns the persisted user preference, which defaults to enabled when unset. */
    private static boolean isDynamicColorsUserEnabled() {
        return ChromeSharedPreferences.getInstance()
                .readBoolean(BravePreferenceKeys.BRAVE_ANDROID_DYNAMIC_COLORS_ENABLED, true);
    }

    public static void applyToActivityIfAvailable(Activity activity) {
        if (!isDynamicColorsEnabled()) {
            return;
        }

        DynamicColors.applyToActivityIfAvailable(activity);
    }

    public static void applyToActivityIfAvailable(
            Activity activity, DynamicColorsOptions dynamicColorsOptions) {
        if (!isDynamicColorsEnabled()) {
            return;
        }

        DynamicColors.applyToActivityIfAvailable(activity, dynamicColorsOptions);
    }
}
