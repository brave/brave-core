/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.model;

import android.graphics.Bitmap;

import org.chromium.brave_wallet.mojom.BlockchainToken;
import org.chromium.brave_wallet.mojom.TransactionInfo;

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
    private boolean mIsImportedAccount;
    private boolean mIsUserSelected;
    private double mTotalGas;
    private double mTotalGasFiat;
    private String[] mAddressesForBitmap;
    private TransactionInfo mTxInfo;
    private String mChainSymbol;
    private int mChainDecimals;

    public WalletListItemModel(int icon, String title, String subTitle, String text1, String text2,
            boolean isImportedAccount) {
        mIcon = icon;
        mTitle = title;
        mSubTitle = subTitle;
        mText1 = text1;
        mText2 = text2;
        mIsImportedAccount = isImportedAccount;
    }

    public WalletListItemModel(
            int icon, String title, String subTitle, String text1, String text2) {
        mIcon = icon;
        mTitle = title;
        mSubTitle = subTitle;
        mText1 = text1;
        mText2 = text2;
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

    public void setIsImportedAccount(boolean isImportedAccount) {
        mIsImportedAccount = isImportedAccount;
    }

    public boolean getIsImportedAccount() {
        return mIsImportedAccount;
    }

    public int getIcon() {
        return mIcon;
    }

    public String getTitle() {
        return mTitle;
    }

    public String getSubTitle() {
        return mSubTitle;
    }

    public String getText1() {
        return mText1;
    }

    public String getText2() {
        return mText2;
    }

    public boolean getIsUserSelected() {
        return mIsUserSelected;
    }

    public void setIsUserSelected(boolean isUserSelected) {
        mIsUserSelected = isUserSelected;
    }
}
