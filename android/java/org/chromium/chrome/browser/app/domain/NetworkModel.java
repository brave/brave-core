/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.app.domain;

import android.content.Context;
import android.content.pm.ApplicationInfo;
import android.text.TextUtils;
import android.util.Pair;

import androidx.annotation.NonNull;
import androidx.lifecycle.DefaultLifecycleObserver;
import androidx.lifecycle.Lifecycle;
import androidx.lifecycle.LifecycleOwner;
import androidx.lifecycle.LiveData;
import androidx.lifecycle.MediatorLiveData;
import androidx.lifecycle.MutableLiveData;

import org.chromium.brave_wallet.mojom.AccountInfo;
import org.chromium.brave_wallet.mojom.BraveWalletConstants;
import org.chromium.brave_wallet.mojom.CoinType;
import org.chromium.brave_wallet.mojom.JsonRpcService;
import org.chromium.brave_wallet.mojom.JsonRpcServiceObserver;
import org.chromium.brave_wallet.mojom.NetworkInfo;
import org.chromium.chrome.browser.crypto_wallet.activities.BuySendSwapActivity;
import org.chromium.chrome.browser.crypto_wallet.util.JavaUtils;
import org.chromium.chrome.browser.crypto_wallet.util.NetworkResponsesCollector;
import org.chromium.chrome.browser.crypto_wallet.util.NetworkUtils;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;
import org.chromium.chrome.browser.crypto_wallet.util.WalletConstants;
import org.chromium.chrome.browser.util.Triple;
import org.chromium.mojo.bindings.Callbacks;
import org.chromium.mojo.system.MojoException;
import org.chromium.url.internal.mojom.Origin;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Set;

public class NetworkModel implements JsonRpcServiceObserver {
    private JsonRpcService mJsonRpcService;
    private final Object mLock = new Object();
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
    private Map<String, NetworkSelectorModel> mNetworkSelectorMap;
    public final LiveData<String[]> mCustomNetworkIds;
    public LiveData<NetworkInfo> mNeedToCreateAccountForNetwork;
    public final LiveData<Triple<String, NetworkInfo, List<NetworkInfo>>> mChainNetworkAllNetwork;
    public final LiveData<Pair<String, List<NetworkInfo>>> mPairChainAndNetwork;
    public final LiveData<String> mChainId;
    public final LiveData<List<NetworkInfo>> mDefaultCoinCryptoNetworks;
    public final LiveData<List<NetworkInfo>> mCryptoNetworks;
    public final LiveData<NetworkInfo> mDefaultNetwork;
    public final LiveData<List<NetworkInfo>> mPrimaryNetworks;
    public final LiveData<List<NetworkInfo>> mSecondaryNetworks;

