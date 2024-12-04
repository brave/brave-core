/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.app.domain;

import android.text.TextUtils;
import android.util.Pair;

import androidx.annotation.NonNull;
import androidx.lifecycle.LiveData;
import androidx.lifecycle.MediatorLiveData;
import androidx.lifecycle.MutableLiveData;

import org.chromium.base.Callbacks;
import org.chromium.brave_wallet.mojom.AccountInfo;
import org.chromium.brave_wallet.mojom.BraveWalletConstants;
import org.chromium.brave_wallet.mojom.BraveWalletService;
import org.chromium.brave_wallet.mojom.CoinType;
import org.chromium.brave_wallet.mojom.JsonRpcService;
import org.chromium.brave_wallet.mojom.JsonRpcServiceObserver;
import org.chromium.brave_wallet.mojom.NetworkInfo;
import org.chromium.chrome.browser.crypto_wallet.util.AndroidUtils;
import org.chromium.chrome.browser.crypto_wallet.util.JavaUtils;
import org.chromium.chrome.browser.crypto_wallet.util.NetworkUtils;
import org.chromium.chrome.browser.crypto_wallet.util.WalletConstants;
import org.chromium.mojo.system.MojoException;
import org.chromium.url.internal.mojom.Origin;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.List;
import java.util.Map;
import java.util.stream.Collectors;

public class NetworkModel implements JsonRpcServiceObserver {
    private BraveWalletService mBraveWalletService;
    private JsonRpcService mJsonRpcService;
    private final Object mLock = new Object();
    private Mode mMode = Mode.WALLET_MODE;

    private final MediatorLiveData<String> _mChainId;
    private final MediatorLiveData<List<NetworkInfo>> _mDefaultCoinCryptoNetworks;
    private final MutableLiveData<List<NetworkInfo>> _mCryptoNetworks;
    private final CryptoSharedData mSharedData;
    private final CryptoSharedActions mCryptoActions;
    private final MediatorLiveData<Pair<String, List<NetworkInfo>>> _mPairChainAndNetwork;
    private final MediatorLiveData<NetworkInfo> _mNeedToCreateAccountForNetwork;
    private final MediatorLiveData<NetworkInfo> _mDefaultNetwork;
    private final MediatorLiveData<String[]> _mCustomNetworkIds;
    private final MediatorLiveData<List<NetworkInfo>> _mPrimaryNetworks;
    private final MediatorLiveData<List<NetworkInfo>> _mSecondaryNetworks;

    public final LiveData<String> mChainId;
    private final MutableLiveData<NetworkLists> _mNetworkLists;
    public final LiveData<String[]> mCustomNetworkIds;
    public LiveData<NetworkInfo> mNeedToCreateAccountForNetwork;
    public final LiveData<Pair<String, List<NetworkInfo>>> mPairChainAndNetwork;
    public final LiveData<List<NetworkInfo>> mDefaultCoinCryptoNetworks;
    public final LiveData<List<NetworkInfo>> mCryptoNetworks;
    public final LiveData<NetworkInfo> mDefaultNetwork;
    public final LiveData<List<NetworkInfo>> mPrimaryNetworks;
    public final LiveData<List<NetworkInfo>> mSecondaryNetworks;
    public final LiveData<NetworkLists> mNetworkLists;

