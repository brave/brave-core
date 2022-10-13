/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.flags;

import java.util.HashMap;

/**
 * Class to override default cached values.
 */
public class BraveCachedFlag extends CachedFlag {
    // Set of flags to override.
    private static final HashMap<String, Boolean> sFlags;
    static {
        sFlags = new HashMap<String, Boolean>();
        sFlags.put(ChromeFeatureList.START_SURFACE_ANDROID, false);
    }

    private final boolean mDefaultValue;

    public BraveCachedFlag(String featureName, boolean defaultValue) {
        super(featureName, defaultValue);

        // Override value if necessary.
        if (sFlags.containsKey(featureName)) {
            mDefaultValue = sFlags.get(featureName);
        } else {
            mDefaultValue = defaultValue;
        }
    }

    @Override
    public boolean isEnabled() {
        return CachedFeatureFlags.isEnabled(mFeatureName, mDefaultValue);
    }
}
