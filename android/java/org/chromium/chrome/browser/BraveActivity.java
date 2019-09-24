/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser;

import android.content.Context;

import org.chromium.base.CommandLine;
import org.chromium.base.ContextUtils;
import org.chromium.base.Log;
import org.chromium.base.annotations.JNINamespace;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.ChromeSwitches;
import org.chromium.chrome.browser.preferences.Pref;
import org.chromium.chrome.browser.preferences.PrefServiceBridge;
/**
 * Brave's extension for ChromeActivity
 */
@JNINamespace("chrome::android")
public abstract class BraveActivity extends ChromeActivity {
    private static final String PREF_CLOSE_TABS_ON_EXIT = "close_tabs_on_exit";

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

    @Override
    public boolean onMenuOrKeyboardAction(int id, boolean fromMenu) {
        if (super.onMenuOrKeyboardAction(id, fromMenu)) {
            return true;
        }

        if (getActivityTab() == null) {
            return false;
        } else if (id == R.id.exit_id) {
            ApplicationLifetime.terminate(false);
        } else {
            return false;
        }

        return true;
    }

    @Override
    public void initializeState() {
        super.initializeState();
        if (isNoRestoreState()) {
            CommandLine.getInstance().appendSwitch(ChromeSwitches.NO_RESTORE_STATE);
        }
    }

    private boolean isNoRestoreState() {
        return ContextUtils.getAppSharedPreferences().getBoolean(PREF_CLOSE_TABS_ON_EXIT, false);
    }

    private native void nativeRestartStatsUpdater();
}
