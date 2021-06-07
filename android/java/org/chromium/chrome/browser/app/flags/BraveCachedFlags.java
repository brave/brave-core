/**
 * Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.app.flags;

import org.chromium.chrome.browser.preferences.BravePreferenceKeys;
import org.chromium.chrome.browser.preferences.BravePrefServiceBridge;
import org.chromium.chrome.browser.preferences.SharedPreferencesManager;
import org.chromium.chrome.browser.util.PackageUtils;


/**
 * Caches the flags that Brave might require before native is loaded in a later next run.
 */
public class BraveCachedFlags {
    private boolean mIsFinishedCachingNativeFlags;

    private static final BraveCachedFlags INSTANCE = new BraveCachedFlags();

    /**
     * @return The singleton.
     */
    public static BraveCachedFlags getInstance() {
        return INSTANCE;
    }

    /**
     * Caches flags that are needed by Activities that launch before the native library is loaded
     * and stores them in SharedPreferences. See
     * chrome/android/java/src/org/chromium/chrome/browser/app/flags/ChromeCachedFlags.java.
     * This will cache java SharedPreference to native.
     */
    public void cacheP3AFlags() {
        if (mIsFinishedCachingNativeFlags) return;

        // Cache prefs for P3A
        SharedPreferencesManager javaPrefs = SharedPreferencesManager.getInstance();
        boolean javaPrefValue =
                javaPrefs.readBoolean(BravePreferenceKeys.BRAVE_P3A_ENABLED, false);
        boolean nativePrefValue = BravePrefServiceBridge.getInstance().getP3AEnabled();
        if (javaPrefValue != nativePrefValue) {
            BravePrefServiceBridge.getInstance().setP3AEnabled(javaPrefValue);
        }

        mIsFinishedCachingNativeFlags = true;
    }
}
