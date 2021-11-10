/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.settings;

import static org.chromium.chrome.browser.settings.BraveWalletAutoLockPreferences.PREF_WALLET_AUTOLOCK_TIME;

import android.os.Bundle;
import android.os.Handler;
import android.text.TextUtils;
import android.view.View;

import androidx.preference.Preference;
import androidx.preference.Preference.OnPreferenceChangeListener;
import androidx.recyclerview.widget.RecyclerView;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.preferences.SharedPreferencesManager;
import org.chromium.components.browser_ui.settings.SettingsUtils;

public class BraveWalletPreferences extends BravePreferenceFragment {

    private static final String PREF_BRAVE_WALLET_AUTOLOCK = "pref_brave_wallet_autolock";
    private static final String PREF_BRAVE_WALLET_RESET = "pref_brave_wallet_reset";

    private BraveWalletAutoLockPreferences mPrefAutolock;
    private final SharedPreferencesManager mSharedPreferencesManager = SharedPreferencesManager.getInstance();
    private SharedPreferencesManager.Observer mPreferenceObserver;

    public BraveWalletPreferences() {
        super();

        mPreferenceObserver = key -> {
            if (TextUtils.equals(key, PREF_WALLET_AUTOLOCK_TIME)) updateAutolockSummary();
        };
    }

    @Override
    public void onActivityCreated(Bundle savedInstanceState) {
        super.onActivityCreated(savedInstanceState);

        mPrefAutolock = (BraveWalletAutoLockPreferences) findPreference(PREF_BRAVE_WALLET_AUTOLOCK);
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        mSharedPreferencesManager.addObserver(mPreferenceObserver);
    }

    @Override
    public void onResume() {
        super.onResume();
        updateAutolockSummary();
    }

    @Override
    public void onStop () {
        super.onStop();
        mSharedPreferencesManager.removeObserver(mPreferenceObserver);
    }

    @Override
    public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {
        getActivity().setTitle(R.string.brave_ui_brave_wallet);
        SettingsUtils.addPreferencesFromResource(this, R.xml.brave_wallet_preferences);
    }

    public void updateAutolockSummary() {
        int autoLockTime = BraveWalletAutoLockPreferences.getPrefWalletAutoLockTime();
        mPrefAutolock.setSummary(getContext().getResources().getQuantityString(R.plurals.time_long_mins, autoLockTime, autoLockTime));
        RecyclerView.ViewHolder viewHolder = (RecyclerView.ViewHolder) getListView().findViewHolderForAdapterPosition(mPrefAutolock.getOrder());
        if (viewHolder != null) {
            viewHolder.itemView.invalidate();
        }
    }
}
