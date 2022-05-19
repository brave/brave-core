package org.chromium.chrome.browser.app.domain;

import android.text.TextUtils;

import androidx.lifecycle.LiveData;
import androidx.lifecycle.MutableLiveData;

import org.chromium.brave_wallet.mojom.BraveWalletConstants;
import org.chromium.brave_wallet.mojom.JsonRpcService;
import org.chromium.brave_wallet.mojom.JsonRpcServiceObserver;
import org.chromium.mojo.system.MojoException;

public class NetworkModel implements JsonRpcServiceObserver {
    private final JsonRpcService mJsonRpcService;
    private final MutableLiveData<String> _mChainId;
    public LiveData<String> mChainId;
    private final CryptoSharedData mSharedData;

    public NetworkModel(JsonRpcService jsonRpcService, CryptoSharedData sharedData) {
        mJsonRpcService = jsonRpcService;
        mSharedData = sharedData;
        _mChainId = new MutableLiveData<>(BraveWalletConstants.MAINNET_CHAIN_ID);
        mChainId = _mChainId;
        jsonRpcService.addObserver(this);
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
    }

    @Override
    public void chainChangedEvent(String chainId, int coin) {
        _mChainId.postValue(chainId);
    }

    @Override
    public void onAddEthereumChainRequestCompleted(String chainId, String error) {}

    @Override
    public void onIsEip1559Changed(String chainId, boolean isEip1559) {}

    @Override
    public void onConnectionError(MojoException e) {}

    @Override
    public void close() {}
}
