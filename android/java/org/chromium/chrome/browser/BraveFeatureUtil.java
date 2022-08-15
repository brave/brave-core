/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser;

import org.chromium.base.annotations.JNINamespace;
import org.chromium.base.annotations.NativeMethods;

@JNINamespace("chrome::android")
public abstract class BraveFeatureUtil {
    public static void enableFeature(
            String featureName, boolean enabled, boolean fallbackToDefault) {
        BraveFeatureUtilJni.get().enableFeature(featureName, enabled, fallbackToDefault);
    }

    @NativeMethods
    interface Natives {
        void enableFeature(String featureName, boolean enabled, boolean fallbackToDefault);
    }
}