    public NetworkModel(JsonRpcService jsonRpcService, CryptoSharedData sharedData,
            CryptoSharedActions cryptoSharedActions, Context context) {
        mJsonRpcService = jsonRpcService;
        mSharedData = sharedData;
        mCryptoActions = cryptoSharedActions;
        mContext = context;
        _mChainId = new MediatorLiveData<>();
        _mChainId.setValue(BraveWalletConstants.MAINNET_CHAIN_ID);
        _mDefaultCoinCryptoNetworks = new MediatorLiveData<>();
        mChainId = _mChainId;
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
        _mChainId.addSource(mSharedData.getCoinTypeLd(), coinType -> {
            mJsonRpcService.getDefaultChainId(coinType, chainId -> {
                String id = BraveWalletConstants.MAINNET_CHAIN_ID;
                if (TextUtils.isEmpty(chainId)) {
                    mJsonRpcService.setNetwork(
                            id, mSharedData.getCoinType(), null, hasSetNetwork -> {});
                } else {
                    id = chainId;
                }
                _mChainId.postValue(id);
            });
        });
        _mDefaultCoinCryptoNetworks.addSource(mSharedData.getCoinTypeLd(), coinType -> {
            mJsonRpcService.getAllNetworks(coinType, networkInfos -> {
                _mDefaultCoinCryptoNetworks.postValue(
                        stripDebugNetwork(context, Arrays.asList(networkInfos)));
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
        _mCustomNetworkIds.addSource(mSharedData.getCoinTypeLd(), coinType -> {
            mJsonRpcService.getCustomNetworks(
                    coinType, customNetworks -> { _mCustomNetworkIds.postValue(customNetworks); });
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
     * @param lifecycle to auto remove network-selection objects.
     * @return NetworkSelectorModel object.
     */
    public NetworkSelectorModel openNetworkSelectorModel(
            String key, NetworkSelectorModel.Mode mode, Lifecycle lifecycle) {
        NetworkSelectorModel networkSelectorModel;
        if (key == null) {
            return new NetworkSelectorModel(mode, this, mContext);
        } else if (mNetworkSelectorMap.containsKey(key)) {
            // Use existing NetworkSelector object to show the previously selected network
            networkSelectorModel = mNetworkSelectorMap.get(key);
            if (networkSelectorModel != null && mode != networkSelectorModel.getMode()) {
                networkSelectorModel.updateSelectorMode(mode);
            }
        } else {
            networkSelectorModel = new NetworkSelectorModel(mode, this, mContext);
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

    public void resetServices(JsonRpcService jsonRpcService) {
        synchronized (mLock) {
            mJsonRpcService = jsonRpcService;
        }
        init();
    }

    public void refreshNetworks() {
        init();
    }

    static void getAllNetworks(JsonRpcService jsonRpcService, List<Integer> supportedCoins,
            Callbacks.Callback1<Set<NetworkInfo>> callback) {
        if (jsonRpcService == null) {
            callback.call(Collections.emptySet());
            return;
        }

        NetworkResponsesCollector networkResponsesCollector =
                new NetworkResponsesCollector(jsonRpcService, supportedCoins);
        networkResponsesCollector.getNetworks(networkInfos -> { callback.call(networkInfos); });
    }

    public void init() {
        synchronized (mLock) {
            if (mJsonRpcService == null) {
                return;
            }

            getAllNetworks(mJsonRpcService, mSharedData.getSupportedCryptoCoins(), allNetworks -> {
                _mCryptoNetworks.postValue(
                        stripDebugNetwork(mContext, new ArrayList<>(allNetworks)));
            });
        }
    }

    public void setNetworkWithAccountCheck(
            NetworkInfo networkToBeSetAsSelected, Callbacks.Callback1<Boolean> callback) {
        NetworkInfo selectedNetwork = _mDefaultNetwork.getValue();
        if (isSameNetwork(networkToBeSetAsSelected, selectedNetwork)) return;
        boolean hasAccountOfNetworkType = hasAccountOfNetworkType(networkToBeSetAsSelected);
        if (hasAccountOfNetworkType) {
            mJsonRpcService.setNetwork(networkToBeSetAsSelected.chainId,
                    networkToBeSetAsSelected.coin, null, isSelected -> {
                        callback.call(isSelected);
                        mCryptoActions.updateCoinType();
                        init();
                    });
        } else {
            _mNeedToCreateAccountForNetwork.postValue(networkToBeSetAsSelected);
            callback.call(false);
        }
    }

    public void setNetworkWithAccountCheck(String chainId, Callbacks.Callback1<Boolean> callback) {
        NetworkInfo networkToBeSetAsSelected = getNetwork(chainId);
        setNetworkWithAccountCheck(networkToBeSetAsSelected, callback);
    }

    public void setNetwork(
            NetworkInfo networkToBeSetAsSelected, Callbacks.Callback1<Boolean> callback) {
        mJsonRpcService.setNetwork(networkToBeSetAsSelected.chainId, networkToBeSetAsSelected.coin,
                null, isSelected -> {
                    callback.call(isSelected);
                    mCryptoActions.updateCoinType();
                    init();
                });
    }

    public void getNetwork(@CoinType.EnumType int coin, Callbacks.Callback1<NetworkInfo> callback) {
        mJsonRpcService.getNetwork(coin, null, networkInfo -> { callback.call(networkInfo); });
    }

    public void clearCreateAccountState() {
        _mNeedToCreateAccountForNetwork.postValue(null);
    }

    public List<NetworkInfo> stripNoBuyNetworks(
            List<NetworkInfo> networkInfos, BuySendSwapActivity.ActivityType type) {
        List<NetworkInfo> networkInfosFiltered = new ArrayList<>();
        for (NetworkInfo networkInfo : networkInfos) {
            if (type == BuySendSwapActivity.ActivityType.BUY
                    && Utils.allowBuy(networkInfo.chainId)) {
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

    private List<NetworkInfo> stripDebugNetwork(Context context, List<NetworkInfo> networkInfos) {
        if ((context.getApplicationInfo().flags & ApplicationInfo.FLAG_DEBUGGABLE) != 0) {
            return networkInfos;
        } else {
            List<NetworkInfo> networkInfosFiltered = new ArrayList<>();
            // remove localhost network
            for (NetworkInfo networkInfo : networkInfos) {
                if (!networkInfo.chainId.equals(BraveWalletConstants.LOCALHOST_CHAIN_ID)) {
                    networkInfosFiltered.add(networkInfo);
                }
            }
            return networkInfosFiltered;
        }
    }

    List<NetworkInfo> getSubTestNetworks(NetworkInfo networkInfo) {
        List<NetworkInfo> cryptoNws = _mCryptoNetworks.getValue();
        if (cryptoNws == null || cryptoNws.size() == 0
                || !WalletConstants.SUPPORTED_TOP_LEVEL_CHAIN_IDS.contains(networkInfo.chainId))
            return Collections.emptyList();
        List<NetworkInfo> list = new ArrayList<>();
        for (NetworkInfo info : cryptoNws) {
            if (WalletConstants.KNOWN_TEST_CHAIN_IDS.contains(info.chainId)
                    && networkInfo.coin == info.coin && !isCustomChain(info.chainId)) {
                list.add(info);
            }
        }
        return list;
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
                return true;
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
        _mChainId.postValue(chainId);
        mCryptoActions.updateCoinAccountNetworkInfo(coin);
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
}
