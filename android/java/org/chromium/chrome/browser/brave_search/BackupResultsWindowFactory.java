/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.brave_search;

import org.jni_zero.CalledByNative;

import org.chromium.base.ContextUtils;
import org.chromium.build.annotations.NullMarked;
import org.chromium.ui.base.WindowAndroid;

/** Creates and destroys {@link WindowAndroid} instances for backup results WebContents. */
@NullMarked
public class BackupResultsWindowFactory {
    @CalledByNative
    public static WindowAndroid create() {
        return new WindowAndroid(
                ContextUtils.getApplicationContext(), /* occlusionTrackingAllowed= */ false);
    }

    @CalledByNative
    public static void destroy(WindowAndroid window) {
        window.destroy();
    }
}
