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

import org.chromium.brave_wallet.mojom.AssetRatioService;
import org.chromium.brave_wallet.mojom.BlockchainRegistry;
import org.chromium.brave_wallet.mojom.BlockchainToken;
import org.chromium.brave_wallet.mojom.BraveWalletService;
import org.chromium.brave_wallet.mojom.EthTxManagerProxy;
import org.chromium.brave_wallet.mojom.JsonRpcService;
import org.chromium.brave_wallet.mojom.KeyringService;
import org.chromium.brave_wallet.mojom.NetworkInfo;
import org.chromium.brave_wallet.mojom.SolanaTxManagerProxy;
import org.chromium.brave_wallet.mojom.TxService;
import org.chromium.chrome.browser.crypto_wallet.util.AssetUtils;
import org.chromium.chrome.browser.crypto_wallet.util.AsyncUtils;
import org.chromium.chrome.browser.crypto_wallet.util.JavaUtils;
import org.chromium.chrome.browser.crypto_wallet.util.PortfolioHelper;

import java.io.Serializable;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

public class PortfolioModel {
    public final LiveData<List<NftDataModel>> mNftModels;
    private final CryptoSharedData mSharedData;
    private final MutableLiveData<List<NftDataModel>> _mNftModels;
    private final Object mLock = new Object();
    public PortfolioHelper mPortfolioHelper;
    private TxService mTxService;
    private KeyringService mKeyringService;
    private BlockchainRegistry mBlockchainRegistry;
    private JsonRpcService mJsonRpcService;
    private EthTxManagerProxy mEthTxManagerProxy;
    private SolanaTxManagerProxy mSolanaTxManagerProxy;
    private BraveWalletService mBraveWalletService;
    private AssetRatioService mAssetRatioService;
    private Context mContext;

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
        _mNftModels = new MutableLiveData<>(Collections.emptyList());
        mNftModels = _mNftModels;
    }

    // TODO(pav): We should fetch and process all portfolio list here
    public void prepareNftListMetaData(List<BlockchainToken> nftList, NetworkInfo networkInfo,
            PortfolioHelper portfolioHelper) {
        mPortfolioHelper = portfolioHelper;
        List<BlockchainToken> ercNfts = JavaUtils.filter(nftList, nft -> nft.isErc721);
        List<NftDataModel> nftDataModels = new ArrayList<>();
        AsyncUtils.MultiResponseHandler nftMetaDataHandler =
                new AsyncUtils.MultiResponseHandler(ercNfts.size());

        ArrayList<AsyncUtils.GetNftMetaDataContext> nftMetaDatas = new ArrayList<>();
        for (BlockchainToken userAsset : nftList) {
            if (userAsset.isErc721) {
                AsyncUtils.GetNftMetaDataContext nftMetaData = new AsyncUtils.GetNftMetaDataContext(
                        nftMetaDataHandler.singleResponseComplete);
                nftMetaData.asset = userAsset;
                mJsonRpcService.getErc721Metadata(userAsset.contractAddress, userAsset.tokenId,
                        userAsset.chainId, nftMetaData);
                nftMetaDatas.add(nftMetaData);
            } else if (userAsset.isNft) { // other nfts e.g. solana
                nftDataModels.add(new NftDataModel(userAsset, networkInfo, null));
            }
        }
        nftMetaDataHandler.setWhenAllCompletedAction(() -> {
            for (AsyncUtils.GetNftMetaDataContext metaData : nftMetaDatas) {
                nftDataModels.add(new NftDataModel(metaData.asset, networkInfo,
                        new Erc721MetaData(metaData.erc721Metadata, metaData.errorCode,
                                metaData.errorMessage)));
            }
            _mNftModels.postValue(nftDataModels);
        });
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
        }
    }

    public class NftDataModel {
        public BlockchainToken token;
        public NetworkInfo networkInfo;
        public Erc721MetaData erc721MetaData;

        public NftDataModel(
                BlockchainToken token, NetworkInfo networkInfo, Erc721MetaData erc721MetaData) {
            this.token = token;
            this.networkInfo = networkInfo;
            this.erc721MetaData = erc721MetaData;
        }
    }

    public class Erc721MetaData implements Serializable {
        public String mDescription;
        public String mImageUrl;
        public String mName;
        public int mErrCode;
        public String mErrMsg;

        public Erc721MetaData(String jsonString, int mErrCode, String mErrMsg) {
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

        public Erc721MetaData(String mDescription, String mImageUrl, String mName) {
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
