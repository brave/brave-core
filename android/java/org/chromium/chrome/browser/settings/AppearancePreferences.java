/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.settings;

import static org.chromium.chrome.browser.settings.MainSettings.PREF_UI_THEME;

import android.content.SharedPreferences;
import android.os.Bundle;
import androidx.preference.Preference;

import org.chromium.base.ContextUtils;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.BraveFeatureList;
import org.chromium.chrome.browser.BraveRelaunchUtils;
import org.chromium.chrome.browser.BraveRewardsNativeWorker;
import org.chromium.chrome.browser.BraveRewardsObserver;
import org.chromium.chrome.browser.flags.ChromeFeatureList;
import org.chromium.chrome.browser.night_mode.NightModeUtils;
import org.chromium.chrome.browser.preferences.BravePreferenceKeys;
import org.chromium.chrome.browser.toolbar.bottom.BottomToolbarConfiguration;
import org.chromium.components.browser_ui.settings.ChromeSwitchPreference;
import org.chromium.components.browser_ui.settings.SettingsUtils;
import org.chromium.chrome.browser.settings.BravePreferenceFragment;
import org.chromium.ui.base.DeviceFormFactor;

public class AppearancePreferences extends BravePreferenceFragment
        implements Preference.OnPreferenceChangeListener, BraveRewardsObserver {
    public static final String PREF_HIDE_BRAVE_REWARDS_ICON = "hide_brave_rewards_icon";
    private BraveRewardsNativeWorker mBraveRewardsNativeWorker;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        getActivity().setTitle(R.string.prefs_appearance);
        SettingsUtils.addPreferencesFromResource(this, R.xml.appearance_preferences);
        boolean isTablet = DeviceFormFactor.isNonMultiDisplayContextOnTablet(
                ContextUtils.getApplicationContext());
        if (isTablet) {
            removePreferenceIfPresent(BravePreferenceKeys.BRAVE_BOTTOM_TOOLBAR_ENABLED_KEY);
        }

        if (!NightModeUtils.isNightModeSupported()) {
            removePreferenceIfPresent(PREF_UI_THEME);
        }

        if (!ChromeFeatureList.isEnabled(BraveFeatureList.BRAVE_REWARDS)) {
            removePreferenceIfPresent(PREF_HIDE_BRAVE_REWARDS_ICON);
        }
    }

    @Override
    public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {}

    private void removePreferenceIfPresent(String key) {
        Preference preference = getPreferenceScreen().findPreference(key);
        if (preference != null) getPreferenceScreen().removePreference(preference);
    }

    @Override
    public void onActivityCreated(Bundle savedInstanceState) {
        super.onActivityCreated(savedInstanceState);

        Preference hideBraveRewardsIconPref = findPreference(PREF_HIDE_BRAVE_REWARDS_ICON);
        if (hideBraveRewardsIconPref != null) {
            hideBraveRewardsIconPref.setEnabled(false);
            hideBraveRewardsIconPref.setOnPreferenceChangeListener(this);
        }

        Preference enableBottomToolbar =
                findPreference(BravePreferenceKeys.BRAVE_BOTTOM_TOOLBAR_ENABLED_KEY);
        if (enableBottomToolbar == null) return;

        enableBottomToolbar.setOnPreferenceChangeListener(this);
        if (enableBottomToolbar instanceof ChromeSwitchPreference) {
            boolean isTablet = DeviceFormFactor.isNonMultiDisplayContextOnTablet(
                    ContextUtils.getApplicationContext());
            ((ChromeSwitchPreference) enableBottomToolbar)
                    .setChecked(!isTablet
                            && BottomToolbarConfiguration.isBottomToolbarEnabled());
        }
    }

    @Override
    public void onStart() {
        mBraveRewardsNativeWorker = BraveRewardsNativeWorker.getInstance();
        if (mBraveRewardsNativeWorker != null) {
            mBraveRewardsNativeWorker.AddObserver(this);
        }
        mBraveRewardsNativeWorker.GetRewardsMainEnabled();
        super.onStart();
    }

    @Override
    public void onStop() {
        if (mBraveRewardsNativeWorker != null) {
            mBraveRewardsNativeWorker.RemoveObserver(this);
        }
        super.onStop();
    }

    @Override
    public boolean onPreferenceChange(Preference preference, Object newValue) {
        String key = preference.getKey();
        if (BravePreferenceKeys.BRAVE_BOTTOM_TOOLBAR_ENABLED_KEY.equals(key)) {
            SharedPreferences prefs = ContextUtils.getAppSharedPreferences();
            Boolean originalStatus = BottomToolbarConfiguration.isBottomToolbarEnabled();
            prefs.edit()
                    .putBoolean(BravePreferenceKeys.BRAVE_BOTTOM_TOOLBAR_ENABLED_KEY,
                            !originalStatus)
                    .apply();
            BraveRelaunchUtils.askForRelaunch(getActivity());
        } else if (PREF_HIDE_BRAVE_REWARDS_ICON.equals(key)) {
            SharedPreferences sharedPreferences = ContextUtils.getAppSharedPreferences();
            SharedPreferences.Editor sharedPreferencesEditor = sharedPreferences.edit();
            sharedPreferencesEditor.putBoolean(PREF_HIDE_BRAVE_REWARDS_ICON, (boolean) newValue);
            sharedPreferencesEditor.apply();
            BraveRelaunchUtils.askForRelaunch(getActivity());
        }
        return true;
    }

    @Override
    public void OnWalletInitialized(int error_code) {}

    @Override
    public void OnPublisherInfo(int tabId) {}

    @Override
    public void OnGetCurrentBalanceReport(double[] report) {}

    @Override
    public void OnNotificationAdded(String id, int type, long timestamp, String[] args) {}

    @Override
    public void OnNotificationsCount(int count) {}

    @Override
    public void OnGetLatestNotification(String id, int type, long timestamp, String[] args) {}

    @Override
    public void OnNotificationDeleted(String id) {}

    @Override
    public void OnIsWalletCreated(boolean created) {}

    @Override
    public void OnGetPendingContributionsTotal(double amount) {}

    @Override
    public void OnGetRewardsMainEnabled(boolean enabled) {
        ChromeSwitchPreference hideBraveRewardsIconPref =
                (ChromeSwitchPreference) findPreference(PREF_HIDE_BRAVE_REWARDS_ICON);
        if (hideBraveRewardsIconPref == null) return;

        hideBraveRewardsIconPref.setEnabled(!enabled);
        if (enabled) {
            hideBraveRewardsIconPref.setChecked(false);
        }
    }

    @Override
    public void OnGetAutoContributeProperties() {}

    @Override
    public void OnGetReconcileStamp(long timestamp) {}

    @Override
    public void OnRecurringDonationUpdated() {}

    @Override
    public void OnResetTheWholeState(boolean success) {}

    @Override
    public void OnRewardsMainEnabled(boolean enabled) {}
}
