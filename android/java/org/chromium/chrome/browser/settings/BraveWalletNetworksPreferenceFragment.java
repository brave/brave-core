/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.settings;

import static org.chromium.chrome.browser.crypto_wallet.util.WalletConstants.ADD_NETWORK_FRAGMENT_ARG_ACTIVE_NETWORK;
import static org.chromium.chrome.browser.crypto_wallet.util.WalletConstants.ADD_NETWORK_FRAGMENT_ARG_CHAIN_ID;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;

import androidx.annotation.Nullable;

import org.chromium.chrome.R;
import org.chromium.components.browser_ui.settings.FragmentSettingsLauncher;
import org.chromium.components.browser_ui.settings.SettingsLauncher;
import org.chromium.components.browser_ui.settings.SettingsUtils;

public class BraveWalletNetworksPreferenceFragment extends BravePreferenceFragment
        implements FragmentSettingsLauncher, BraveWalletAddNetworksFragment.Launcher {
    private static final String PREF_BRAVE_WALLET_NETWORKS_ADD = "pref_brave_wallet_networks_add";
    private static final int REQUEST_CODE_ADD_NETWORK = 1;

    // SettingsLauncher injected from main Settings Activity.
    private SettingsLauncher mSettingsLauncher;
    private BraveWalletAddNetworksFragment.Refresher mRefresher;

    @Override
    public void onCreatePreferences(@Nullable Bundle savedInstanceState, String rootKey) {
        getActivity().setTitle(R.string.brave_wallet_networks_title);
        SettingsUtils.addPreferencesFromResource(this, R.xml.brave_wallet_networks_preference);

        BraveWalletNetworksPreference mNetworksAddPref =
                (BraveWalletNetworksPreference) findPreference(PREF_BRAVE_WALLET_NETWORKS_ADD);
        mNetworksAddPref.registerActivityLauncher(this);
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        BraveWalletNetworksPreference mNetworksAddPref =
                (BraveWalletNetworksPreference) findPreference(PREF_BRAVE_WALLET_NETWORKS_ADD);
        mNetworksAddPref.destroy();
    }

    @Override
    public void setSettingsLauncher(SettingsLauncher settingsLauncher) {
        mSettingsLauncher = settingsLauncher;
    }

    @Override
    public void launchAddNetwork(String chainId, boolean activeNetwork) {
        Bundle fragmentArgs = new Bundle();
        fragmentArgs.putString(ADD_NETWORK_FRAGMENT_ARG_CHAIN_ID, chainId);
        fragmentArgs.putBoolean(ADD_NETWORK_FRAGMENT_ARG_ACTIVE_NETWORK, activeNetwork);
        Intent intent = mSettingsLauncher.createSettingsActivityIntent(
                getActivity(), BraveWalletAddNetworksFragment.class.getName(), fragmentArgs);
        startActivityForResult(intent, REQUEST_CODE_ADD_NETWORK);
    }

    @Override
    public void setRefresher(BraveWalletAddNetworksFragment.Refresher refresher) {
        mRefresher = refresher;
    }

    @Override
    public void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        if (resultCode != Activity.RESULT_OK) return;
        if (mRefresher != null) {
            mRefresher.refreshNetworksList();
        }
    }
}
