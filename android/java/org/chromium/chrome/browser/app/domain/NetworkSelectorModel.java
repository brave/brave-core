/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.app.domain;

import android.content.Context;

import androidx.lifecycle.LiveData;
import androidx.lifecycle.MutableLiveData;
import androidx.lifecycle.Transformations;

import org.chromium.base.Callbacks;
import org.chromium.brave_wallet.mojom.NetworkInfo;
import org.chromium.chrome.browser.crypto_wallet.util.NetworkUtils;
import org.chromium.chrome.browser.util.LiveDataUtil;

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
    private final MutableLiveData<List<NetworkInfo>> _mSelectedNetworks;
    private Mode mMode;
    private SelectionMode mSelectionMode;
    public LiveData<NetworkModel.NetworkLists> mNetworkListsLd;
    private final LiveData<NetworkInfo> mSelectedNetwork;
    private String mSelectedChainId;
    public final LiveData<List<NetworkInfo>> mSelectedNetworks;

    public NetworkSelectorModel(
            Mode mode, SelectionMode type, NetworkModel networkModel, Context context) {
        mMode = mode;
        mSelectionMode = type;
        if (mMode == null) {
            mMode = Mode.DEFAULT_WALLET_NETWORK;
        }
        mNetworkModel = networkModel;
        mContext = context;
        _mSelectedNetwork = new MutableLiveData<>();
        mSelectedNetwork = _mSelectedNetwork;
        _mSelectedNetworks = new MutableLiveData<>(Collections.emptyList());
        mSelectedNetworks = _mSelectedNetworks;
        init();
    }

    public NetworkSelectorModel(NetworkModel networkModel, Context context) {
        this(Mode.DEFAULT_WALLET_NETWORK, SelectionMode.SINGLE, networkModel, context);
    }

    public void init() {
        if (mMode == Mode.DEFAULT_WALLET_NETWORK) {
            _mSelectedNetwork.postValue(mNetworkModel.mDefaultNetwork.getValue());
        } else if (SelectionMode.MULTI == mSelectionMode) {
            LiveDataUtil.observeOnce(mNetworkModel.mNetworkLists, networkLists -> {
                _mSelectedNetworks.postValue(
                        NetworkUtils.nonTestNetwork(networkLists.mCoreNetworks));
            });
        } else if (mMode == Mode.LOCAL_NETWORK_FILTER) {
            if (mSelectedChainId == null) {
                _mSelectedNetwork.postValue(NetworkUtils.getAllNetworkOption(mContext));
            }
        }
        mNetworkListsLd = Transformations.map(mNetworkModel.mNetworkLists, networkLists -> {
            if (networkLists == null) return new NetworkModel.NetworkLists();
            NetworkModel.NetworkLists networkListsCopy =
                    new NetworkModel.NetworkLists(networkLists);
            List<NetworkInfo> allNetworkList = new ArrayList<>(networkLists.mCoreNetworks);
            if (mMode == Mode.LOCAL_NETWORK_FILTER && SelectionMode.MULTI != mSelectionMode) {
                NetworkInfo allNetwork = NetworkUtils.getAllNetworkOption(mContext);
                networkListsCopy.mPrimaryNetworkList.add(0, allNetwork);
                // Selected local network can be "All networks"
                allNetworkList.add(0, allNetwork);
            }
            final NetworkInfo network = mNetworkModel.getNetwork(mSelectedChainId);
            if (network != null) {
                updateLocalNetwork(allNetworkList, mSelectedChainId, network.coin);
            }
            return networkListsCopy;
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
                // Mode.DEFAULT_WALLET_NETWORK is for panel network selection which means that we
                // should change active orign's network.
                boolean setNetworkAsDefault = mMode != Mode.DEFAULT_WALLET_NETWORK;
                // Delegate to network model to handle account creation flow if required
                mNetworkModel.setNetworkWithAccountCheck(
                        networkToBeSetAsSelected, setNetworkAsDefault, isSet -> {
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

    public void setSelectedNetworks(List<NetworkInfo> networkInfos) {
        _mSelectedNetworks.postValue(networkInfos);
    }

    public Mode getMode() {
        return mMode;
    }

    public SelectionMode getSelectionType() {
        return mSelectionMode;
    }
    private void updateLocalNetwork(List<NetworkInfo> networkInfos, String chainId, int coin) {
        NetworkInfo networkInfo = NetworkUtils.findNetwork(networkInfos, chainId, coin);
        if (networkInfo != null) {
            _mSelectedNetwork.postValue(networkInfo);
        }
    }

    public enum Mode { DEFAULT_WALLET_NETWORK, LOCAL_NETWORK_FILTER }
    public enum SelectionMode { SINGLE, MULTI }
}
