/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.activities;

import android.widget.Toast;

import androidx.recyclerview.widget.RecyclerView;

import com.google.android.material.appbar.MaterialToolbar;

import org.chromium.brave_wallet.mojom.NetworkInfo;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.app.domain.WalletModel;
import org.chromium.chrome.browser.crypto_wallet.adapters.NetworkSelectorAdapter;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;

public class NetworkSelectorActivity
        extends BraveWalletBaseActivity implements NetworkSelectorAdapter.NetworkClickListener {
    private static final String TAG = NetworkSelectorActivity.class.getSimpleName();
    private RecyclerView mRVNetworkSelector;
    private NetworkSelectorAdapter networkSelectorAdapter;
    private MaterialToolbar mToolbar;
    private String mSelectedNetwork;
    private WalletModel mWalletModel;

    @Override
    protected void triggerLayoutInflation() {
        setContentView(R.layout.activity_network_selector);
        mToolbar = findViewById(R.id.toolbar);
        mToolbar.setTitle(R.string.brave_wallet_network_activity_title);
        setSupportActionBar(mToolbar);

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
        mWalletModel.getCryptoModel().getNetworkModel().mCryptoNetworks.observe(
                this, networkInfos -> {
                    networkSelectorAdapter = new NetworkSelectorAdapter(this, networkInfos);
                    networkSelectorAdapter.setOnNetworkItemSelected(this);
                    mRVNetworkSelector.setAdapter(networkSelectorAdapter);
                });
        setSelectedNetworkObserver();
    }

    private void setSelectedNetworkObserver() {
        mWalletModel.getCryptoModel().getNetworkModel().mDefaultNetwork.observe(
                this, networkInfo -> {
                    if (networkInfo != null) {
                        mSelectedNetwork = Utils.getShortNameOfNetwork(networkInfo.chainName);
                        networkSelectorAdapter.setSelectedNetwork(networkInfo.chainName);
                    }
                });
    }

    @Override
    public void onNetworkItemSelected(NetworkInfo networkInfo) {
        mWalletModel.getCryptoModel().getNetworkModel().setNetworkWithAccountCheck(
                networkInfo, isSelected -> {
                    if (!isSelected) {
                        Toast.makeText(this,
                                     getString(R.string.brave_wallet_network_selection_error,
                                             networkInfo.chainName),
                                     Toast.LENGTH_SHORT)
                                .show();
                        networkSelectorAdapter.setSelectedNetwork(mSelectedNetwork);
                    }
                    finish();
                });
    }
}
