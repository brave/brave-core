/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.model;

import android.graphics.Bitmap;
import android.text.TextUtils;

import org.chromium.brave_wallet.mojom.AccountInfo;
import org.chromium.brave_wallet.mojom.BlockchainToken;
import org.chromium.brave_wallet.mojom.NetworkInfo;
import org.chromium.brave_wallet.mojom.TransactionInfo;
import org.chromium.chrome.browser.app.domain.PortfolioModel;
import org.chromium.chrome.browser.crypto_wallet.util.ParsedTransaction;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;

public class WalletListItemModel {
    private int mIcon;
    private String mIconPath;
    private String mTitle;
    private String mSubTitle;
    private String mTxStatus;
    private Bitmap mTxStatusBitmap;
    private String mText1;
    private String mText2;
    private BlockchainToken mBlockchainToken;
    private AccountInfo mAccountInfo;
    private boolean mVisible;
    private double mTotalGas;
    private double mTotalGasFiat;
    private String[] mAddressesForBitmap;
    private TransactionInfo mTxInfo;
    private String mChainSymbol;
    private int mChainDecimals;
    private PortfolioModel.NftDataModel mNftDataModel;
    private NetworkInfo mAssetNetwork;
    private String mBrowserResPath;

    public String mId;
    private ParsedTransaction mParsedTx;

    public WalletListItemModel(
            int icon, String title, String subTitle, String id, String text1, String text2) {
        mIcon = icon;
        mTitle = title;
        mSubTitle = subTitle;
        mId = id;
        mText1 = text1;
        mText2 = text2;
    }

    private WalletListItemModel() {}

    public static WalletListItemModel makeForAccountInfoWithBalances(
            AccountInfo accountInfo, String fiatBalanceString, String cryptoBalanceString) {
        WalletListItemModel result = new WalletListItemModel();
        result.mIcon = Utils.getCoinIcon(accountInfo.accountId.coin);
        result.mTitle = accountInfo.name;
        // TODO(apaymyshev): handle bitcoin account.
        result.mSubTitle = accountInfo.address;
        result.mId = "";
        result.mText1 = fiatBalanceString;
        result.mText2 = cryptoBalanceString;
        result.mAccountInfo = accountInfo;
        return result;
    }

    public static WalletListItemModel makeForAccountInfo(AccountInfo accountInfo) {
        return makeForAccountInfoWithBalances(accountInfo, null, null);
    }

    public boolean isNft() {
        return mBlockchainToken.isNft || mBlockchainToken.isErc721;
    }

    public boolean hasNftImageLink() {
        return isNft() && mNftDataModel != null && mNftDataModel.nftMetadata != null
                && !TextUtils.isEmpty(mNftDataModel.nftMetadata.mImageUrl);
    }

    public void setTransactionInfo(TransactionInfo txInfo) {
        mTxInfo = txInfo;
    }

    public TransactionInfo getTransactionInfo() {
        return mTxInfo;
    }

    public void setAddressesForBitmap(String addressFrom, String addressTo) {
        mAddressesForBitmap = new String[2];
        mAddressesForBitmap[0] = addressFrom;
        mAddressesForBitmap[1] = addressTo;
    }

    public String[] getAddressesForBitmap() {
        return mAddressesForBitmap;
    }

    public void setIconPath(String iconPath) {
        mIconPath = iconPath;
    }

    public String getIconPath() {
        return mIconPath;
    }

    public void setTotalGas(double totalGas) {
        mTotalGas = totalGas;
    }

    public double getTotalGas() {
        return mTotalGas;
    }

    public void setTotalGasFiat(double totalGasFiat) {
        mTotalGasFiat = totalGasFiat;
    }

    public double getTotalGasFiat() {
        return mTotalGasFiat;
    }

    public void setChainSymbol(String chainSymbol) {
        mChainSymbol = chainSymbol;
    }

    public String getChainSymbol() {
        return mChainSymbol;
    }

    public void setChainDecimals(int chainDecimals) {
        mChainDecimals = chainDecimals;
    }

    public int getChainDecimals() {
        return mChainDecimals;
    }

    public void setTxStatus(String txStatus) {
        mTxStatus = txStatus;
    }

    public String getTxStatus() {
        return mTxStatus;
    }

    public void setTxStatusBitmap(Bitmap txStatusBitmap) {
        mTxStatusBitmap = txStatusBitmap;
    }

    public Bitmap getTxStatusBitmap() {
        return mTxStatusBitmap;
    }

    public void setBlockchainToken(BlockchainToken blockchainToken) {
        mBlockchainToken = blockchainToken;
    }

    public BlockchainToken getBlockchainToken() {
        return mBlockchainToken;
    }

    public void setAccountInfo(AccountInfo accountInfo) {
        mAccountInfo = accountInfo;
    }

    public AccountInfo getAccountInfo() {
        return mAccountInfo;
    }

    public int getIcon() {
        return mIcon;
    }

    // ERC721 has format [Title #ID]
    public String getTitle() {
        return isErc721() ? Utils.formatErc721TokenTitle(mTitle, Utils.hexToIntString(mId))
                          : mTitle;
    }

    public String getSubTitle() {
        return isErc721() ? Utils.formatErc721TokenTitle(mSubTitle, Utils.hexToIntString(mId))
                          : mSubTitle;
    }

    public String getText1() {
        return mText1;
    }

    public String getText2() {
        return mText2;
    }

    public boolean isVisible() {
        return mVisible;
    }

    public void isVisible(final boolean visible) {
        mVisible = visible;
    }

    public boolean isErc721() {
        return !(mId == null || mId.trim().isEmpty());
    }

    public boolean isAccount() {
        return mAccountInfo != null;
    }

    public PortfolioModel.NftDataModel getNftDataModel() {
        return mNftDataModel;
    }

    public void setNftDataModel(PortfolioModel.NftDataModel mNftDataModel) {
        this.mNftDataModel = mNftDataModel;
    }

    public NetworkInfo getAssetNetwork() {
        return mAssetNetwork;
    }

    public void setAssetNetwork(NetworkInfo mAssetNetwork) {
        this.mAssetNetwork = mAssetNetwork;
    }

    public void setBrowserResourcePath(String resPath) {
        mBrowserResPath = resPath;
    }

    public String getBrowserResourcePath() {
        return mBrowserResPath;
    }

    public boolean isNativeAsset() {
        if (mAssetNetwork == null || mBlockchainToken == null) return false;
        return Utils.isNativeToken(mAssetNetwork, mBlockchainToken);
    }

    public String getNetworkIcon() {
        if (mAssetNetwork == null || TextUtils.isEmpty(getBrowserResourcePath())) return "";
        return "file://" + getBrowserResourcePath() + "/" + Utils.getNetworkIconName(mAssetNetwork);
    }

    public ParsedTransaction getParsedTx() {
        return mParsedTx;
    }

    public void setParsedTx(ParsedTransaction mParsedTx) {
        this.mParsedTx = mParsedTx;
    }
}
