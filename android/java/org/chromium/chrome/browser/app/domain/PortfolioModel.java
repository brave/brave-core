/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.app.domain;

import android.content.Context;
import android.text.TextUtils;

import androidx.lifecycle.LiveData;
import androidx.lifecycle.MutableLiveData;

import org.json.JSONException;
import org.json.JSONObject;

import org.chromium.brave_wallet.mojom.AccountInfo;
import org.chromium.brave_wallet.mojom.AssetRatioService;
import org.chromium.brave_wallet.mojom.BlockchainRegistry;
import org.chromium.brave_wallet.mojom.BlockchainToken;
import org.chromium.brave_wallet.mojom.BraveWalletService;
import org.chromium.brave_wallet.mojom.CoinType;
import org.chromium.brave_wallet.mojom.EthTxManagerProxy;
import org.chromium.brave_wallet.mojom.JsonRpcService;
import org.chromium.brave_wallet.mojom.KeyringService;
import org.chromium.brave_wallet.mojom.NetworkInfo;
import org.chromium.brave_wallet.mojom.SolanaTxManagerProxy;
import org.chromium.brave_wallet.mojom.TxService;
import org.chromium.chrome.browser.crypto_wallet.activities.BraveWalletBaseActivity;
import org.chromium.chrome.browser.crypto_wallet.observers.BraveWalletServiceObserverImpl;
import org.chromium.chrome.browser.crypto_wallet.observers.BraveWalletServiceObserverImpl.BraveWalletServiceObserverImplDelegate;
import org.chromium.chrome.browser.crypto_wallet.observers.KeyringServiceObserverImpl;
import org.chromium.chrome.browser.crypto_wallet.util.AssetUtils;
import org.chromium.chrome.browser.crypto_wallet.util.AsyncUtils;
import org.chromium.chrome.browser.crypto_wallet.util.JavaUtils;
import org.chromium.chrome.browser.crypto_wallet.util.NetworkUtils;
import org.chromium.chrome.browser.crypto_wallet.util.PortfolioHelper;
import org.chromium.chrome.browser.crypto_wallet.util.TokenUtils;
import org.chromium.chrome.browser.crypto_wallet.util.WalletConstants;
import org.chromium.chrome.browser.crypto_wallet.util.WalletUtils;
import org.chromium.mojo.bindings.Callbacks;

import java.io.Serializable;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.List;

public class PortfolioModel implements BraveWalletServiceObserverImplDelegate {
    public final LiveData<List<NftDataModel>> mNftModels;
    private final CryptoSharedData mSharedData;
    private final Object mLock = new Object();
    private TxService mTxService;
    private KeyringService mKeyringService;
    private BlockchainRegistry mBlockchainRegistry;
    private JsonRpcService mJsonRpcService;
    private EthTxManagerProxy mEthTxManagerProxy;
    private SolanaTxManagerProxy mSolanaTxManagerProxy;
    private BraveWalletService mBraveWalletService;
    private AssetRatioService mAssetRatioService;
    private Context mContext;
    private final MutableLiveData<List<NftDataModel>> _mNftModels;
    private MutableLiveData<Boolean> _mIsDiscoveringUserAssets;
    public LiveData<Boolean> mIsDiscoveringUserAssets;
    private List<NetworkInfo> mAllNetworkInfos;
    public PortfolioHelper mPortfolioHelper;

    public PortfolioModel(Context context, TxService txService, KeyringService keyringService,
            BlockchainRegistry blockchainRegistry, JsonRpcService jsonRpcService,
            EthTxManagerProxy ethTxManagerProxy, SolanaTxManagerProxy solanaTxManagerProxy,
            BraveWalletService braveWalletService, AssetRatioService assetRatioService,
            CryptoSharedData sharedData) {
        mContext = context;
        mTxService = txService;
        mKeyringService = keyringService;
        mBlockchainRegistry = blockchainRegistry;
        mJsonRpcService = jsonRpcService;
        mEthTxManagerProxy = ethTxManagerProxy;
        mSolanaTxManagerProxy = solanaTxManagerProxy;
        mBraveWalletService = braveWalletService;
        mAssetRatioService = assetRatioService;
        mSharedData = sharedData;
        _mNftModels = new MutableLiveData<>();
        mNftModels = _mNftModels;
        _mIsDiscoveringUserAssets = new MutableLiveData<>();
        mIsDiscoveringUserAssets = _mIsDiscoveringUserAssets;
        addServiceObservers();
    }

