/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.settings;

import android.content.Context;
import android.util.AttributeSet;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.preference.Preference;
import androidx.preference.PreferenceViewHolder;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

import org.chromium.base.Callbacks;
import org.chromium.brave_wallet.mojom.CoinType;
import org.chromium.brave_wallet.mojom.JsonRpcService;
import org.chromium.brave_wallet.mojom.NetworkInfo;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.crypto_wallet.JsonRpcServiceFactory;
import org.chromium.components.browser_ui.widget.TintedDrawable;
import org.chromium.mojo.bindings.ConnectionErrorHandler;
import org.chromium.mojo.system.MojoException;

/**
 * Brave Wallet network preference hosted by {@link BraveWalletNetworksPreferenceFragment}. It shows
 * all the networks available and let the user add, edit, or remove them.
 */
public class BraveWalletNetworksPreference extends Preference
        implements ConnectionErrorHandler, NetworkPreferenceAdapter.ItemClickListener {

    private TextView mAddNetwork;
    private RecyclerView mRecyclerView;
    @Nullable private BraveWalletAddNetworksFragment.Listener mListener;
    private JsonRpcService mJsonRpcService;

    public BraveWalletNetworksPreference(Context context, AttributeSet attrs) {
        super(context, attrs);
        initJsonRpcService();
    }

    @Override
    public void onBindViewHolder(@NonNull PreferenceViewHolder holder) {
        super.onBindViewHolder(holder);

        mAddNetwork = (TextView) holder.findViewById(R.id.add_network);
        mRecyclerView = (RecyclerView) holder.findViewById(R.id.network_list);

        if (mAddNetwork != null) {
            mAddNetwork.setCompoundDrawablesRelativeWithIntrinsicBounds(
                    TintedDrawable.constructTintedDrawable(
                            getContext(),
                            R.drawable.plus,
                            R.color.default_control_color_active_baseline),
                    null,
                    null,
                    null);
            mAddNetwork.setOnClickListener(
                    view -> {
                        if (mListener != null) {
                            mListener.addNewNetwork();
                        }
                    });
        }

        if (mRecyclerView != null) {
            mRecyclerView.setLayoutManager(new LinearLayoutManager(getContext()));
        }
        updateNetworks();
    }

    public void destroy() {
        mJsonRpcService.close();
    }

    @Override
    public void onConnectionError(MojoException e) {
        mJsonRpcService.close();
        mJsonRpcService = null;
        initJsonRpcService();
    }

    @Override
    public void onItemClick(@NonNull NetworkInfo chain, boolean activeNetwork) {
        if (mListener != null) {
            mListener.modifyNetwork(chain.chainId, activeNetwork);
        }
    }

    @Override
    public void onItemRemove(@NonNull NetworkInfo chain) {
        assert mJsonRpcService != null;
        mJsonRpcService.removeChain(
                chain.chainId,
                CoinType.ETH,
                success -> {
                    if (!success) {
                        return;
                    }
                    updateNetworks();
                });
    }

    @Override
    public void onItemSetAsActive(@NonNull NetworkInfo chain) {
        assert mJsonRpcService != null;
        mJsonRpcService.setNetwork(
                chain.chainId,
                CoinType.ETH,
                null,
                success -> {
                    if (!success) {
                        return;
                    }
                    updateNetworks();
                });
    }

    public void setListener(@Nullable BraveWalletAddNetworksFragment.Listener listener) {
        mListener = listener;
    }

    public void updateNetworks() {
        if (mJsonRpcService != null && mRecyclerView != null) {
            getAvailableChainIds(
                    (defaultChainId, networks, customChainIds, hiddenChainIds) -> {
                        NetworkPreferenceAdapter adapter =
                                new NetworkPreferenceAdapter(
                                        getContext(),
                                        defaultChainId,
                                        networks,
                                        customChainIds,
                                        hiddenChainIds,
                                        this);
                        mRecyclerView.setAdapter(adapter);
                    });
        }
    }

    /**
     * Gets all the available networks including default chain ID, custom chain IDs, and hidden
     * chain IDs. The method can be called ONLY after the JSON RPC service has been correctly
     * initialized.
     *
     * @param callback Callback returning four parameters default chain ID, all networks available,
     *     custom chain IDs, and hidden chain IDs.
     */
    private void getAvailableChainIds(
            @NonNull
                    final Callbacks.Callback4<String, NetworkInfo[], String[], String[]> callback) {
        mJsonRpcService.getDefaultChainId(
                CoinType.ETH,
                defaultChainId ->
                        mJsonRpcService.getAllNetworks(
                                CoinType.ETH,
                                networks ->
                                        mJsonRpcService.getCustomNetworks(
                                                CoinType.ETH,
                                                customChainIds ->
                                                        mJsonRpcService.getHiddenNetworks(
                                                                CoinType.ETH,
                                                                hiddenChainIds ->
                                                                        callback.call(
                                                                                defaultChainId,
                                                                                networks,
                                                                                customChainIds,
                                                                                hiddenChainIds)))));
    }

    /**
     * Initialize JSON RPC service directly without network model as preference screen is not linked
     * to Brave activity so it's not possible to retrieve activity models.
     */
    private void initJsonRpcService() {
        if (mJsonRpcService != null) {
            return;
        }

        mJsonRpcService = JsonRpcServiceFactory.getInstance().getJsonRpcService(this);
    }
}