    @SuppressWarnings("NoStreams")
    public NetworkModel(
            BraveWalletService braveWalletService,
            @NonNull JsonRpcService jsonRpcService,
            CryptoSharedData sharedData,
            CryptoSharedActions cryptoSharedActions) {
        mBraveWalletService = braveWalletService;
        mJsonRpcService = jsonRpcService;
        mSharedData = sharedData;
        mCryptoActions = cryptoSharedActions;

        _mChainId = new MediatorLiveData<>();
        _mChainId.setValue(BraveWalletConstants.MAINNET_CHAIN_ID);
        mChainId = _mChainId;
        _mDefaultCoinCryptoNetworks = new MediatorLiveData<>();
        mDefaultCoinCryptoNetworks = _mDefaultCoinCryptoNetworks;
        _mCryptoNetworks = new MutableLiveData<>(Collections.emptyList());
        mCryptoNetworks = _mCryptoNetworks;
        _mPairChainAndNetwork = new MediatorLiveData<>();
        mPairChainAndNetwork = _mPairChainAndNetwork;
        _mDefaultNetwork = new MediatorLiveData<>();
        mDefaultNetwork = _mDefaultNetwork;
        _mNeedToCreateAccountForNetwork = new MediatorLiveData<>();
        mNeedToCreateAccountForNetwork = _mNeedToCreateAccountForNetwork;
        _mCustomNetworkIds = new MediatorLiveData<>();
        _mCustomNetworkIds.postValue(new String[0]);
        mCustomNetworkIds = _mCustomNetworkIds;
        _mPrimaryNetworks = new MediatorLiveData<>();
        mPrimaryNetworks = _mPrimaryNetworks;
        _mSecondaryNetworks = new MediatorLiveData<>();
        mSecondaryNetworks = _mSecondaryNetworks;
        jsonRpcService.addObserver(this);
        _mNetworkLists = new MutableLiveData<>();
        mNetworkLists = _mNetworkLists;
        _mPairChainAndNetwork.setValue(Pair.create("", Collections.emptyList()));
        _mPairChainAndNetwork.addSource(
                _mChainId,
                chainId -> {
                    _mPairChainAndNetwork.setValue(
                            Pair.create(chainId, _mDefaultCoinCryptoNetworks.getValue()));
                });
        _mPairChainAndNetwork.addSource(
                _mDefaultCoinCryptoNetworks,
                networks -> {
                    _mPairChainAndNetwork.setValue(Pair.create(_mChainId.getValue(), networks));
                });
        _mDefaultNetwork.addSource(
                _mPairChainAndNetwork,
                chainIdAndInfosPair -> {
                    String chainId = chainIdAndInfosPair.first;
                    List<NetworkInfo> cryptoNetworks = chainIdAndInfosPair.second;
                    if (chainId == null || cryptoNetworks == null) return;
                    for (NetworkInfo networkInfo : cryptoNetworks) {
                        if (networkInfo.chainId.equals(chainId)) {
                            _mDefaultNetwork.postValue(networkInfo);
                            break;
                        }
                    }
                });
        _mChainId.addSource(mSharedData.getCoinTypeLd(), coinType -> updateChainId());
        _mDefaultCoinCryptoNetworks.addSource(
                mSharedData.getCoinTypeLd(),
                coinType -> {
                    getAllNetworks(
                            mJsonRpcService,
                            networkInfoSet -> {
                                _mDefaultCoinCryptoNetworks.postValue(
                                        new ArrayList<>(
                                                networkInfoSet.stream()
                                                        .filter(n -> n.coin == coinType)
                                                        .collect(Collectors.toList())));
                            });
                });

        _mCustomNetworkIds.addSource(
                mSharedData.getCoinTypeLd(),
                coinType -> {
                    mJsonRpcService.getCustomNetworks(coinType, _mCustomNetworkIds::postValue);
                });
        _mPrimaryNetworks.addSource(mCryptoNetworks, networkInfos -> {
            List<NetworkInfo> primaryNws = new ArrayList<>();
            for (NetworkInfo networkInfo : networkInfos) {
                if (WalletConstants.SUPPORTED_TOP_LEVEL_CHAIN_IDS.contains(networkInfo.chainId)) {
                    primaryNws.add(networkInfo);
                }
            }
            _mPrimaryNetworks.postValue(primaryNws);
        });
        _mSecondaryNetworks.addSource(mCryptoNetworks, networkInfos -> {
            List<NetworkInfo> secondaryNws = new ArrayList<>();
            for (NetworkInfo networkInfo : networkInfos) {
                if (!WalletConstants.SUPPORTED_TOP_LEVEL_CHAIN_IDS.contains(networkInfo.chainId)
                        && !WalletConstants.KNOWN_TEST_CHAIN_IDS.contains(networkInfo.chainId)) {
                    secondaryNws.add(networkInfo);
                }
            }
            _mSecondaryNetworks.postValue(secondaryNws);
        });
    }

