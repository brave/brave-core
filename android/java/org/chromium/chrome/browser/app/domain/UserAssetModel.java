/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.app.domain;

import androidx.lifecycle.LiveData;
import androidx.lifecycle.MutableLiveData;

import org.chromium.brave_wallet.mojom.BlockchainRegistry;
import org.chromium.brave_wallet.mojom.BlockchainToken;
import org.chromium.brave_wallet.mojom.BraveWalletService;
import org.chromium.brave_wallet.mojom.JsonRpcService;
import org.chromium.brave_wallet.mojom.NetworkInfo;
import org.chromium.chrome.browser.crypto_wallet.adapters.WalletCoinAdapter;
import org.chromium.chrome.browser.crypto_wallet.util.AssetUtils;
import org.chromium.chrome.browser.crypto_wallet.util.JavaUtils;
import org.chromium.chrome.browser.crypto_wallet.util.NetworkUtils;
import org.chromium.chrome.browser.crypto_wallet.util.TokenUtils;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.List;

public class UserAssetModel {
    private final Object mLock = new Object();
    private BraveWalletService mBraveWalletService;
    private JsonRpcService mJsonRpcService;
    private BlockchainRegistry mBlockchainRegistry;
    private final WalletCoinAdapter.AdapterType mType;
    private NetworkInfo mSelectedNetwork;
    private List<NetworkInfo> mCryptoNetworks;
    private CryptoSharedData mSharedData;
    private MutableLiveData<AssetsResult> _mAssetsResult;
    public LiveData<AssetsResult> mAssetsResult;

    public UserAssetModel(BraveWalletService braveWalletService, JsonRpcService jsonRpcService,
                          BlockchainRegistry blockchainRegistry,
                          CryptoSharedData sharedData, WalletCoinAdapter.AdapterType type) {
        mBraveWalletService = braveWalletService;
        mJsonRpcService = jsonRpcService;
        mBlockchainRegistry = blockchainRegistry;
        mSharedData = sharedData;
        mType = type;
        _mAssetsResult = new MutableLiveData<>();
        mAssetsResult = _mAssetsResult;
    }

    public void fetchAssets(boolean nftsOnly, NetworkInfo selectedNetwork){
        synchronized (mLock) {
            mSelectedNetwork = selectedNetwork;
            if (JavaUtils.anyNull(mBraveWalletService, mJsonRpcService, mSelectedNetwork)) return;
            NetworkModel.getAllNetworks(mJsonRpcService, mSharedData.getSupportedCryptoCoins(),
                    allNetworks -> {
                        mCryptoNetworks = new ArrayList<>(allNetworks);
                        if (mType == WalletCoinAdapter.AdapterType.EDIT_VISIBLE_ASSETS_LIST) {
                            if (NetworkUtils.isAllNetwork(mSelectedNetwork)) {
                                mBraveWalletService.getAllUserAssets(userAssets -> {
                                    TokenUtils.getAllTokens(mBlockchainRegistry, mCryptoNetworks
                                            ,
//                                            nftsOnly ? TokenUtils.TokenType.NFTS
//                                                    : TokenUtils.TokenType.NON_NFTS,
                                            tokens -> {
                                                AssetUtils.updateMissingLogoWithNetworkIcon(userAssets);

                                                _mAssetsResult.postValue(new AssetsResult(Arrays.asList(TokenUtils.distinctiveConcatenatedArrays(tokens,userAssets)),
                                                        Arrays.asList(userAssets)));
                                            });
                                });
                            }else {
                                TokenUtils.getUserAssetsFiltered(mBraveWalletService, mSelectedNetwork,
                                        mSelectedNetwork.coin, TokenUtils.TokenType.ALL, userAssets -> {
                                            TokenUtils.getAllTokensFiltered(mBraveWalletService,
                                                    mBlockchainRegistry, mSelectedNetwork,
                                                    mSelectedNetwork.coin,
                                                    nftsOnly ? TokenUtils.TokenType.NFTS
                                                            : TokenUtils.TokenType.NON_NFTS,
                                                    tokens -> {
                                                        _mAssetsResult.postValue(new AssetsResult(Arrays.asList(tokens), Arrays.asList(userAssets)));
                                                    });
                                        });
                            }
                        } else if (mType == WalletCoinAdapter.AdapterType.SEND_ASSETS_LIST) {
                            assert mSelectedNetwork != null;
                            TokenUtils.getUserAssetsFiltered(mBraveWalletService, mSelectedNetwork,
                                    mSelectedNetwork.coin, TokenUtils.TokenType.ALL, tokens -> {
                                        _mAssetsResult.postValue(new AssetsResult(Arrays.asList(tokens), Collections.emptyList()));
                                    });
                        } else if (mType == WalletCoinAdapter.AdapterType.SWAP_TO_ASSETS_LIST
                                || mType == WalletCoinAdapter.AdapterType.SWAP_FROM_ASSETS_LIST) {
                            assert mSelectedNetwork != null;
                            TokenUtils.getAllTokensFiltered(mBraveWalletService,
                                    mBlockchainRegistry,
                                    mSelectedNetwork, mSelectedNetwork.coin,
                                    TokenUtils.TokenType.ERC20, tokens -> {
                                        _mAssetsResult.postValue(new AssetsResult(Arrays.asList(tokens),
                                                Collections.emptyList()));
                                    });
                        } else if (mType == WalletCoinAdapter.AdapterType.BUY_ASSETS_LIST) {
                            TokenUtils.getBuyTokensFiltered(mBlockchainRegistry, mSelectedNetwork,
                                    TokenUtils.TokenType.ALL, BuyModel.SUPPORTED_RAMP_PROVIDERS,
                                    tokens -> {
                                        _mAssetsResult.postValue(new AssetsResult(Arrays.asList(tokens), Collections.emptyList()));
                                    });
                        }
                    });
        }
    }

    public static class AssetsResult{
        public List<BlockchainToken> tokens;
        public List<BlockchainToken> userAssets;

        public AssetsResult(List<BlockchainToken> tokens,
                            List<BlockchainToken> userAssets) {
            this.tokens = tokens;
            this.userAssets = userAssets;
        }
    }
}
