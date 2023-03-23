/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.app.domain;

import android.content.Context;

import androidx.lifecycle.LiveData;
import androidx.lifecycle.MutableLiveData;
import androidx.lifecycle.Transformations;

import org.chromium.brave_wallet.mojom.NetworkInfo;
import org.chromium.chrome.browser.crypto_wallet.presenters.NetworkInfoPresenter;
import org.chromium.chrome.browser.crypto_wallet.util.NetworkUtils;
import org.chromium.mojo.bindings.Callbacks;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

/**
 * NetworkSelectorModel handles the selector of network for either global or local network
 * selection.
 * {@link NetworkSelectorModel#mMode} acts as a switch between global or local network selection
 * live data events. Local selection can be observed via {@link
 * NetworkSelectorModel#mSelectedNetwork} and global via {@link NetworkModel#mDefaultNetwork}
 */
public class NetworkSelectorModel {
    private final Context mContext;
    private final NetworkModel mNetworkModel;
    private final MutableLiveData<NetworkInfo> _mSelectedNetwork;
    private Mode mMode;
    public LiveData<List<NetworkInfoPresenter>> mPrimaryNetworks;
    public LiveData<List<NetworkInfoPresenter>> mSecondaryNetworks;
    private final LiveData<NetworkInfo> mSelectedNetwork;
    private String mSelectedChainId;

    public NetworkSelectorModel(Mode mode, NetworkModel networkModel, Context context) {
        mMode = mode;
        if (mMode == null) {
            mMode = Mode.DEFAULT_WALLET_NETWORK;
        }
        mNetworkModel = networkModel;
        mContext = context;
        _mSelectedNetwork = new MutableLiveData<>();
        mSelectedNetwork = _mSelectedNetwork;
        init();
    }

    public NetworkSelectorModel(NetworkModel networkModel, Context context) {
        this(Mode.DEFAULT_WALLET_NETWORK, networkModel, context);
    }

    public void init() {
        if (mMode == Mode.DEFAULT_WALLET_NETWORK) {
            _mSelectedNetwork.postValue(mNetworkModel.mDefaultNetwork.getValue());
        } else if (mMode == Mode.LOCAL_NETWORK_FILTER) {
            if (mSelectedChainId == null) {
                _mSelectedNetwork.postValue(NetworkUtils.getAllNetworkOption(mContext));
            }
        }
        mPrimaryNetworks = Transformations.map(mNetworkModel.mPrimaryNetworks, networkInfos -> {
            List<NetworkInfoPresenter> list = new ArrayList<>();
            if (mMode == Mode.LOCAL_NETWORK_FILTER) {
                list.add(0,
                        new NetworkInfoPresenter(NetworkUtils.getAllNetworkOption(mContext), true,
                                Collections.emptyList()));
            }
            for (NetworkInfo networkInfo : networkInfos) {
                list.add(new NetworkInfoPresenter(
                        networkInfo, true, mNetworkModel.getSubTestNetworks(networkInfo)));
            }
            updateLocalNetwork(networkInfos, mSelectedChainId);
            return list;
        });
        mSecondaryNetworks = Transformations.map(mNetworkModel.mSecondaryNetworks, networkInfos -> {
            List<NetworkInfoPresenter> list = new ArrayList<>();
            for (NetworkInfo networkInfo : networkInfos) {
                list.add(new NetworkInfoPresenter(networkInfo, false, Collections.emptyList()));
            }
            updateLocalNetwork(networkInfos, mSelectedChainId);
            return list;
        });
    }

    /**
     * Reset the mode
     * DEFAULT_WALLET_NETWORK for global wallet network
     * LOCAL_NETWORK_FILTER for only onscreen network selection via "mSelectedNetwork"
     * @param mode to set selected network
     */
    public void updateSelectorMode(Mode mode) {
        this.mMode = mode;
        init();
    }

    public void setNetworkWithAccountCheck(
            NetworkInfo networkToBeSetAsSelected, Callbacks.Callback1<Boolean> callback) {
        // Default/Global wallet network does not support "All Networks"
        if (!networkToBeSetAsSelected.chainId.equals(
                    NetworkUtils.getAllNetworkOption(mContext).chainId)) {
            boolean hasAccountOfNetworkType =
                    mNetworkModel.hasAccountOfNetworkType(networkToBeSetAsSelected);
            if (!hasAccountOfNetworkType || mMode == Mode.DEFAULT_WALLET_NETWORK) {
                // Delegate to network model to handle account creation flow if required
                mNetworkModel.setNetworkWithAccountCheck(networkToBeSetAsSelected, isSet -> {
                    callback.call(isSet);
                    if (isSet) {
                        _mSelectedNetwork.postValue(networkToBeSetAsSelected);
                    }
                });
                return;
            }
        }
        if (mMode == Mode.LOCAL_NETWORK_FILTER) {
            _mSelectedNetwork.postValue(networkToBeSetAsSelected);
            callback.call(true);
        }
    }

    public LiveData<NetworkInfo> getSelectedNetwork() {
        if (mMode == Mode.DEFAULT_WALLET_NETWORK) {
            return mNetworkModel.mDefaultNetwork;
        } else {
            return mSelectedNetwork;
        }
    }

    public Mode getMode() {
        return mMode;
    }

    private void updateLocalNetwork(List<NetworkInfo> networkInfos, String chainId) {
        NetworkInfo networkInfo = NetworkUtils.findNetwork(networkInfos, chainId);
        if (networkInfo != null) {
            _mSelectedNetwork.postValue(networkInfo);
        }
    }

    public enum Mode { DEFAULT_WALLET_NETWORK, LOCAL_NETWORK_FILTER }
}