    // TODO(pav): We should fetch and process all portfolio list here
    public void prepareNftListMetaData(List<BlockchainToken> nftList, NetworkInfo networkInfo,
            PortfolioHelper portfolioHelper) {
        mPortfolioHelper = portfolioHelper;
        // Filter out and calculate the size of supported NFTs.
        // The total sum will be used by `MultiResponseHandler` to detect
        // when `setWhenAllCompletedAction()` can be processed.
        int erc721NftsSize = JavaUtils.filter(nftList, (nft -> nft.isErc721)).size();
        int solanaNftsSize = JavaUtils.filter(nftList, (nft -> nft.coin == CoinType.SOL)).size();
        List<NftDataModel> nftDataModels = new ArrayList<>();
        AsyncUtils.MultiResponseHandler nftMetaDataHandler =
                new AsyncUtils.MultiResponseHandler(erc721NftsSize + solanaNftsSize);

        ArrayList<AsyncUtils.BaseGetNftMetadataContext> nftMetadataList = new ArrayList<>();
        for (BlockchainToken userAsset : nftList) {
            if (userAsset.isErc721) {
                AsyncUtils.GetNftErc721MetadataContext nftMetadata =
                        new AsyncUtils.GetNftErc721MetadataContext(
                                nftMetaDataHandler.singleResponseComplete);
                nftMetadata.asset = userAsset;
                mJsonRpcService.getErc721Metadata(userAsset.contractAddress, userAsset.tokenId,
                        userAsset.chainId, nftMetadata);
                nftMetadataList.add(nftMetadata);
            } else if (userAsset.isNft) {
                if (userAsset.coin == CoinType.SOL) {
                    // Solana NFTs.
                    AsyncUtils.GetNftSolanaMetadataContext nftMetadata =
                            new AsyncUtils.GetNftSolanaMetadataContext(
                                    nftMetaDataHandler.singleResponseComplete);
                    nftMetadata.asset = userAsset;
                    mJsonRpcService.getSolTokenMetadata(
                            userAsset.chainId, userAsset.contractAddress, nftMetadata);
                    nftMetadataList.add(nftMetadata);

                } else {
                    // Other NFTs.
                    nftDataModels.add(new NftDataModel(userAsset, networkInfo, null));
                }
            }
        }
        nftMetaDataHandler.setWhenAllCompletedAction(() -> {
            for (AsyncUtils.BaseGetNftMetadataContext metadata : nftMetadataList) {
                nftDataModels.add(new NftDataModel(metadata.asset, networkInfo,
                        new NftMetadata(metadata.tokenMetadata, metadata.errorCode,
                                metadata.errorMessage)));
            }
            _mNftModels.postValue(nftDataModels);
        });
    }

    public void discoverAssetsOnAllSupportedChains() {
        if (mBraveWalletService == null) return;
        _mIsDiscoveringUserAssets.postValue(true);
        mBraveWalletService.discoverAssetsOnAllSupportedChains();
    }

