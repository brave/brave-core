/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.activities;

import static org.chromium.chrome.browser.crypto_wallet.util.WalletConstants.ADD_NETWORK_FRAGMENT_ARG_ACTIVE_NETWORK;
import static org.chromium.chrome.browser.crypto_wallet.util.WalletConstants.ADD_NETWORK_FRAGMENT_ARG_CHAIN_ID;

import android.content.Intent;
import android.os.Bundle;
import android.widget.Toast;

import androidx.recyclerview.widget.RecyclerView;

import com.google.android.material.appbar.MaterialToolbar;

import org.chromium.base.task.PostTask;
import org.chromium.brave_wallet.mojom.NetworkInfo;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.app.domain.NetworkSelectorModel;
import org.chromium.chrome.browser.app.domain.WalletModel;
import org.chromium.chrome.browser.crypto_wallet.adapters.NetworkSelectorAdapter;
import org.chromium.chrome.browser.crypto_wallet.util.JavaUtils;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;
import org.chromium.chrome.browser.settings.BraveSettingsLauncherImpl;
import org.chromium.chrome.browser.settings.BraveWalletAddNetworksFragment;
import org.chromium.chrome.browser.util.LiveDataUtil;
import org.chromium.components.browser_ui.settings.SettingsLauncher;
import org.chromium.content_public.browser.UiThreadTaskTraits;

import java.util.ArrayList;

public class NetworkSelectorActivity
        extends BraveWalletBaseActivity implements NetworkSelectorAdapter.NetworkClickListener {
    public static String NETWORK_SELECTOR_MODE = "network_selector_mode";
    private static final String TAG = NetworkSelectorActivity.class.getSimpleName();
    private NetworkSelectorModel.Mode mMode;
    private RecyclerView mRVNetworkSelector;
    private NetworkSelectorAdapter networkSelectorAdapter;
    private MaterialToolbar mToolbar;
    private String mSelectedNetwork;
    private SettingsLauncher mSettingsLauncher;
    private WalletModel mWalletModel;
    private NetworkSelectorModel mNetworkSelectorModel;

    @Override
    protected void triggerLayoutInflation() {
        setContentView(R.layout.activity_network_selector);
        mToolbar = findViewById(R.id.toolbar);
        mToolbar.setTitle(R.string.brave_wallet_network_activity_title);
        mToolbar.setOnMenuItemClickListener(item -> {
            launchAddNetwork();
            return true;
        });

        Intent intent = getIntent();
        mMode = JavaUtils.safeVal(
                (NetworkSelectorModel.Mode) intent.getSerializableExtra(NETWORK_SELECTOR_MODE),
                NetworkSelectorModel.Mode.DEFAULT_WALLET_NETWORK);
        mRVNetworkSelector = findViewById(R.id.rv_network_activity);
        onInitialLayoutInflationComplete();
    }

    @Override
    public void finishNativeInitialization() {
        super.finishNativeInitialization();
        initState();
    }

    private void initState() {
        BraveActivity activity = BraveActivity.getBraveActivity();
        if (activity != null) {
            mWalletModel = activity.getWalletModel();
        }
        mSettingsLauncher = new BraveSettingsLauncherImpl();
        mNetworkSelectorModel =
                mWalletModel.getCryptoModel().getNetworkModel().openNetworkSelectorModel(mMode);
        networkSelectorAdapter = new NetworkSelectorAdapter(this, new ArrayList<>());
        mRVNetworkSelector.setAdapter(networkSelectorAdapter);
        networkSelectorAdapter.setOnNetworkItemSelected(this);
        mNetworkSelectorModel.mPrimaryNetworks.observe(this, primaryNetworkInfos -> {
            networkSelectorAdapter.addPrimaryNetwork(primaryNetworkInfos);
        });
        mNetworkSelectorModel.mSecondaryNetworks.observe(this, secondaryNetworkInfos -> {
            networkSelectorAdapter.addSecondaryNetwork(secondaryNetworkInfos);
        });
        setSelectedNetworkObserver();
    }

    private void launchAddNetwork() {
        Bundle fragmentArgs = new Bundle();
        fragmentArgs.putString(ADD_NETWORK_FRAGMENT_ARG_CHAIN_ID, "");
        fragmentArgs.putBoolean(ADD_NETWORK_FRAGMENT_ARG_ACTIVE_NETWORK, false);
        Intent intent = mSettingsLauncher.createSettingsActivityIntent(
                this, BraveWalletAddNetworksFragment.class.getName(), fragmentArgs);
        startActivity(intent);
    }

    private void setSelectedNetworkObserver() {
        if (mMode == NetworkSelectorModel.Mode.DEFAULT_WALLET_NETWORK) {
            LiveDataUtil.observeOnce(
                    mWalletModel.getCryptoModel().getNetworkModel().mDefaultNetwork,
                    networkInfo -> { updateNetworkSelection(networkInfo); });
        } else if (mMode == NetworkSelectorModel.Mode.LOCAL_NETWORK_FILTER) {
            mNetworkSelectorModel.mSelectedNetwork.observe(
                    this, networkInfo -> { updateNetworkSelection(networkInfo); });
        }
    }

    private void updateNetworkSelection(NetworkInfo networkInfo) {
        if (networkInfo != null) {
            mSelectedNetwork = Utils.getShortNameOfNetwork(networkInfo.chainName);
            networkSelectorAdapter.setSelectedNetwork(networkInfo.chainName);
        }
    }

    @Override
    public void onNetworkItemSelected(NetworkInfo networkInfo) {
        if (mMode == NetworkSelectorModel.Mode.DEFAULT_WALLET_NETWORK) {
            mWalletModel.getCryptoModel().getNetworkModel().setNetworkWithAccountCheck(
                    networkInfo, isSelected -> { updateNetworkUi(networkInfo, isSelected); });
        } else if (mMode == NetworkSelectorModel.Mode.LOCAL_NETWORK_FILTER) {
            mNetworkSelectorModel.setNetworkWithAccountCheck(
                    networkInfo, isSelected -> { updateNetworkUi(networkInfo, isSelected); });
        }
    }

    private void updateNetworkUi(NetworkInfo networkInfo, Boolean isSelected) {
        if (!isSelected) {
            Toast.makeText(this,
                         getString(R.string.brave_wallet_network_selection_error,
                                 networkInfo.chainName),
                         Toast.LENGTH_SHORT)
                    .show();
        }
        // Add little delay for smooth selection animation
        PostTask.postDelayedTask(UiThreadTaskTraits.DEFAULT, () -> {
            if (!isActivityFinishingOrDestroyed()) {
                finish();
            }
        }, 200);
    }
}
