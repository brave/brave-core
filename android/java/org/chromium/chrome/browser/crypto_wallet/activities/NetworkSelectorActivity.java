/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.activities;

import android.widget.Toast;

import androidx.recyclerview.widget.RecyclerView;

import com.google.android.material.appbar.MaterialToolbar;

import org.chromium.base.Log;
import org.chromium.brave_wallet.mojom.CoinType;
import org.chromium.brave_wallet.mojom.JsonRpcService;
import org.chromium.brave_wallet.mojom.NetworkInfo;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.crypto_wallet.adapters.NetworkSelectorAdapter;
import org.chromium.chrome.browser.crypto_wallet.adapters.NetworkSelectorAdapter.NetworkSelectorItem;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;

public class NetworkSelectorActivity
        extends BraveWalletBaseActivity implements NetworkSelectorAdapter.NetworkClickListener {
    private static final String TAG = NetworkSelectorActivity.class.getSimpleName();
    private RecyclerView mRVNetworkSelector;
    private NetworkSelectorAdapter networkSelectorAdapter;
    private MaterialToolbar mToolbar;
    private String mSelectedNetwork;

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
        JsonRpcService jsonRpcService = getJsonRpcService();
        assert jsonRpcService != null;
        jsonRpcService.getAllNetworks(CoinType.ETH, chains -> {
            networkSelectorAdapter =
                    new NetworkSelectorAdapter(this, Utils.makeNetworksList(this, chains),
                            Utils.makeNetworksAbbrevList(this, chains));
            networkSelectorAdapter.setOnNetworkItemSelected(this);
            mRVNetworkSelector.setAdapter(networkSelectorAdapter);
            fetchSelectedNetwork();
        });
    }

    private void fetchSelectedNetwork() {
        JsonRpcService jsonRpcService = getJsonRpcService();
        assert jsonRpcService != null;
        jsonRpcService.getNetwork(CoinType.ETH, selectedNetwork -> {
            mSelectedNetwork = selectedNetwork.chainName;
            networkSelectorAdapter.setSelectedNetwork(selectedNetwork.chainName);
        });
    }

    @Override
    public void onNetworkItemSelected(NetworkSelectorItem networkSelectorItem) {
        JsonRpcService jsonRpcService = getJsonRpcService();
        if (jsonRpcService != null) {
            jsonRpcService.getAllNetworks(CoinType.ETH, chains -> {
                NetworkInfo selectedNetwork =
                        Utils.getNetworkInfoByName(networkSelectorItem.getNetworkName(), chains);
                jsonRpcService.setNetwork(
                        selectedNetwork.chainId, CoinType.ETH, (success) -> {
                            if (!success) {
                                Toast.makeText(this,
                                             getString(
                                                     R.string.brave_wallet_network_selection_error,
                                                     networkSelectorItem.getNetworkShortName()),
                                             Toast.LENGTH_SHORT)
                                        .show();
                                networkSelectorAdapter.setSelectedNetwork(mSelectedNetwork);
                                Log.e(TAG, "Could not set network");
                            } else {
                                finish();
                            }
                        });
            });
        }
    }
}
