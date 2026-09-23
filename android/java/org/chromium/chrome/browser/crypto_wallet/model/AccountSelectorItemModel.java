/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.model;

import android.graphics.Bitmap;

import org.chromium.brave_wallet.mojom.AccountInfo;
import org.chromium.brave_wallet.mojom.BlockchainToken;
import org.chromium.brave_wallet.mojom.NetworkInfo;
import org.chromium.brave_wallet.mojom.TransactionInfo;
import org.chromium.chrome.browser.crypto_wallet.util.ParsedTransaction;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;

public class AccountSelectorItemModel {
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
    private boolean mSelected;
    private TransactionInfo mTxInfo;
    private NetworkInfo mAssetNetwork;

    public String mId;
    private ParsedTransaction mParsedTx;

    public AccountSelectorItemModel(
            int icon, String title, String subTitle, String id, String text1, String text2) {
        mIcon = icon;
        mTitle = title;
        mSubTitle = subTitle;
        mId = id;
        mText1 = text1;
        mText2 = text2;
    }

    private AccountSelectorItemModel() {}

    public static AccountSelectorItemModel makeForAccountInfoWithBalances(
            AccountInfo accountInfo, String fiatBalanceString, String cryptoBalanceString) {
        AccountSelectorItemModel result = new AccountSelectorItemModel();
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

    public static AccountSelectorItemModel makeForAccountInfo(AccountInfo accountInfo) {
        return makeForAccountInfoWithBalances(accountInfo, null, null);
    }

    public void setTransactionInfo(TransactionInfo txInfo) {
        mTxInfo = txInfo;
    }

    public TransactionInfo getTransactionInfo() {
        return mTxInfo;
    }

    public void setIconPath(String iconPath) {
        mIconPath = iconPath;
    }

    public String getIconPath() {
        return mIconPath;
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
        return isErc721()
                ? Utils.formatErc721TokenTitle(mTitle, Utils.hexToIntString(mId))
                : mTitle;
    }

    public String getSubTitle() {
        return isErc721()
                ? Utils.formatErc721TokenTitle(mSubTitle, Utils.hexToIntString(mId))
                : mSubTitle;
    }

    public String getText1() {
        return mText1;
    }

    public String getText2() {
        return mText2;
    }

    public boolean isSelected() {
        return mSelected;
    }

    public void setSelected(final boolean selected) {
        mSelected = selected;
    }

    public boolean isErc721() {
        return !(mId == null || mId.trim().isEmpty());
    }

    public boolean isAccount() {
        return mAccountInfo != null;
    }

    public NetworkInfo getAssetNetwork() {
        return mAssetNetwork;
    }

    public void setAssetNetwork(NetworkInfo mAssetNetwork) {
        this.mAssetNetwork = mAssetNetwork;
    }

    public ParsedTransaction getParsedTx() {
        return mParsedTx;
    }

    public void setParsedTx(ParsedTransaction mParsedTx) {
        this.mParsedTx = mParsedTx;
    }
}
