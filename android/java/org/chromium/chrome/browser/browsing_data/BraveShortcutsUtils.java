/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.browsing_data;

import org.jni_zero.JNINamespace;
import org.jni_zero.NativeMethods;

import org.chromium.build.annotations.NullMarked;
import org.chromium.chrome.browser.profiles.Profile;

/** Utility to ensure ShortcutsBackend is initialized before clearing browsing data. */
@JNINamespace("brave")
@NullMarked
public class BraveShortcutsUtils {
    /**
     * Force-initializes ShortcutsBackend for the given profile, then invokes {@code callback}.
     *
     * <p>If the backend is already initialized the callback runs synchronously. Otherwise it is
     * deferred until the backend's async DB init completes (OnShortcutsLoaded), so that a
     * subsequent clearBrowsingData call will see an initialized backend and correctly handle
     * OnHistoryDeletions.
     */
    public static void initThenRun(Profile profile, Runnable callback) {
        BraveShortcutsUtilsJni.get().initThenRun(profile, callback);
    }

    @NativeMethods
    interface Natives {
        void initThenRun(Profile profile, Runnable callback);
    }
}