    private void updateChainId() {
        if (mMode == Mode.WALLET_MODE) {
            if (mJsonRpcService == null) {
                return;
            }
            Integer coinBoxed = mSharedData.getCoinTypeLd().getValue();
            // Unboxing a live data value directly may
            // produce a null pointer exception, so it's required
            // checking against null.
            if (coinBoxed == null) {
                return;
            }
            @CoinType.EnumType int coin = coinBoxed;
            mJsonRpcService.getNetwork(coin, null, networkInfo -> {
                if (networkInfo != null) {
                    _mChainId.postValue(networkInfo.chainId);
                }
            });
        } else if (mMode == Mode.PANEL_MODE) {
            if (mBraveWalletService == null) {
                return;
            }
            mBraveWalletService.getNetworkForSelectedAccountOnActiveOrigin(networkInfo -> {
                if (networkInfo != null) {
                    _mChainId.postValue(networkInfo.chainId);
                }
            });
        }
    }

    public void setAccountInfosFromKeyRingModel(
            LiveData<List<AccountInfo>> accountInfosFromKeyRingModel) {
        setUpAccountObserver(accountInfosFromKeyRingModel);
    }

    void setUpAccountObserver(LiveData<List<AccountInfo>> accounts) {
        // getAccounts can be null as it's being set via a setter
        // clear the _mNeedToCreateAccountForNetwork state once a account has been created
        // think we may not need this since it's being cleared with clearCreateAccountState anyway
        if (mSharedData.getAccounts() == null) return;
        _mNeedToCreateAccountForNetwork.addSource(accounts, accountInfos -> {
            if (_mNeedToCreateAccountForNetwork.getValue() == null) return;
            for (AccountInfo accountInfo : accountInfos) {
                if (accountInfo.accountId.coin == _mNeedToCreateAccountForNetwork.getValue().coin) {
                    _mNeedToCreateAccountForNetwork.postValue(null);
                    break;
                }
            }
        });
    }

    public void resetServices(
            BraveWalletService braveWalletService, JsonRpcService jsonRpcService) {
        synchronized (mLock) {
            mBraveWalletService = braveWalletService;
            mJsonRpcService = jsonRpcService;
        }
        init();
    }

    public void refreshNetworks() {
        init();
    }

    @SuppressWarnings("NoStreams")
    static void getAllNetworks(
            JsonRpcService jsonRpcService, Callbacks.Callback1<List<NetworkInfo>> callback) {
        if (jsonRpcService == null) {
            callback.call(Collections.emptyList());
            return;
        }
        jsonRpcService.getAllNetworks(
                networks -> {
                    List<NetworkInfo> networkInfoList = new ArrayList<>();
                    networkInfoList.addAll(Arrays.asList(networks));
                    if (!AndroidUtils.isDebugBuild()) {
                        networkInfoList =
                                networkInfoList.stream()
                                        .filter(
                                                networkInfo ->
                                                        !NetworkUtils.Filters.isLocalNetwork(
                                                                networkInfo))
                                        .collect(Collectors.toList());
                    }

                    networkInfoList.sort(NetworkUtils.sSortNetworkByPriority);
                    callback.call(networkInfoList);
                });
    }

    public void init() {
        synchronized (mLock) {
            if (mJsonRpcService == null) {
                return;
            }

            // Mark hidden networks as visible in preferences.
            for (Map.Entry<String, Integer> entry :
                    WalletConstants.KNOWN_TEST_CHAINS_MAP.entrySet()) {
                if (!AndroidUtils.isDebugBuild()
                        && entry.getKey().equals(BraveWalletConstants.LOCALHOST_CHAIN_ID)) {
                    // Hide local host for non-debug builds.
                    mJsonRpcService.addHiddenNetwork(
                            entry.getValue(), entry.getKey(), result -> {/* No-op. */});
                } else {
                    mJsonRpcService.removeHiddenNetwork(
                            entry.getValue(), entry.getKey(), result -> {/* No-op. */});
                }
            }

            getAllNetworks(
                    mJsonRpcService,
                    cryptoNetworks -> {
                        _mCryptoNetworks.postValue(cryptoNetworks);

                        List<NetworkInfo> primary = new ArrayList<>();
                        List<NetworkInfo> secondary = new ArrayList<>();
                        List<NetworkInfo> test = new ArrayList<>();
                        NetworkLists networkLists = new NetworkLists(primary, secondary, test);
                        for (NetworkInfo networkInfo : cryptoNetworks) {
                            if (WalletConstants.SUPPORTED_TOP_LEVEL_CHAIN_IDS.contains(
                                    networkInfo.chainId)) {
                                primary.add(networkInfo);
                            } else if (WalletConstants.KNOWN_TEST_CHAIN_IDS.contains(
                                    networkInfo.chainId)) {
                                test.add(networkInfo);
                            } else {
                                secondary.add(networkInfo);
                            }
                        }
                        _mNetworkLists.postValue(networkLists);
                    });
        }
    }

