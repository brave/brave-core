/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser;

import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;
import android.net.Uri;
import android.os.Build;
import android.provider.Settings;
import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

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
import org.chromium.ui.widget.Toast;

/**
 * Brave's extension for ChromeActivity
 */
@JNINamespace("chrome::android")
public abstract class BraveActivity extends ChromeActivity {
    private static final String PREF_CLOSE_TABS_ON_EXIT = "close_tabs_on_exit";

    /**
     * Settings for sending local notification reminders.
     */
    public static final String BRAVE_PRODUCTION_PACKAGE_NAME = "com.brave.browser";
    public static final String BRAVE_DEVELOPMENT_PACKAGE_NAME = "com.brave.browser_default";

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
        } else if (id == R.id.set_default_browser) {
            handleBraveSetDefaultBrowserDialog();
        } else if (id == R.id.brave_rewards_id) {
            // Implement handler.
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

    private boolean isNoRestoreState() {
        return ContextUtils.getAppSharedPreferences().getBoolean(PREF_CLOSE_TABS_ON_EXIT, false);
    }

    public boolean isBraveSetAsDefaultBrowser() {
        Intent browserIntent = new Intent("android.intent.action.VIEW", Uri.parse("http://"));
        boolean supportsDefault = Build.VERSION.SDK_INT >= 24;
        ResolveInfo resolveInfo = getPackageManager().resolveActivity(
                browserIntent, supportsDefault ? PackageManager.MATCH_DEFAULT_ONLY : 0);
        return resolveInfo.activityInfo.packageName.equals(BRAVE_PRODUCTION_PACKAGE_NAME)
                || resolveInfo.activityInfo.packageName.equals(BRAVE_DEVELOPMENT_PACKAGE_NAME);
    }

    private void handleBraveSetDefaultBrowserDialog() {
        /* (Albert Wang): Default app settings didn't get added until API 24
         * https://developer.android.com/reference/android/provider/Settings#ACTION_MANAGE_DEFAULT_APPS_SETTINGS
         */
        Intent browserIntent = new Intent("android.intent.action.VIEW", Uri.parse("http://"));
        boolean supportsDefault = Build.VERSION.SDK_INT >= 24;
        ResolveInfo resolveInfo = getPackageManager().resolveActivity(
                browserIntent, supportsDefault ? PackageManager.MATCH_DEFAULT_ONLY : 0);
        Context context = ContextUtils.getApplicationContext();
        if (isBraveSetAsDefaultBrowser()) {
            Toast toast = Toast.makeText(
                    context, R.string.brave_already_set_as_default_browser, Toast.LENGTH_LONG);
            toast.show();
            return;
        }
        if (supportsDefault) {
            if (resolveInfo.activityInfo.packageName.equals("com.google.android.setupwizard")
                    || resolveInfo.activityInfo.packageName.equals("android")) {
                LayoutInflater inflater = getLayoutInflater();
                View layout = inflater.inflate(R.layout.brave_set_default_browser_dialog,
                        (ViewGroup) findViewById(R.id.brave_set_default_browser_toast_container));

                Toast toast = new Toast(context);
                toast.setDuration(Toast.LENGTH_LONG);
                toast.setView(layout);
                toast.setGravity(Gravity.TOP, 0, 40);
                toast.show();
                Intent intent =
                        new Intent(Intent.ACTION_VIEW, Uri.parse("http://www.brave.com/blog"));
                intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
                context.startActivity(intent);
            } else {
                Intent intent = new Intent(Settings.ACTION_MANAGE_DEFAULT_APPS_SETTINGS);
                intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
                context.startActivity(intent);
            }
        } else {
            if (resolveInfo.activityInfo.packageName.equals("com.google.android.setupwizard")
                    || resolveInfo.activityInfo.packageName.equals("android")) {
                // (Albert Wang): From what I've experimented on 6.0,
                // default browser popup is in the middle of the screen for
                // these versions. So we shouldn't show the toast.
                Intent intent =
                        new Intent(Intent.ACTION_VIEW, Uri.parse("http://www.brave.com/blog"));
                intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
                context.startActivity(intent);
            } else {
                Toast toast = Toast.makeText(
                        context, R.string.brave_default_browser_go_to_settings, Toast.LENGTH_LONG);
                toast.show();
                return;
            }
        }
    }

    private native void nativeRestartStatsUpdater();
}
