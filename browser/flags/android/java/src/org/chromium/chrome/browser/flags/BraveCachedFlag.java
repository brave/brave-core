/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

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
        sFlags.put(ChromeFeatureList.INCOGNITO_REAUTHENTICATION_FOR_ANDROID, true);
    }

    // Will be deleted in bytecode. Variable from the parent class will be used instead.
    private boolean mDefaultValue;

    public BraveCachedFlag(String featureName, boolean defaultValue) {
        super(featureName, defaultValue);

        maybeOverrideDefaultValue(featureName, defaultValue);
    }

    private void maybeOverrideDefaultValue(String featureName, boolean defaultValue) {
        // Override value if necessary.
        if (sFlags.containsKey(featureName)) {
            mDefaultValue = sFlags.get(featureName);
        } else {
            mDefaultValue = defaultValue;
        }
    }
}
