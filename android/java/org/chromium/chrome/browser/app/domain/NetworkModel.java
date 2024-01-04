/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.app.domain;

import android.content.Context;
import android.text.TextUtils;
import android.util.Pair;

import androidx.annotation.NonNull;
import androidx.lifecycle.DefaultLifecycleObserver;
import androidx.lifecycle.Lifecycle;
import androidx.lifecycle.LifecycleOwner;
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
import org.chromium.chrome.browser.crypto_wallet.util.AssetUtils;
import org.chromium.chrome.browser.crypto_wallet.util.JavaUtils;
import org.chromium.chrome.browser.crypto_wallet.util.NetworkResponsesCollector;
import org.chromium.chrome.browser.crypto_wallet.util.NetworkUtils;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;
import org.chromium.chrome.browser.crypto_wallet.util.WalletConstants;
import org.chromium.chrome.browser.crypto_wallet.web_ui.WebUiActivityType;
import org.chromium.chrome.browser.util.Triple;
import org.chromium.mojo.system.MojoException;
import org.chromium.url.internal.mojom.Origin;

import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
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
    private final MediatorLiveData<Triple<String, NetworkInfo, List<NetworkInfo>>>
            _mChainNetworkAllNetwork;
    private final Context mContext;
    private final MediatorLiveData<String[]> _mCustomNetworkIds;
    private final MediatorLiveData<List<NetworkInfo>> _mPrimaryNetworks;
    private final MediatorLiveData<List<NetworkInfo>> _mSecondaryNetworks;
    private final Map<String, NetworkSelectorModel> mNetworkSelectorMap;

    public final LiveData<String> mChainId;
    private final MutableLiveData<NetworkLists> _mNetworkLists;
    public final LiveData<String[]> mCustomNetworkIds;
    public LiveData<NetworkInfo> mNeedToCreateAccountForNetwork;
    public final LiveData<Triple<String, NetworkInfo, List<NetworkInfo>>> mChainNetworkAllNetwork;
    public final LiveData<Pair<String, List<NetworkInfo>>> mPairChainAndNetwork;
    public final LiveData<List<NetworkInfo>> mDefaultCoinCryptoNetworks;
    public final LiveData<List<NetworkInfo>> mCryptoNetworks;
    public final LiveData<NetworkInfo> mDefaultNetwork;
    public final LiveData<List<NetworkInfo>> mPrimaryNetworks;
    public final LiveData<List<NetworkInfo>> mSecondaryNetworks;
    public final LiveData<NetworkLists> mNetworkLists;

    public NetworkModel(BraveWalletService braveWalletService, JsonRpcService jsonRpcService,
            CryptoSharedData sharedData, CryptoSharedActions cryptoSharedActions, Context context) {
        mBraveWalletService = braveWalletService;
        mJsonRpcService = jsonRpcService;
        mSharedData = sharedData;
        mCryptoActions = cryptoSharedActions;
        mContext = context;

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
        _mChainNetworkAllNetwork = new MediatorLiveData<>();
        mChainNetworkAllNetwork = _mChainNetworkAllNetwork;
        _mCustomNetworkIds = new MediatorLiveData<>();
        _mCustomNetworkIds.postValue(new String[0]);
        mCustomNetworkIds = _mCustomNetworkIds;
        _mPrimaryNetworks = new MediatorLiveData<>();
        mPrimaryNetworks = _mPrimaryNetworks;
        _mSecondaryNetworks = new MediatorLiveData<>();
        mSecondaryNetworks = _mSecondaryNetworks;
        jsonRpcService.addObserver(this);
        mNetworkSelectorMap = new HashMap<>();
        _mNetworkLists = new MutableLiveData<>();
        mNetworkLists = _mNetworkLists;
        _mPairChainAndNetwork.setValue(Pair.create("", Collections.emptyList()));
        _mPairChainAndNetwork.addSource(_mChainId, chainId -> {
            _mPairChainAndNetwork.setValue(
                    Pair.create(chainId, _mDefaultCoinCryptoNetworks.getValue()));
        });
        _mPairChainAndNetwork.addSource(_mDefaultCoinCryptoNetworks, networks -> {
            _mPairChainAndNetwork.setValue(Pair.create(_mChainId.getValue(), networks));
        });
        _mDefaultNetwork.addSource(_mPairChainAndNetwork, chainIdAndInfosPair -> {
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
        _mDefaultCoinCryptoNetworks.addSource(mSharedData.getCoinTypeLd(), coinType -> {
            getAllNetworks(mJsonRpcService, Collections.singletonList(coinType), networkInfoSet -> {
                _mDefaultCoinCryptoNetworks.postValue(new ArrayList<>(networkInfoSet));
            });
        });

        _mChainNetworkAllNetwork.addSource(_mDefaultNetwork, networkInfo -> {
            NetworkInfo currNetwork = null;
            if (_mChainNetworkAllNetwork.getValue() != null) {
                currNetwork = _mChainNetworkAllNetwork.getValue().second;
            }
            if (currNetwork != null && networkInfo != null
                    && NetworkUtils.Filters.isSameNetwork(currNetwork, networkInfo)) {
                return;
            }
            _mChainNetworkAllNetwork.postValue(
                    Triple.create(networkInfo.chainId, networkInfo, _mCryptoNetworks.getValue()));
        });
        _mChainNetworkAllNetwork.addSource(_mCryptoNetworks, networkInfos -> {
            String chainId = null;
            NetworkInfo networkInfo = _mDefaultNetwork.getValue();
            if (networkInfo != null) {
                chainId = networkInfo.chainId;
            }
            _mChainNetworkAllNetwork.postValue(Triple.create(chainId, networkInfo, networkInfos));
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

    /**
     * Create a network model to handle default wallet network selector events
     * @return mNetworkSelectorModel object in DEFAULT_WALLET_NETWORK mode
     */
    public NetworkSelectorModel openNetworkSelectorModel() {
        return new NetworkSelectorModel(this, mContext);
    }

    /**
     * Create a network selector model to handle either default or local state (onscreen e.g. {@link
     * org.chromium.chrome.browser.crypto_wallet.fragments.PortfolioFragment}). Local network
     * selection can be used by many views so make sure to use the same key which acts as a contract
     * between the view and the selection activity.
     * @param key acts as a binding key between caller and selection activity.
     * @param mode to handle network selection event globally or locally.
     * @param selectionMode to allow Single or Multiple network selection.
     * @param lifecycle to auto remove network-selection objects.
     * @return NetworkSelectorModel object.
     */
    public NetworkSelectorModel openNetworkSelectorModel(String key, NetworkSelectorModel.Mode mode,
            NetworkSelectorModel.SelectionMode selectionMode, Lifecycle lifecycle) {
        NetworkSelectorModel networkSelectorModel;
        if (key == null) {
            return new NetworkSelectorModel(mode, selectionMode, this, mContext);
        } else if (mNetworkSelectorMap.containsKey(key)) {
            // Use existing NetworkSelector object to show the previously selected network
            networkSelectorModel = mNetworkSelectorMap.get(key);
            if (networkSelectorModel != null && mode != networkSelectorModel.getMode()) {
                networkSelectorModel.updateSelectorMode(mode);
            }
        } else {
            networkSelectorModel = new NetworkSelectorModel(mode, selectionMode, this, mContext);
            mNetworkSelectorMap.put(key, networkSelectorModel);
        }
        if (lifecycle != null) {
            lifecycle.addObserver(new DefaultLifecycleObserver() {
                @Override
                public void onDestroy(@NonNull LifecycleOwner owner) {
                    mNetworkSelectorMap.remove(key);
                }
            });
        }
        return networkSelectorModel;
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

    static void getAllNetworks(JsonRpcService jsonRpcService, List<Integer> supportedCoins,
            Callbacks.Callback1<List<NetworkInfo>> callback) {
        if (jsonRpcService == null) {
            callback.call(Collections.emptyList());
            return;
        }

        NetworkResponsesCollector networkResponsesCollector =
                new NetworkResponsesCollector(jsonRpcService, supportedCoins);
        networkResponsesCollector.getNetworks(networkInfoList -> {
            if (!AndroidUtils.isDebugBuild()) {
                networkInfoList =
                        networkInfoList.stream()
                                .filter(networkInfo
                                        -> !NetworkUtils.Filters.isLocalNetwork(networkInfo))
                                .collect(Collectors.toList());
            }
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
                    mJsonRpcService, mSharedData.getSupportedCryptoCoins(), cryptoNetworks -> {
                        _mCryptoNetworks.postValue(cryptoNetworks);

                        List<NetworkInfo> primary = new ArrayList<>();
                        List<NetworkInfo> secondary = new ArrayList<>();
                        List<NetworkInfo> test = new ArrayList<>();
                        NetworkLists networkLists =
                                new NetworkLists(cryptoNetworks, primary, secondary, test);
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

    public void setDefaultNetwork(NetworkInfo networkInfo, Callbacks.Callback1<Boolean> callback) {
        mJsonRpcService.setNetwork(networkInfo.chainId, networkInfo.coin, null, success -> {
            callback.call(success);
            mCryptoActions.updateCoinType();
            init();
        });
    }

    public void clearCreateAccountState() {
        _mNeedToCreateAccountForNetwork.postValue(null);
    }

    public List<NetworkInfo> stripNoBuyNetworks(
            List<NetworkInfo> networkInfos, WebUiActivityType type) {
        List<NetworkInfo> networkInfosFiltered = new ArrayList<>();
        for (NetworkInfo networkInfo : networkInfos) {
            if (type == WebUiActivityType.BUY && Utils.allowBuy(networkInfo.chainId)) {
                networkInfosFiltered.add(networkInfo);
            }
        }

        return networkInfosFiltered;
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

    private boolean isCustomChain(String netWorkChainId) {
        String[] customNetworkChains = JavaUtils.safeVal(_mCustomNetworkIds.getValue());
        for (String chain : customNetworkChains) {
            if (chain.equals(netWorkChainId)) {
                return true;
            }
        }
        return false;
    }

    boolean hasAccountOfNetworkType(NetworkInfo networkToBeSetAsSelected) {
        List<AccountInfo> accountInfos = mSharedData.getAccounts().getValue();
        for (AccountInfo accountInfo : accountInfos) {
            if (accountInfo.accountId.coin == networkToBeSetAsSelected.coin) {
                if (accountInfo.accountId.coin != CoinType.FIL
                        || AssetUtils.getKeyring(CoinType.FIL, networkToBeSetAsSelected.chainId)
                                == accountInfo.accountId.keyringId) {
                    return true;
                }
            }
        }
        return false;
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
    public void onIsEip1559Changed(String chainId, boolean isEip1559) {}

    @Override
    public void onConnectionError(MojoException e) {}

    @Override
    public void close() {}

    public void updateMode(Mode mode) {
        mMode = mode;
        updateChainId();
    }

    public enum Mode { PANEL_MODE, WALLET_MODE }

    public static class NetworkLists {
        // Networks from core.
        public List<NetworkInfo> mCoreNetworks;
        public List<NetworkInfo> mPrimaryNetworkList;
        public List<NetworkInfo> mSecondaryNetworkList;
        public List<NetworkInfo> mTestNetworkList;

        public NetworkLists() {
            mCoreNetworks = Collections.emptyList();
            mPrimaryNetworkList = Collections.emptyList();
            mSecondaryNetworkList = Collections.emptyList();
            mTestNetworkList = Collections.emptyList();
        }
        public NetworkLists(List<NetworkInfo> mCoreNetworks, List<NetworkInfo> mPrimaryNetworkList,
                List<NetworkInfo> mSecondaryNetworkList, List<NetworkInfo> mTestNetworkList) {
            this.mCoreNetworks = mCoreNetworks;
            this.mPrimaryNetworkList = mPrimaryNetworkList;
            this.mSecondaryNetworkList = mSecondaryNetworkList;
            this.mTestNetworkList = mTestNetworkList;
        }

        public NetworkLists(NetworkLists networkLists) {
            this.mCoreNetworks = new ArrayList<>(networkLists.mCoreNetworks);
            this.mPrimaryNetworkList = new ArrayList<>(networkLists.mPrimaryNetworkList);
            this.mSecondaryNetworkList = new ArrayList<>(networkLists.mSecondaryNetworkList);
            this.mTestNetworkList = new ArrayList<>(networkLists.mTestNetworkList);
        }
    }
}
