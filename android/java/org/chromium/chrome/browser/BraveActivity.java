/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser;

import android.content.Context;

import org.chromium.base.Log;
import org.chromium.base.annotations.JNINamespace;
import org.chromium.chrome.browser.preferences.Pref;
import org.chromium.chrome.browser.preferences.PrefServiceBridge;

/**
 * Brave's extension for ChromeActivity
 */
@JNINamespace("chrome::android")
public abstract class BraveActivity extends ChromeActivity {
    @Override
    public void onResumeWithNative() {
        super.onResumeWithNative();
        nativeRestartStatsUpdater();
    }

    @Override
    public void onStartWithNative() {
        super.onStartWithNative();

        // Disable NTP suggestions
        PrefServiceBridge.getInstance().setBoolean(Pref.NTP_ARTICLES_SECTION_ENABLED, false);
        PrefServiceBridge.getInstance().setBoolean(Pref.NTP_ARTICLES_LIST_VISIBLE, false);
    }

    private native void nativeRestartStatsUpdater();
}