    public void fetchAssetsByType(TokenUtils.TokenType type, NetworkInfo selectedNetwork,
            BraveWalletBaseActivity braveWalletBaseActivity,
            Callbacks.Callback1<PortfolioHelper> callback) {
        synchronized (mLock) {
            if (mJsonRpcService == null) {
                return;
            }
            NetworkModel.getAllNetworks(
                    mJsonRpcService, mSharedData.getSupportedCryptoCoins(), allNetworks -> {
                        mAllNetworkInfos = new ArrayList<>(allNetworks);
                        if (selectedNetwork.chainId.equals(
                                    NetworkUtils.getAllNetworkOption(mContext).chainId)) {
                            mKeyringService.getKeyringsInfo(
                                    mSharedData.getEnabledKeyrings(), keyringInfos -> {
                                        AccountInfo[] accountInfos =
                                                WalletUtils
                                                        .getAccountInfosFromKeyrings(keyringInfos)
                                                        .toArray(new AccountInfo[0]);
                                        mPortfolioHelper =
                                                new PortfolioHelper(braveWalletBaseActivity,
                                                        mAllNetworkInfos, accountInfos);
                                        mPortfolioHelper.setSelectedNetworks(
                                                NetworkUtils.nonTestNetwork(mAllNetworkInfos));
                                        mPortfolioHelper.fetchAssetsAndDetails(type, callback);
                                    });
                        } else {
                            mKeyringService.getKeyringInfo(
                                    AssetUtils.getKeyring(
                                            selectedNetwork.coin, selectedNetwork.chainId),
                                    keyringInfo -> {
                                        mPortfolioHelper =
                                                new PortfolioHelper(braveWalletBaseActivity,
                                                        mAllNetworkInfos, keyringInfo.accountInfos);
                                        mPortfolioHelper.setSelectedNetworks(
                                                Arrays.asList(selectedNetwork));
                                        mPortfolioHelper.fetchAssetsAndDetails(type, callback);
                                    });
                        }
                    });
        }
    }

    @Override
    public void onDiscoverAssetsCompleted(BlockchainToken[] discoveredAssets) {
        _mIsDiscoveringUserAssets.postValue(false);
    }

    public void clearNftModels() {
        _mNftModels.postValue(Collections.emptyList());
    }

    private void addServiceObservers() {
        if (mBraveWalletService != null) {
            BraveWalletServiceObserverImpl walletServiceObserver =
                    new BraveWalletServiceObserverImpl(this);
            mBraveWalletService.addObserver(walletServiceObserver);
        }
    }

    void resetServices(Context context, TxService mTxService, KeyringService mKeyringService,
            BlockchainRegistry mBlockchainRegistry, JsonRpcService mJsonRpcService,
            EthTxManagerProxy mEthTxManagerProxy, SolanaTxManagerProxy mSolanaTxManagerProxy,
            BraveWalletService mBraveWalletService, AssetRatioService mAssetRatioService) {
        synchronized (mLock) {
            mContext = context;
            this.mTxService = mTxService;
            this.mKeyringService = mKeyringService;
            this.mBlockchainRegistry = mBlockchainRegistry;
            this.mJsonRpcService = mJsonRpcService;
            this.mEthTxManagerProxy = mEthTxManagerProxy;
            this.mSolanaTxManagerProxy = mSolanaTxManagerProxy;
            this.mBraveWalletService = mBraveWalletService;
            this.mAssetRatioService = mAssetRatioService;
            addServiceObservers();
        }
    }

    public static class NftDataModel {
        public BlockchainToken token;
        public NetworkInfo networkInfo;
        public NftMetadata nftMetadata;

        public NftDataModel(
                BlockchainToken token, NetworkInfo networkInfo, NftMetadata nftMetadata) {
            this.token = token;
            this.networkInfo = networkInfo;
            this.nftMetadata = nftMetadata;
        }
    }

    public static class NftMetadata implements Serializable {
        public String mImageUrl;
        public String mName;
        public int mErrCode;
        public String mErrMsg;
        // ERC721 only, but it may be present anyway in other standards.
        public String mDescription;

        public NftMetadata(String jsonString, int mErrCode, String mErrMsg) {
            this.mErrCode = mErrCode;
            this.mErrMsg = mErrMsg;
            try {
                JSONObject metaDataJObj = new JSONObject(jsonString);
                mDescription = metaDataJObj.optString("description");
                mImageUrl = refineUrl(metaDataJObj.optString("image"));
                mName = metaDataJObj.optString("name");
            } catch (JSONException ignored) {
            }
        }

        public NftMetadata(String mDescription, String mImageUrl, String mName) {
            this.mDescription = mDescription;
            this.mImageUrl = mImageUrl;
            this.mName = mName;
        }

        private String refineUrl(String url) {
            if (TextUtils.isEmpty(url)) return "";
            if (url.startsWith("data:image"))
                return url;
            else
                return AssetUtils.httpifyIpfsUrl(url);
        }
    }
}
