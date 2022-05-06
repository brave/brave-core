/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.app.domain;

import androidx.lifecycle.LiveData;
import androidx.lifecycle.Transformations;

import org.chromium.brave_wallet.mojom.AccountInfo;
import org.chromium.brave_wallet.mojom.BraveWalletConstants;
import org.chromium.brave_wallet.mojom.KeyringService;
import org.chromium.brave_wallet.mojom.TransactionInfo;
import org.chromium.brave_wallet.mojom.TransactionStatus;
import org.chromium.brave_wallet.mojom.TxService;
import org.chromium.chrome.browser.crypto_wallet.util.PendingTxHelper;

import java.util.ArrayList;
import java.util.List;

public class CryptoModel {
    private TxService mTxService;
    private PendingTxHelper mPendingTxHelper;
    private KeyringService mKeyringService;
    private LiveData<List<TransactionInfo>> mPendingTransactions;
    // Todo: create a models for network and portfolio
    // Todo: create method to create and return new models for Asset, Account,
    //  TransactionConfirmation

    public CryptoModel(TxService txService, KeyringService keyringService) {
        this.mTxService = txService;
        this.mKeyringService = keyringService;
        mPendingTxHelper = new PendingTxHelper(mTxService, new AccountInfo[0], true, null, true);
        init();
    }

    public void resetServices(TxService txService, KeyringService keyringService) {
        mTxService = txService;
        mKeyringService = keyringService;
        mPendingTxHelper.setTxService(txService);
        init();
    }

    private void init() {
        getPendingTxHelper().fetchTransactions(null);
        mKeyringService.getKeyringInfo(BraveWalletConstants.DEFAULT_KEYRING_ID,
                keyringInfo -> { mPendingTxHelper.setAccountInfos(keyringInfo.accountInfos); });
        mPendingTransactions =
                Transformations.map(mPendingTxHelper.mTransactionInfoLd, transactionInfos -> {
                    List<TransactionInfo> pendingTransactionInfo = new ArrayList<>();
                    for (TransactionInfo info : transactionInfos) {
                        if (info.txStatus == TransactionStatus.UNAPPROVED) {
                            pendingTransactionInfo.add(info);
                        }
                    }
                    return pendingTransactionInfo;
                });
    }

    public void refreshTransactions() {
        mPendingTxHelper.fetchTransactions(null);
    }

    public LiveData<TransactionInfo> getSelectedPendingRequest() {
        return mPendingTxHelper.mSelectedPendingRequest;
    }

    public LiveData<List<TransactionInfo>> getPendingTransactions() {
        return mPendingTransactions;
    }

    public LiveData<List<TransactionInfo>> getAllTransactions() {
        return mPendingTxHelper.mTransactionInfoLd;
    }

    public PendingTxHelper getPendingTxHelper() {
        return mPendingTxHelper;
    }
}
