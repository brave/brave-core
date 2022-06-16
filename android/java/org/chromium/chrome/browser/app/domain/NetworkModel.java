package org.chromium.chrome.browser.app.domain;

import android.text.TextUtils;

import androidx.lifecycle.LiveData;
import androidx.lifecycle.MediatorLiveData;
import androidx.lifecycle.MutableLiveData;

import org.chromium.brave_wallet.mojom.BraveWalletConstants;
import org.chromium.brave_wallet.mojom.JsonRpcService;
import org.chromium.brave_wallet.mojom.JsonRpcServiceObserver;
import org.chromium.brave_wallet.mojom.NetworkInfo;
import org.chromium.mojo.system.MojoException;
import org.chromium.mojo.system.Pair;

public class NetworkModel implements JsonRpcServiceObserver {
    private JsonRpcService mJsonRpcService;
    private final MutableLiveData<String> _mChainId;
    private final MutableLiveData<NetworkInfo[]> _mCryptoNetworks;
    private final CryptoSharedData mSharedData;
    public final LiveData<String> mChainId;
    public final LiveData<NetworkInfo[]> mCryptoNetworks;
    public final MediatorLiveData<Pair<String, NetworkInfo[]>> mPairChainAndNetwork =
            new MediatorLiveData<>();

    public NetworkModel(JsonRpcService jsonRpcService, CryptoSharedData sharedData) {
        mJsonRpcService = jsonRpcService;
        mSharedData = sharedData;
        _mChainId = new MutableLiveData<>(BraveWalletConstants.MAINNET_CHAIN_ID);
        _mCryptoNetworks = new MutableLiveData<>();
        mChainId = _mChainId;
        mCryptoNetworks = _mCryptoNetworks;
        jsonRpcService.addObserver(this);
        mPairChainAndNetwork.setValue(Pair.create("", new NetworkInfo[] {}));
        mPairChainAndNetwork.addSource(_mChainId, chainId -> {
            mPairChainAndNetwork.setValue(Pair.create(chainId, _mCryptoNetworks.getValue()));
        });
        mPairChainAndNetwork.addSource(_mCryptoNetworks, networks -> {
            mPairChainAndNetwork.setValue(Pair.create(_mChainId.getValue(), networks));
        });
    }

    public void resetServices(JsonRpcService jsonRpcService) {
        mJsonRpcService = jsonRpcService;
        init();
    }

    public void init() {
        mJsonRpcService.getChainId(mSharedData.getCoinType(), chainId -> {
            String id = BraveWalletConstants.MAINNET_CHAIN_ID;
            if (TextUtils.isEmpty(chainId)) {
                mJsonRpcService.setNetwork(id, mSharedData.getCoinType(), hasSetNetwork -> {});
            } else {
                id = chainId;
            }
            _mChainId.postValue(id);
        });
        mJsonRpcService.getAllNetworks(mSharedData.getCoinType(),
                networkInfos -> { _mCryptoNetworks.postValue(networkInfos); });
    }

    @Override
    public void chainChangedEvent(String chainId, int coin) {
        _mChainId.postValue(chainId);
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
