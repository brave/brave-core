/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.util;

import android.app.Activity;

import com.google.android.material.color.DynamicColors;
import com.google.android.material.color.DynamicColorsOptions;

import org.chromium.base.BraveFeatureList;
import org.chromium.chrome.browser.flags.ChromeFeatureMap;
import org.chromium.components.cached_flags.CachedFlag;

public class BraveDynamicColors {
    // Declared here (not in BraveCachedFlags) to avoid circular class-initialization:
    // BraveCachedFlags extends ChromeCachedFlags whose <clinit> creates a BraveCachedFlags
    // instance before BraveCachedFlags static fields are ready. Accessing this field early
    // only triggers BraveDynamicColors class-init, which has no such circular dependency.
    public static final CachedFlag sDynamicColorsEnabled =
            new CachedFlag(
                    ChromeFeatureMap.getInstance(),
                    BraveFeatureList.BRAVE_ANDROID_DYNAMIC_COLORS,
                    false);

    public static void applyToActivityIfAvailable(Activity activity) {
        if (!sDynamicColorsEnabled.isEnabled()) {
            // We disable dynamic colors as it causes styling issues with Brave theme.
            return;
        }

        DynamicColors.applyToActivityIfAvailable(activity);
    }

    public static void applyToActivityIfAvailable(
            Activity activity, DynamicColorsOptions dynamicColorsOptions) {
        if (!sDynamicColorsEnabled.isEnabled()) {
            // We disable dynamic colors as it causes styling issues with Brave theme.
            return;
        }

        DynamicColors.applyToActivityIfAvailable(activity, dynamicColorsOptions);
    }
}
