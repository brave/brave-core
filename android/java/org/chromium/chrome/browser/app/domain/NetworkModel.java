/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.app.domain;

import android.content.Context;
import android.content.pm.ApplicationInfo;
import android.text.TextUtils;
import android.util.Pair;

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
import org.chromium.chrome.browser.crypto_wallet.model.CryptoAccountTypeInfo;
import org.chromium.chrome.browser.crypto_wallet.util.JavaUtils;
import org.chromium.chrome.browser.crypto_wallet.util.NetworkResponsesCollector;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;
import org.chromium.chrome.browser.crypto_wallet.util.WalletConstants;
import org.chromium.chrome.browser.util.Triple;
import org.chromium.mojo.bindings.Callbacks;
import org.chromium.mojo.system.MojoException;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

public class NetworkModel implements JsonRpcServiceObserver {
    private JsonRpcService mJsonRpcService;
    private final Object mLock = new Object();
    private final MediatorLiveData<String> _mChainId;
    private final MediatorLiveData<NetworkInfo[]> _mDefaultCoinCryptoNetworks;
    private final MutableLiveData<NetworkInfo[]> _mCryptoNetworks;
    private final CryptoSharedData mSharedData;
    private final CryptoSharedActions mCryptoActions;
    private final MediatorLiveData<Pair<String, NetworkInfo[]>> _mPairChainAndNetwork;
    private final MediatorLiveData<NetworkInfo> _mNeedToCreateAccountForNetwork;
    private final MediatorLiveData<NetworkInfo> _mDefaultNetwork;
    private final MediatorLiveData<Triple<String, NetworkInfo, NetworkInfo[]>>
            _mChainNetworkAllNetwork;
    private final Context mContext;
    private final MediatorLiveData<String[]> _mCustomNetworkIds;
    private final MediatorLiveData<List<NetworkInfo>> _mPrimaryNetworks;
    private final MediatorLiveData<List<NetworkInfo>> _mSecondaryNetworks;
    private NetworkSelectorModel mNetworkSelectorModel;
    public final LiveData<String[]> mCustomNetworkIds;
    public LiveData<NetworkInfo> mNeedToCreateAccountForNetwork;
    public final LiveData<Triple<String, NetworkInfo, NetworkInfo[]>> mChainNetworkAllNetwork;
    public final LiveData<Pair<String, NetworkInfo[]>> mPairChainAndNetwork;
    public final LiveData<String> mChainId;
    public final LiveData<NetworkInfo[]> mDefaultCoinCryptoNetworks;
    public final LiveData<NetworkInfo[]> mCryptoNetworks;
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
        _mCryptoNetworks = new MutableLiveData<>(new NetworkInfo[0]);
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
        mNetworkSelectorModel = new NetworkSelectorModel(this, mContext);
        _mPairChainAndNetwork.setValue(Pair.create("", new NetworkInfo[] {}));
        _mPairChainAndNetwork.addSource(_mChainId, chainId -> {
            _mPairChainAndNetwork.setValue(
                    Pair.create(chainId, _mDefaultCoinCryptoNetworks.getValue()));
        });
        _mPairChainAndNetwork.addSource(_mDefaultCoinCryptoNetworks, networks -> {
            _mPairChainAndNetwork.setValue(Pair.create(_mChainId.getValue(), networks));
        });
        _mDefaultNetwork.addSource(_mPairChainAndNetwork, chainIdAndInfosPair -> {
            String chainId = chainIdAndInfosPair.first;
            NetworkInfo[] cryptoNetworks = chainIdAndInfosPair.second;
            if (chainId == null || cryptoNetworks == null) return;
            for (NetworkInfo networkInfo : cryptoNetworks) {
                if (networkInfo.chainId.equals(chainId)
                        && sharedData.getCoinType() == networkInfo.coin) {
                    _mDefaultNetwork.postValue(networkInfo);
                    break;
                }
            }
        });
        _mChainId.addSource(mSharedData.getCoinTypeLd(), coinType -> {
            mJsonRpcService.getChainId(coinType, chainId -> {
                String id = BraveWalletConstants.MAINNET_CHAIN_ID;
                if (TextUtils.isEmpty(chainId)) {
                    mJsonRpcService.setNetwork(id, mSharedData.getCoinType(), hasSetNetwork -> {});
                } else {
                    id = chainId;
                }
                _mChainId.postValue(id);
            });
        });
        _mDefaultCoinCryptoNetworks.addSource(mSharedData.getCoinTypeLd(), coinType -> {
            mJsonRpcService.getAllNetworks(coinType, networkInfos -> {
                _mDefaultCoinCryptoNetworks.postValue(stripDebugNetwork(context, networkInfos));
            });
        });

