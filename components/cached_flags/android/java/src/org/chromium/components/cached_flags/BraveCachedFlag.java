/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.components.cached_flags;

import org.chromium.base.FeatureMap;

import java.util.HashMap;

/** Class to override default cached values. */
public class BraveCachedFlag extends CachedFlag {
    private static final String ANDROID_OPEN_INCOGNITO_AS_WINDOW = "AndroidOpenIncognitoAsWindow";

    // Set of flags to override. An entry only takes effect while upstream still declares a
    // CachedFlag with the same name, because maybeOverrideDefaultValue() is reached from the
    // CachedFlag constructor.
    private static final HashMap<String, Boolean> sFlags;

    static {
        sFlags = new HashMap<String, Boolean>();
        sFlags.put(ANDROID_OPEN_INCOGNITO_AS_WINDOW, false);
    }

    public BraveCachedFlag(
            FeatureMap featureMap,
            String featureName,
            boolean defaultValue,
            boolean defaultValueInTests) {
        super(featureMap, featureName, defaultValue, defaultValueInTests);

        maybeOverrideDefaultValue(featureName, defaultValue);
    }

    public BraveCachedFlag(FeatureMap featureMap, String featureName, boolean defaultValue) {
        super(featureMap, featureName, defaultValue);

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
