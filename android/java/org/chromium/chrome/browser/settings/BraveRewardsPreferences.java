/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.settings;

import android.content.SharedPreferences;
import android.os.Bundle;

import androidx.preference.Preference;
import androidx.preference.Preference.OnPreferenceChangeListener;

import org.chromium.base.ContextUtils;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.BraveRelaunchUtils;
import org.chromium.chrome.browser.BraveRewardsNativeWorker;
import org.chromium.chrome.browser.BraveRewardsObserver;
import org.chromium.chrome.browser.BraveRewardsPanelPopup;
import org.chromium.chrome.browser.preferences.BravePrefServiceBridge;
import org.chromium.chrome.browser.settings.BravePreferenceFragment;
import org.chromium.components.browser_ui.settings.ChromeSwitchPreference;
import org.chromium.components.browser_ui.settings.SettingsUtils;

/**
 * Fragment to keep track of all Brave Rewards related preferences.
 */
public class BraveRewardsPreferences extends BravePreferenceFragment
        implements OnPreferenceChangeListener, BraveRewardsObserver {
    public static final String PREF_ADS_SWITCH = "ads_switch";

    // flag, if exists: default state (off) for background Brave ads has been set
    public static final String PREF_ADS_SWITCH_DEFAULT_HAS_BEEN_SET = "ads_switch_default_set";

    private ChromeSwitchPreference mAdsSwitch;

    private BraveRewardsNativeWorker mBraveRewardsNativeWorker;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        getActivity().setTitle(R.string.brave_ui_brave_rewards);
        SettingsUtils.addPreferencesFromResource(this, R.xml.brave_rewards_preferences);
    }

    @Override
    public void onActivityCreated(Bundle savedInstanceState) {
        super.onActivityCreated(savedInstanceState);

        mAdsSwitch = (ChromeSwitchPreference) findPreference(PREF_ADS_SWITCH);
        boolean isAdsInBackgroundEnabled = getPrefAdsInBackgroundEnabled();
        mAdsSwitch.setChecked(isAdsInBackgroundEnabled);
        mAdsSwitch.setOnPreferenceChangeListener(new OnPreferenceChangeListener() {
            @Override
            public boolean onPreferenceChange(Preference preference, Object newValue) {
                setPrefAdsInBackgroundEnabled((boolean) newValue);
                return true;
            }
        });
    }

    @Override
    public void onStart() {
        mBraveRewardsNativeWorker = BraveRewardsNativeWorker.getInstance();
        if (mBraveRewardsNativeWorker != null) {
            mBraveRewardsNativeWorker.AddObserver(this);
        }
        super.onStart();
    }

    @Override
    public void onStop() {
        if (mBraveRewardsNativeWorker != null) {
            mBraveRewardsNativeWorker.RemoveObserver(this);
        }
        super.onStop();
    }

    /**
     * Returns the user preference for whether the brave ads in background is enabled.
     *
     */
    public static boolean getPrefAdsInBackgroundEnabled() {
        SharedPreferences sharedPreferences = ContextUtils.getAppSharedPreferences();
        return sharedPreferences.getBoolean(PREF_ADS_SWITCH, true);
    }

    /**
     * Sets the user preference for whether the brave ads in background is enabled.
     */
    public void setPrefAdsInBackgroundEnabled(boolean enabled) {
        SharedPreferences sharedPreferences = ContextUtils.getAppSharedPreferences();
        SharedPreferences.Editor sharedPreferencesEditor = sharedPreferences.edit();
        sharedPreferencesEditor.putBoolean(PREF_ADS_SWITCH, enabled);
        sharedPreferencesEditor.apply();
    }

    @Override
    public void onDisplayPreferenceDialog(Preference preference) {
        if (preference instanceof BraveRewardsResetPreference) {
            BraveRewardsResetPreferenceDialog dialogFragment =
                    BraveRewardsResetPreferenceDialog.newInstance(
                            (BraveRewardsResetPreference) preference);
            dialogFragment.setTargetFragment(this, 0);
            dialogFragment.show(getFragmentManager(), BraveRewardsResetPreferenceDialog.TAG);
        } else {
            super.onDisplayPreferenceDialog(preference);
        }
    }

    @Override
    public boolean onPreferenceChange(Preference preference, Object newValue) {
        return true;
    }

    @Override
    public void OnResetTheWholeState(boolean success) {
        if (success) {
            SharedPreferences sharedPreferences = ContextUtils.getAppSharedPreferences();
            SharedPreferences.Editor sharedPreferencesEditor = sharedPreferences.edit();
            sharedPreferencesEditor.putBoolean(
                    BraveRewardsPanelPopup.PREF_GRANTS_NOTIFICATION_RECEIVED, false);
            sharedPreferencesEditor.putBoolean(
                    BraveRewardsPanelPopup.PREF_WAS_BRAVE_REWARDS_TURNED_ON, false);
            sharedPreferencesEditor.apply();
            BravePrefServiceBridge.getInstance().setSafetynetCheckFailed(false);
            BraveRelaunchUtils.askForRelaunch(getActivity());
        } else {
            BraveRelaunchUtils.askForRelaunchCustom(getActivity());
        }
    }

    @Override
    public void onCreatePreferences(Bundle bundle, String s) {}
}
