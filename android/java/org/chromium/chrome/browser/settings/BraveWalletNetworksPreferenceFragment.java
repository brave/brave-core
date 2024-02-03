/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.settings;

import static org.chromium.chrome.browser.crypto_wallet.util.WalletConstants.ADD_NETWORK_FRAGMENT_ARG_ACTIVE_NETWORK;
import static org.chromium.chrome.browser.crypto_wallet.util.WalletConstants.ADD_NETWORK_FRAGMENT_ARG_CHAIN_ID;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;

import androidx.activity.result.ActivityResultLauncher;
import androidx.activity.result.contract.ActivityResultContracts;
import androidx.annotation.Nullable;

import org.chromium.chrome.R;
import org.chromium.components.browser_ui.settings.FragmentSettingsLauncher;
import org.chromium.components.browser_ui.settings.SettingsLauncher;
import org.chromium.components.browser_ui.settings.SettingsUtils;

public class BraveWalletNetworksPreferenceFragment extends BravePreferenceFragment
        implements FragmentSettingsLauncher, BraveWalletAddNetworksFragment.Launcher {
    private static final String PREF_BRAVE_WALLET_NETWORKS_ADD = "pref_brave_wallet_networks_add";

    // SettingsLauncher injected from main Settings Activity.
    private SettingsLauncher mSettingsLauncher;
    private BraveWalletAddNetworksFragment.Refresher mRefresher;
    private ActivityResultLauncher<Intent> mAddNetworkActivityResultLauncher;

    @Override
    public void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        // Pass @{code ActivityResultRegistry} reference explicitly to avoid crash
        // https://github.com/brave/brave-browser/issues/31882
        mAddNetworkActivityResultLauncher =
                registerForActivityResult(
                        new ActivityResultContracts.StartActivityForResult(),
                        requireActivity().getActivityResultRegistry(),
                        result -> {
                            if (result.getResultCode() != Activity.RESULT_OK) return;
                            if (mRefresher != null) {
                                mRefresher.refreshNetworksList();
                            }
                        });
    }

    @Override
    public void onCreatePreferences(@Nullable Bundle savedInstanceState, String rootKey) {
        requireActivity().setTitle(R.string.brave_wallet_networks_title);
        SettingsUtils.addPreferencesFromResource(this, R.xml.brave_wallet_networks_preference);

        BraveWalletNetworksPreference mNetworksAddPref =
                findPreference(PREF_BRAVE_WALLET_NETWORKS_ADD);
        if (mNetworksAddPref != null) {
            mNetworksAddPref.registerActivityLauncher(this);
        }
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        BraveWalletNetworksPreference mNetworksAddPref =
                findPreference(PREF_BRAVE_WALLET_NETWORKS_ADD);
        if (mNetworksAddPref != null) {
            mNetworksAddPref.destroy();
        }
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
        mAddNetworkActivityResultLauncher.launch(intent);
    }

    @Override
    public void setRefresher(BraveWalletAddNetworksFragment.Refresher refresher) {
        mRefresher = refresher;
    }
}
