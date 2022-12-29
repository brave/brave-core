/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.settings;

import android.content.Context;
import android.util.AttributeSet;
import android.widget.TextView;

import androidx.preference.Preference;
import androidx.preference.PreferenceViewHolder;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

import org.chromium.brave_wallet.mojom.CoinType;
import org.chromium.brave_wallet.mojom.JsonRpcService;
import org.chromium.brave_wallet.mojom.NetworkInfo;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.crypto_wallet.JsonRpcServiceFactory;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;
import org.chromium.components.browser_ui.widget.TintedDrawable;
import org.chromium.mojo.bindings.ConnectionErrorHandler;
import org.chromium.mojo.system.MojoException;

public class BraveWalletNetworksPreference extends Preference
        implements ConnectionErrorHandler, NetworkListBaseAdapter.ItemClickListener,
                   BraveWalletAddNetworksFragment.Refresher {
    private TextView mbtAddNetwork;
    private RecyclerView mRecyclerView;
    private NetworkListBaseAdapter mAdapter;
    private BraveWalletAddNetworksFragment.Launcher mLauncher;
    private JsonRpcService mJsonRpcService;

    public BraveWalletNetworksPreference(Context context, AttributeSet attrs) {
        super(context, attrs);
        mAdapter = new NetworkListBaseAdapter(context, this);
    }

    @Override
    public void onBindViewHolder(PreferenceViewHolder holder) {
        super.onBindViewHolder(holder);

        assert mLauncher != null;

        mbtAddNetwork = (TextView) holder.findViewById(R.id.add_network);
        mbtAddNetwork.setCompoundDrawablesRelativeWithIntrinsicBounds(
                TintedDrawable.constructTintedDrawable(getContext(), R.drawable.plus,
                        R.color.default_control_color_active_baseline),
                null, null, null);
        mbtAddNetwork.setOnClickListener(view -> { mLauncher.launchAddNetwork("", false); });
        mLauncher.setRefresher(this);

        InitJsonRpcService();

        mRecyclerView = (RecyclerView) holder.findViewById(R.id.network_list);
        LinearLayoutManager layoutMangager = new LinearLayoutManager(getContext());
        mRecyclerView.setLayoutManager(layoutMangager);
        updateNetworksList();
    }

    public void destroy() {
        mJsonRpcService.close();
    }

    @Override
    public void onConnectionError(MojoException e) {
        mJsonRpcService.close();
        mJsonRpcService = null;
        InitJsonRpcService();
    }

    @Override
    public void onItemClicked(NetworkInfo chain, boolean activeNetwork) {
        mLauncher.launchAddNetwork(chain.chainId, activeNetwork);
    }

    @Override
    public void onItemRemove(NetworkInfo chain) {
        assert mJsonRpcService != null;
        mJsonRpcService.removeChain(chain.chainId, CoinType.ETH, success -> {
            if (!success) {
                return;
            }
            updateNetworksList();
        });
    }

    @Override
    public void onItemSetAsActive(NetworkInfo chain) {
        assert mJsonRpcService != null;
        mJsonRpcService.setNetwork(chain.chainId, CoinType.ETH, success -> {
            if (!success) {
                return;
            }
            updateNetworksList();
        });
    }

    @Override
    public void refreshNetworksList() {
        updateNetworksList();
    }

    private void InitJsonRpcService() {
        if (mJsonRpcService != null) {
            return;
        }

        mJsonRpcService = JsonRpcServiceFactory.getInstance().getJsonRpcService(this);
    }

    public void registerActivityLauncher(BraveWalletAddNetworksFragment.Launcher launcher) {
        mLauncher = launcher;
    }

    private void updateNetworksList() {
        BraveActivity activity = BraveActivity.getBraveActivity();
        if (activity != null) {
            activity.getWalletModel().getCryptoModel().getNetworkModel().refreshNetworks();
        }
        assert mJsonRpcService != null;
        mJsonRpcService.getChainId(CoinType.ETH, chainId -> {
            mJsonRpcService.getAllNetworks(CoinType.ETH, chains -> {
                mJsonRpcService.getCustomNetworks(CoinType.ETH, customNetworkIds -> {
                    mAdapter.setDisplayedNetworks(
                            chainId, Utils.getNetworkInfosByChainIds(customNetworkIds, chains));
                    if (mRecyclerView.getAdapter() != mAdapter) {
                        mRecyclerView.setAdapter(mAdapter);
                    }
                });
            });
        });
    }
}