        _mChainNetworkAllNetwork.addSource(_mDefaultNetwork, networkInfo -> {
            _mChainNetworkAllNetwork.postValue(
                    Triple.create(networkInfo.chainId, networkInfo, _mCryptoNetworks.getValue()));
        });
        _mChainNetworkAllNetwork.addSource(_mCryptoNetworks, networkInfos -> {
            _mChainNetworkAllNetwork.postValue(
                    Triple.create(_mChainId.getValue(), _mDefaultNetwork.getValue(), networkInfos));
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
        return openNetworkSelectorModel(NetworkSelectorModel.Mode.DEFAULT_WALLET_NETWORK);
    }

    public NetworkSelectorModel openNetworkSelectorModel(NetworkSelectorModel.Mode mode) {
        mNetworkSelectorModel.updateSelectorMode(mode);
        return mNetworkSelectorModel;
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
                if (accountInfo.coin == _mNeedToCreateAccountForNetwork.getValue().coin) {
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

    public void init() {
        synchronized (mLock) {
            if (mJsonRpcService == null) {
                return;
            }

            List<Integer> coins = new ArrayList<>();
            for (CryptoAccountTypeInfo cryptoAccountTypeInfo :
                    mSharedData.getSupportedCryptoAccountTypes()) {
                coins.add(cryptoAccountTypeInfo.getCoinType());
            }
            NetworkResponsesCollector networkResponsesCollector =
                    new NetworkResponsesCollector(mJsonRpcService, coins);
            networkResponsesCollector.getNetworks(networkInfos -> {
                _mCryptoNetworks.postValue(
                        stripDebugNetwork(mContext, networkInfos.toArray(new NetworkInfo[0])));
            });
        }
    }

    public void setNetworkWithAccountCheck(
            NetworkInfo networkToBeSetAsSelected, Callbacks.Callback1<Boolean> callback) {
        NetworkInfo selectedNetwork = _mDefaultNetwork.getValue();
        if (isSameNetwork(networkToBeSetAsSelected, selectedNetwork)) return;
        boolean hasAccountOfNetworkType = hasAccountOfNetworkType(networkToBeSetAsSelected);
        if (hasAccountOfNetworkType) {
            mJsonRpcService.setNetwork(
                    networkToBeSetAsSelected.chainId, networkToBeSetAsSelected.coin, isSelected -> {
                        callback.call(isSelected);
                        mCryptoActions.updateCoinType();
                        init();
                    });
        } else {
            _mNeedToCreateAccountForNetwork.postValue(networkToBeSetAsSelected);
            callback.call(false);
        }
    }

    public void setNetwork(
            NetworkInfo networkToBeSetAsSelected, Callbacks.Callback1<Boolean> callback) {
        mJsonRpcService.setNetwork(
                networkToBeSetAsSelected.chainId, networkToBeSetAsSelected.coin, isSelected -> {
                    callback.call(isSelected);
                    mCryptoActions.updateCoinType();
                    init();
                });
    }

    public void getNetwork(@CoinType.EnumType int coin, Callbacks.Callback1<NetworkInfo> callback) {
        mJsonRpcService.getNetwork(coin, networkInfo -> { callback.call(networkInfo); });
    }

    public void clearCreateAccountState() {
        _mNeedToCreateAccountForNetwork.postValue(null);
    }

    public NetworkInfo[] stripNoBuySwapNetworks(
            NetworkInfo[] networkInfos, BuySendSwapActivity.ActivityType type) {
        List<NetworkInfo> networkInfosFiltered = new ArrayList<>();
        for (NetworkInfo networkInfo : networkInfos) {
            if (type == BuySendSwapActivity.ActivityType.BUY && Utils.allowBuy(networkInfo.chainId)
                    || (type == BuySendSwapActivity.ActivityType.SWAP
                            && Utils.allowSwap(networkInfo.chainId))) {
                networkInfosFiltered.add(networkInfo);
            }
        }

        return networkInfosFiltered.toArray(new NetworkInfo[networkInfosFiltered.size()]);
    }

    private NetworkInfo[] stripDebugNetwork(Context context, NetworkInfo[] networkInfos) {
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
            return networkInfosFiltered.toArray(new NetworkInfo[0]);
        }
    }

    List<NetworkInfo> getSubTestNetworks(NetworkInfo networkInfo) {
        NetworkInfo[] cryptoNws = _mCryptoNetworks.getValue();
        if (cryptoNws == null || cryptoNws.length == 0
                || !WalletConstants.SUPPORTED_TOP_LEVEL_CHAIN_IDS.contains(networkInfo.chainId))
            return Collections.emptyList();
        List<NetworkInfo> list = new ArrayList<>();
        for (NetworkInfo info : cryptoNws) {
            if (WalletConstants.KNOWN_TEST_CHAIN_IDS.contains(info.chainId)
                    && networkInfo.coin == info.coin && networkInfo.symbol.equals(info.symbol)
                    && !isCustomChain(info.chainId)) {
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
        boolean hasAccountOfNetworkType = false;
        for (AccountInfo accountInfo : accountInfos) {
            hasAccountOfNetworkType = accountInfo.coin == networkToBeSetAsSelected.coin;
            if (hasAccountOfNetworkType) {
                break;
            }
        }
        return hasAccountOfNetworkType;
    }

    private boolean isSameNetwork(
            NetworkInfo networkToBeSetAsSelected, NetworkInfo selectedNetwork) {
        return selectedNetwork != null
                && selectedNetwork.chainId.equals(networkToBeSetAsSelected.chainId)
                && selectedNetwork.coin == networkToBeSetAsSelected.coin;
    }

    @Override
    public void chainChangedEvent(String chainId, int coin) {
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
