/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.util;

import org.chromium.brave_wallet.mojom.AccountInfo;
import org.chromium.brave_wallet.mojom.EthTxController;
import org.chromium.brave_wallet.mojom.TransactionInfo;
import org.chromium.brave_wallet.mojom.TransactionStatus;
import org.chromium.chrome.browser.crypto_wallet.util.AsyncUtils;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.HashMap;

public class PendingTxHelper {
    private EthTxController mEthTxController;
    private AccountInfo[] mAccountInfos;
    private HashMap<String, TransactionInfo[]> mTxInfos;

    public PendingTxHelper(EthTxController ethTxController, AccountInfo[] accountInfos) {
        assert ethTxController != null;
        mEthTxController = ethTxController;
        mAccountInfos = accountInfos;
        mTxInfos = new HashMap<String, TransactionInfo[]>();
    }

    public HashMap<String, TransactionInfo[]> getTransactions() {
        return mTxInfos;
    }

    public void fetchTransactions(Runnable runWhenDone) {
        AsyncUtils.MultiResponseHandler allTxMultiResponse =
                new AsyncUtils.MultiResponseHandler(mAccountInfos.length);
        ArrayList<AsyncUtils.GetAllTransactionInfoResponseContext> allTxContexts =
                new ArrayList<AsyncUtils.GetAllTransactionInfoResponseContext>();
        for (AccountInfo accountInfo : mAccountInfos) {
            AsyncUtils.GetAllTransactionInfoResponseContext allTxContext =
                    new AsyncUtils.GetAllTransactionInfoResponseContext(
                            allTxMultiResponse.singleResponseComplete, accountInfo.name);

            allTxContexts.add(allTxContext);

            mEthTxController.getAllTransactionInfo(accountInfo.address, allTxContext);
        }

        allTxMultiResponse.setWhenAllCompletedAction(() -> {
            for (AsyncUtils.GetAllTransactionInfoResponseContext allTxContext : allTxContexts) {
                ArrayList<TransactionInfo> newValue = new ArrayList<TransactionInfo>();
                for (TransactionInfo txInfo : allTxContext.txInfos) {
                    if (txInfo.txStatus == TransactionStatus.UNAPPROVED) {
                        newValue.add(txInfo);
                    }
                }
                TransactionInfo[] newArray = new TransactionInfo[newValue.size()];
                newArray = newValue.toArray(newArray);
                TransactionInfo[] value = mTxInfos.get(allTxContext.name);
                if (value == null) {
                    mTxInfos.put(allTxContext.name, newArray);
                } else {
                    TransactionInfo[] both = Arrays.copyOf(value, value.length + newArray.length);
                    System.arraycopy(newArray, 0, both, value.length, newArray.length);
                    mTxInfos.put(allTxContext.name, both);
                }
            }
            runWhenDone.run();
        });
    }
}
