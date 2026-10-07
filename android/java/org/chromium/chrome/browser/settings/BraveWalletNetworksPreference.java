/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.settings;

import android.content.Context;
import android.util.AttributeSet;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.widget.AppCompatButton;
import androidx.fragment.app.Fragment;
import androidx.preference.Preference;
import androidx.preference.PreferenceViewHolder;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

import org.chromium.base.Callback;
import org.chromium.brave_wallet.mojom.AllNetworks;
import org.chromium.brave_wallet.mojom.CoinType;
import org.chromium.brave_wallet.mojom.JsonRpcService;
import org.chromium.brave_wallet.mojom.NetworkInfo;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.crypto_wallet.BraveWalletServiceFactory;
import org.chromium.mojo.bindings.ConnectionErrorHandler;
import org.chromium.mojo.system.MojoException;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

/**
 * Brave Wallet network preference hosted by {@link BraveWalletNetworksPreferenceFragment}. It shows
 * all the networks available and let the user add, edit, or remove them.
 */
public class BraveWalletNetworksPreference extends Preference
        implements ConnectionErrorHandler, NetworkPreferenceAdapter.ItemClickListener {

    private AppCompatButton mAddNetwork;
    private RecyclerView mRecyclerView;
    @Nullable private BraveWalletAddNetworksFragment.Listener mListener;
    private JsonRpcService mJsonRpcService;
    private Fragment mPreferenceFragment;

    public BraveWalletNetworksPreference(Context context, AttributeSet attrs) {
        super(context, attrs);
        initJsonRpcService();
    }

    @Override
    public void onBindViewHolder(@NonNull PreferenceViewHolder holder) {
        super.onBindViewHolder(holder);

        mAddNetwork = (AppCompatButton) holder.findViewById(R.id.add_network);
        mRecyclerView = (RecyclerView) holder.findViewById(R.id.network_list);

        if (mAddNetwork != null) {
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
        if (mJsonRpcService != null) {
            mJsonRpcService.close();
            mJsonRpcService = null;
        }
    }

    @Override
    public void onConnectionError(MojoException e) {
        if (mJsonRpcService != null) {
            mJsonRpcService.close();
            mJsonRpcService = null;
        }
        if (mPreferenceFragment != null) {
            // TODO: remove when https://github.com/brave/brave-browser/issues/27887
            // will be resolved.
            mPreferenceFragment.requireActivity().recreate();
        }
    }

    @Override
    public void onItemEdit(@NonNull NetworkInfo chain, boolean activeNetwork) {
        if (mListener != null) {
            mListener.modifyNetwork(chain.chainId, activeNetwork);
        }
    }

    @Override
    public void onItemRemove(
            @NonNull NetworkInfo chain, @NonNull final Callback<Boolean> callback) {
        assert mJsonRpcService != null;
        mJsonRpcService.removeChain(chain.chainId, chain.coin, callback::onResult);
    }

    @Override
    public void onItemSetAsActive(
            @NonNull NetworkInfo chain, @NonNull final Callback<Boolean> callback) {
        assert mJsonRpcService != null;
        mJsonRpcService.setNetwork(chain.chainId, chain.coin, null, callback::onResult);
    }

    @Override
    public void onItemVisibilityChange(
            @NonNull NetworkInfo chain, boolean show, @NonNull Callback<Boolean> callback) {
        assert mJsonRpcService != null;
        if (show) {
            mJsonRpcService.removeHiddenNetwork(chain.coin, chain.chainId, callback::onResult);
        } else {
            mJsonRpcService.addHiddenNetwork(chain.coin, chain.chainId, callback::onResult);
        }
    }

    public void setListener(@Nullable BraveWalletAddNetworksFragment.Listener listener) {
        mListener = listener;
    }

    private static final int[] SUPPORTED_COIN_TYPES = {
        CoinType.ETH, CoinType.FIL, CoinType.SOL, CoinType.BTC
    };

    public void updateNetworks() {
        if (mJsonRpcService == null || mRecyclerView == null) {
            return;
        }
        mJsonRpcService.getAllNetworks(
                allNetworks -> {
                    final NetworkPreferenceAdapter.NetworkListContainer[] containers =
                            new NetworkPreferenceAdapter.NetworkListContainer
                                    [SUPPORTED_COIN_TYPES.length];
                    for (int i = 0; i < SUPPORTED_COIN_TYPES.length; i++) {
                        final int coinType = SUPPORTED_COIN_TYPES[i];
                        final int index = i;
                        mJsonRpcService.getDefaultChainId(
                                coinType,
                                defaultChainId -> {
                                    containers[index] =
                                            buildNetworkListContainer(
                                                    coinType, allNetworks, defaultChainId);
                                    for (NetworkPreferenceAdapter.NetworkListContainer container :
                                            containers) {
                                        if (container == null) {
                                            return;
                                        }
                                    }
                                    mRecyclerView.setAdapter(
                                            new NetworkPreferenceAdapter(
                                                    getContext(),
                                                    containers[0],
                                                    containers[1],
                                                    containers[2],
                                                    containers[3],
                                                    this));
                                });
                    }
                });
    }

    @SuppressWarnings("NoStreams")
    private NetworkInfo[] filterNetworksByCoin(
            @CoinType.EnumType final int coinType, NetworkInfo[] networks) {
        return Arrays.stream(networks).filter(n -> n.coin == coinType).toArray(NetworkInfo[]::new);
    }

    /**
     * Builds a {@link NetworkPreferenceAdapter.NetworkListContainer} for the given coin type out of
     * a single, already-fetched {@link AllNetworks} result.
     */
    private NetworkPreferenceAdapter.NetworkListContainer buildNetworkListContainer(
            @CoinType.EnumType final int coinType, AllNetworks allNetworks, String defaultChainId) {
        NetworkInfo[] coinNetworks = filterNetworksByCoin(coinType, allNetworks.networks);
        List<String> coinChainIds = new ArrayList<>();
        for (NetworkInfo network : coinNetworks) {
            coinChainIds.add(network.chainId);
        }
        List<String> customChainIdsList =
                new ArrayList<>(Arrays.asList(allNetworks.customChainIds));
        customChainIdsList.retainAll(coinChainIds);
        List<String> hiddenChainIdsList =
                new ArrayList<>(Arrays.asList(allNetworks.hiddenChainIds));
        hiddenChainIdsList.retainAll(coinChainIds);
        return new NetworkPreferenceAdapter.NetworkListContainer(
                coinNetworks,
                defaultChainId,
                hiddenChainIdsList.toArray(new String[0]),
                customChainIdsList.toArray(new String[0]));
    }

    /**
     * Initialize JSON RPC service directly without network model as preference screen is not linked
     * to Brave activity so it's not possible to retrieve activity models.
     */
    private void initJsonRpcService() {
        if (mJsonRpcService != null) {
            return;
        }

        mJsonRpcService = BraveWalletServiceFactory.getInstance().getJsonRpcService(this);
    }

    public void setPreferenceFragment(@NonNull final Fragment preferenceFragment) {
        mPreferenceFragment = preferenceFragment;
    }
}
