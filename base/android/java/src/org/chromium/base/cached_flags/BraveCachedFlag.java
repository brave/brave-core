/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.base.cached_flags;

import org.chromium.base.FeatureMap;

import java.util.HashMap;

/** Class to override default cached values. */
public class BraveCachedFlag extends CachedFlag {
    private static final String INCOGNITO_REAUTHENTICATION_FOR_ANDROID =
            "IncognitoReauthenticationForAndroid";
    private static final String MAGIC_STACK_ANDROID = "MagicStackAndroid";
    private static final String RETAIN_OMNIBOX_ON_FOCUS = "RetainOmniboxOnFocus";
    private static final String SURFACE_POLISH = "SurfacePolish";

    // Set of flags to override.
    private static final HashMap<String, Boolean> sFlags;

    static {
        sFlags = new HashMap<String, Boolean>();
        sFlags.put(INCOGNITO_REAUTHENTICATION_FOR_ANDROID, true);
        sFlags.put(MAGIC_STACK_ANDROID, false);
        sFlags.put(RETAIN_OMNIBOX_ON_FOCUS, true);
        sFlags.put(SURFACE_POLISH, false);
    }

    // Will be deleted in bytecode. Variable from the parent class will be used instead.
    private boolean mDefaultValue;

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
