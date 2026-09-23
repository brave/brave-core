/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser;

import org.jni_zero.JNINamespace;
import org.jni_zero.NativeMethods;

import org.chromium.build.annotations.NullMarked;

@JNINamespace("chrome::android")
@NullMarked
public abstract class BraveFeatureUtil {
    // The method sets a feature state that is passed in `enabled` var. However it can
    // set the feature to a Default state if Default state is what is `enabled` var.
    // For example: A feature `dummy_name` has 3 states and a Default value is Disabled:
    // 0: Default (Disabled)
    // 1: Enabled
    // 2: Disabled
    // enableFeature("dummy_name", false, true) sets the `dummy_name` feature state to Default
    // enableFeature("dummy_name", false, false) sets the `dummy_name` feature state to Disabled
    public static void enableFeature(
            String featureName, boolean enabled, boolean fallbackToDefault) {
        BraveFeatureUtilJni.get().enableFeature(featureName, enabled, fallbackToDefault);
    }

    @NativeMethods
    interface Natives {
        void enableFeature(String featureName, boolean enabled, boolean fallbackToDefault);
    }
}
