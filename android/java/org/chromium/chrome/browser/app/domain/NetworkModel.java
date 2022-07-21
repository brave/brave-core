package org.chromium.chrome.browser.app.domain;

import android.content.Context;
import android.content.pm.ApplicationInfo;
import android.text.TextUtils;

import androidx.lifecycle.LiveData;
import androidx.lifecycle.MediatorLiveData;
import androidx.lifecycle.MutableLiveData;

import org.chromium.brave_wallet.mojom.AccountInfo;
import org.chromium.brave_wallet.mojom.BraveWalletConstants;
import org.chromium.brave_wallet.mojom.JsonRpcService;
import org.chromium.brave_wallet.mojom.JsonRpcServiceObserver;
import org.chromium.brave_wallet.mojom.NetworkInfo;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.crypto_wallet.model.CryptoAccountTypeInfo;
import org.chromium.chrome.browser.crypto_wallet.util.NetworkResponsesCollector;
import org.chromium.mojo.bindings.Callbacks;
import org.chromium.mojo.system.MojoException;
import org.chromium.mojo.system.Pair;

import java.util.ArrayList;
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
    private MediatorLiveData<NetworkInfo> _mNeedToCreateAccountForNetwork;
    private final MediatorLiveData<NetworkInfo> _mDefaultNetwork;
    private final Context mContext;
    public LiveData<NetworkInfo> mNeedToCreateAccountForNetwork;
    public final LiveData<Pair<String, NetworkInfo[]>> mPairChainAndNetwork;
    public final LiveData<String> mChainId;
    public final LiveData<NetworkInfo[]> mDefaultCoinCryptoNetworks;
    public final LiveData<NetworkInfo[]> mCryptoNetworks;
    public final LiveData<NetworkInfo> mDefaultNetwork;

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
        jsonRpcService.addObserver(this);
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
    }

    public void setAccountInfosFromKeyRingModel(
            LiveData<List<AccountInfo>> accountInfosFromKeyRingModel) {
        setUpAccountObserver(accountInfosFromKeyRingModel);
    }

    void setUpAccountObserver(LiveData<List<AccountInfo>> accounts) {
        // getAccounts can be null as it's being set via a setter
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

    public void setNetwork(
            NetworkInfo networkToBeSetAsSelected, Callbacks.Callback1<Boolean> callback) {
        List<AccountInfo> accountInfos = mSharedData.getAccounts().getValue();
        boolean hasAccountOfNetworkType = false;
        for (AccountInfo accountInfo : accountInfos) {
            hasAccountOfNetworkType = accountInfo.coin == networkToBeSetAsSelected.coin;
            if (hasAccountOfNetworkType) {
                break;
            }
        }
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

    public void clearCreateAccountState() {
        _mNeedToCreateAccountForNetwork.postValue(null);
    }

    private NetworkInfo[] stripDebugNetwork(Context context, NetworkInfo[] networkInfos) {
        if ((context.getApplicationInfo().flags & ApplicationInfo.FLAG_DEBUGGABLE) != 0) {
            return networkInfos;
        } else {
            String localHost = context.getString(R.string.localhost);
            List<NetworkInfo> networkInfosFiltered = new ArrayList<>();
            // strip localhost network
            for (NetworkInfo networkInfo : networkInfos) {
                if (!networkInfo.chainName.equals(localHost)) {
                    networkInfosFiltered.add(networkInfo);
                }
            }
            return networkInfosFiltered.toArray(new NetworkInfo[networkInfosFiltered.size()]);
        }
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
