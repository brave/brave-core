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
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import androidx.activity.result.ActivityResultLauncher;
import androidx.activity.result.contract.ActivityResultContracts;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

import org.chromium.chrome.R;
import org.chromium.components.browser_ui.settings.FragmentSettingsLauncher;
import org.chromium.components.browser_ui.settings.SettingsLauncher;
import org.chromium.components.browser_ui.settings.SettingsUtils;

public class BraveWalletNetworksPreferenceFragment extends BravePreferenceFragment
        implements FragmentSettingsLauncher, BraveWalletAddNetworksFragment.Listener {
    // Preference key from R.xml.brave_wallet_networks_preference.
    private static final String PREF_BRAVE_WALLET_NETWORKS_ADD = "pref_brave_wallet_networks_add";

    // SettingsLauncher injected from main Settings Activity.
    private SettingsLauncher mSettingsLauncher;
    private ActivityResultLauncher<Intent> mAddNetworkActivityResultLauncher;

    @Override
    public void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        requireActivity().setTitle(R.string.brave_wallet_networks_title);

        // Pass {@code ActivityResultRegistry} reference explicitly to avoid crash
        // https://github.com/brave/brave-browser/issues/31882
        mAddNetworkActivityResultLauncher =
                registerForActivityResult(
                        new ActivityResultContracts.StartActivityForResult(),
                        requireActivity().getActivityResultRegistry(),
                        result -> {
                            if (result.getResultCode() == Activity.RESULT_OK) {
                                BraveWalletNetworksPreference addNetworkPreference =
                                        findPreference(PREF_BRAVE_WALLET_NETWORKS_ADD);
                                if (addNetworkPreference != null) {
                                    addNetworkPreference.updateNetworks();
                                }
                            }
                        });
    }

    @Override
    public void onCreatePreferences(@Nullable Bundle savedInstanceState, String rootKey) {
        SettingsUtils.addPreferencesFromResource(this, R.xml.brave_wallet_networks_preference);

        BraveWalletNetworksPreference braveWalletNetworksPreference =
                findPreference(PREF_BRAVE_WALLET_NETWORKS_ADD);
        if (braveWalletNetworksPreference != null) {
            braveWalletNetworksPreference.setListener(this);
        }
    }

    @NonNull
    @Override
    public RecyclerView onCreateRecyclerView(
            @NonNull LayoutInflater inflater,
            @NonNull ViewGroup parent,
            @Nullable Bundle savedInstanceState) {
        final RecyclerView recyclerView =
                super.onCreateRecyclerView(inflater, parent, savedInstanceState);
        recyclerView.setOverScrollMode(View.OVER_SCROLL_NEVER);
        return recyclerView;
    }

    @NonNull
    @Override
    public RecyclerView.LayoutManager onCreateLayoutManager() {
        final LinearLayoutManager linearLayoutManager =
                new LinearLayoutManager(requireContext()) {
                    @Override
                    public boolean canScrollVertically() {
                        return false;
                    }
                };

        return linearLayoutManager;
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        BraveWalletNetworksPreference braveWalletNetworksPreference =
                findPreference(PREF_BRAVE_WALLET_NETWORKS_ADD);
        if (braveWalletNetworksPreference != null) {
            // It's important to close the JSON RPC service inside
            // the preference screen otherwise it may generate memory leaks.
            braveWalletNetworksPreference.destroy();
        }
    }

    @Override
    public void setSettingsLauncher(SettingsLauncher settingsLauncher) {
        mSettingsLauncher = settingsLauncher;
    }

    @Override
    public void addNewNetwork() {
        launchIntent(null, false);
    }

    @Override
    public void modifyNetwork(@NonNull final String chainId, boolean activeNetwork) {
        launchIntent(chainId, activeNetwork);
    }

    private void launchIntent(@Nullable final String chainId, boolean activeNetwork) {
        final Bundle fragmentArgs;
        if (chainId != null) {
            fragmentArgs = new Bundle();
            fragmentArgs.putString(ADD_NETWORK_FRAGMENT_ARG_CHAIN_ID, chainId);
            fragmentArgs.putBoolean(ADD_NETWORK_FRAGMENT_ARG_ACTIVE_NETWORK, activeNetwork);
        } else {
            fragmentArgs = null;
        }
        Intent intent =
                mSettingsLauncher.createSettingsActivityIntent(
                        requireContext(),
                        BraveWalletAddNetworksFragment.class,
                        fragmentArgs);
        mAddNetworkActivityResultLauncher.launch(intent);
    }
}
