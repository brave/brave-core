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
public class BraveVpnPreferences extends BravePreferenceFragment
        implements OnPreferenceChangeListener, BraveRewardsObserver {
    public static final String PREF_VPN_SWITCH = "vpn_switch";

    private ChromeSwitchPreference mVpnSwitch;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        getActivity().setTitle(R.string.brave_vpn);
        SettingsUtils.addPreferencesFromResource(this, R.xml.brave_vpn_preferences);
    }

    @Override
    public void onActivityCreated(Bundle savedInstanceState) {
        super.onActivityCreated(savedInstanceState);

        mVpnSwitch = (ChromeSwitchPreference) findPreference(PREF_VPN_SWITCH);
        // boolean isAdsInBackgroundEnabled = getPrefAdsInBackgroundEnabled();
        // mAdsSwitch.setChecked(isAdsInBackgroundEnabled);
        // mAdsSwitch.setOnPreferenceChangeListener(new OnPreferenceChangeListener() {
        //     @Override
        //     public boolean onPreferenceChange(Preference preference, Object newValue) {
        //         setPrefAdsInBackgroundEnabled((boolean) newValue);
        //         return true;
        //     }
        // });
    }

    // @Override
    // public void onStart() {
    //     mBraveRewardsNativeWorker = BraveRewardsNativeWorker.getInstance();
    //     if (mBraveRewardsNativeWorker != null) {
    //         mBraveRewardsNativeWorker.AddObserver(this);
    //     }
    //     super.onStart();
    // }

    // @Override
    // public void onStop() {
    //     if (mBraveRewardsNativeWorker != null) {
    //         mBraveRewardsNativeWorker.RemoveObserver(this);
    //     }
    //     super.onStop();
    // }

    @Override
    public boolean onPreferenceChange(Preference preference, Object newValue) {
        return true;
    }

    @Override
    public void onCreatePreferences(Bundle bundle, String s) {}
}
