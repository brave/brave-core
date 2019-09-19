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
import org.chromium.chrome.browser.preferences.BraveSearchEngineUtils;
import org.chromium.chrome.browser.preferences.Pref;
import org.chromium.chrome.browser.preferences.PrefServiceBridge;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.tab.DesktopModeTabHelper;

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
            if (id == R.id.request_desktop_site_id || id == R.id.request_desktop_site_check_id) {
                // If user selects desktop site opiton explicitly vi app menu, respect it.
                // Current tab will ignore global desktop mode.
                ignoreDesktopModeForTab(getActivityTab());
            }

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

        BraveSearchEngineUtils.initializeBraveSearchEngineStates(getTabModelSelector());
    }

    @Override
    public void onResume() {
        super.onResume();

        Tab tab = getActivityTab();
        if (tab == null)
            return;

        // Set proper active DSE whenever brave returns to foreground.
        // If active tab is private, set private DSE as an active DSE.
        BraveSearchEngineUtils.updateActiveDSE(tab.isIncognito());
    }

    @Override
    public void onPause() {
        super.onPause();

        Tab tab = getActivityTab();
        if (tab == null)
            return;

        // Set normal DSE as an active DSE when brave goes in background
        // because currently set DSE is used by outside of brave(ex, brave search widget).
        if (tab.isIncognito()) {
            BraveSearchEngineUtils.updateActiveDSE(false);
        }
    }

    private void ignoreDesktopModeForTab(Tab tab) {
        assert tab != null;
        DesktopModeTabHelper helper = DesktopModeTabHelper.get(tab);
        if (helper != null)
            helper.ignoreDesktopMode();
    }

    private boolean isNoRestoreState() {
        return ContextUtils.getAppSharedPreferences().getBoolean(PREF_CLOSE_TABS_ON_EXIT, false);
    }

    private native void nativeRestartStatsUpdater();
}
