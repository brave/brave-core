/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.app.domain;

import androidx.annotation.NonNull;
import androidx.lifecycle.LiveData;
import androidx.lifecycle.MutableLiveData;
import androidx.lifecycle.Transformations;

import org.chromium.base.Callbacks;
import org.chromium.brave_wallet.mojom.NetworkInfo;

import java.util.Collections;
import java.util.List;

/**
 * NetworkSelectorModel handles the selector of network for either global or local network
 * selection. Global selection can be observed via {@link NetworkModel#mDefaultNetwork}
 */
public class NetworkSelectorModel {
    private final NetworkModel mNetworkModel;
    private final MutableLiveData<List<NetworkInfo>> _mSelectedNetworks;
    private final SelectionMode mSelectionMode;
    public LiveData<NetworkModel.NetworkLists> mNetworkListsLd;
    public final LiveData<List<NetworkInfo>> mSelectedNetworks;

    public NetworkSelectorModel(SelectionMode type, NetworkModel networkModel) {
        mSelectionMode = type;
        mNetworkModel = networkModel;
        _mSelectedNetworks = new MutableLiveData<>(Collections.emptyList());
        mSelectedNetworks = _mSelectedNetworks;
        init();
    }

    public void init() {
        mNetworkListsLd =
                Transformations.map(
                        mNetworkModel.mNetworkLists,
                        networkLists -> {
                            if (networkLists == null) {
                                return new NetworkModel.NetworkLists();
                            }
                            return new NetworkModel.NetworkLists(networkLists);
                        });
    }

    public void setNetworkWithAccountCheck(
            @NonNull NetworkInfo networkToBeSetAsSelected, Callbacks.Callback1<Boolean> callback) {
        boolean setNetworkAsDefault = false;
        // Delegate to network model to handle account creation flow if required
        mNetworkModel.setNetworkWithAccountCheck(
                networkToBeSetAsSelected, setNetworkAsDefault, callback);
    }

    public LiveData<NetworkInfo> getSelectedNetwork() {
        return mNetworkModel.mDefaultNetwork;
    }

    public void setSelectedNetworks(List<NetworkInfo> networkInfos) {
        _mSelectedNetworks.postValue(networkInfos);
    }

    public SelectionMode getSelectionType() {
        return mSelectionMode;
    }

    public enum SelectionMode {
        SINGLE,
        MULTI
    }
}