    public void setNetworkWithAccountCheck(NetworkInfo networkToBeSetAsSelected,
            boolean setNetworkAsDefault, Callbacks.Callback1<Boolean> callback) {
        NetworkInfo selectedNetwork = _mDefaultNetwork.getValue();
        if (isSameNetwork(networkToBeSetAsSelected, selectedNetwork)) return;

        mBraveWalletService.ensureSelectedAccountForChain(
                networkToBeSetAsSelected.coin, networkToBeSetAsSelected.chainId, accountId -> {
                    if (accountId == null) {
                        _mNeedToCreateAccountForNetwork.postValue(networkToBeSetAsSelected);
                        callback.call(false);
                        return;
                    }
                    if (setNetworkAsDefault) {
                        setDefaultNetwork(networkToBeSetAsSelected, callback);
                    } else {
                        setNetworkForSelectedAccountOnActiveOrigin(
                                networkToBeSetAsSelected, callback);
                    }
                });
    }

    public void setNetworkForSelectedAccountOnActiveOrigin(
            NetworkInfo networkToBeSetAsSelected, Callbacks.Callback1<Boolean> callback) {
        mBraveWalletService.setNetworkForSelectedAccountOnActiveOrigin(
                networkToBeSetAsSelected.chainId, success -> {
                    callback.call(success);
                    mCryptoActions.updateCoinType();
                    init();
                });
    }

    public void setDefaultNetwork(
            @NonNull final NetworkInfo networkInfo,
            @NonNull final Callbacks.Callback1<Boolean> callback) {
        mJsonRpcService.setNetwork(
                networkInfo.chainId,
                networkInfo.coin,
                null,
                success -> {
                    callback.call(success);
                    mCryptoActions.updateCoinType();
                    init();
                });
    }

    public void clearCreateAccountState() {
        _mNeedToCreateAccountForNetwork.postValue(null);
    }

    public NetworkInfo getNetwork(String chainId) {
        if (TextUtils.isEmpty(chainId)) return null;
        List<NetworkInfo> cryptoNws = JavaUtils.safeVal(_mCryptoNetworks.getValue());
        for (NetworkInfo info : cryptoNws) {
            if (info.chainId.equals(chainId)) {
                return info;
            }
        }
        return null;
    }

    private boolean isSameNetwork(
            NetworkInfo networkToBeSetAsSelected, NetworkInfo selectedNetwork) {
        return selectedNetwork != null
                && selectedNetwork.chainId.equals(networkToBeSetAsSelected.chainId)
                && selectedNetwork.coin == networkToBeSetAsSelected.coin;
    }

    @Override
    public void chainChangedEvent(String chainId, int coin, Origin origin) {
        updateChainId();
    }

    @Override
    public void onAddEthereumChainRequestCompleted(String chainId, String error) {
        init();
    }

    @Override
    public void onConnectionError(MojoException e) {}

    @Override
    public void close() {}

    public void updateMode(Mode mode) {
        mMode = mode;
        updateChainId();
    }

    public enum Mode {
        PANEL_MODE,
        WALLET_MODE
    }

    public static class NetworkLists {
        public List<NetworkInfo> mPrimaryNetworkList;
        public List<NetworkInfo> mSecondaryNetworkList;
        public List<NetworkInfo> mTestNetworkList;

        public NetworkLists(
                List<NetworkInfo> mPrimaryNetworkList,
                List<NetworkInfo> mSecondaryNetworkList,
                List<NetworkInfo> mTestNetworkList) {
            this.mPrimaryNetworkList = mPrimaryNetworkList;
            this.mSecondaryNetworkList = mSecondaryNetworkList;
            this.mTestNetworkList = mTestNetworkList;
        }
    }
}
