/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.util;

import org.chromium.brave_wallet.mojom.AccountInfo;
import org.chromium.brave_wallet.mojom.EthTxService;
import org.chromium.brave_wallet.mojom.TransactionInfo;
import org.chromium.brave_wallet.mojom.TransactionStatus;
import org.chromium.brave_wallet.mojom.TransactionType;
import org.chromium.chrome.browser.crypto_wallet.util.AsyncUtils;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.Comparator;
import java.util.HashMap;
import java.util.Locale;

public class PendingTxHelper {
    private EthTxService mEthTxService;
    private AccountInfo[] mAccountInfos;
    private HashMap<String, TransactionInfo[]> mTxInfos;
    private boolean mReturnAll;
    private String mFilterByContractAddress;
    private String mRopstenContractAddress;

    public PendingTxHelper(EthTxService ethTxService, AccountInfo[] accountInfos, boolean returnAll,
            String filterByContractAddress) {
        assert ethTxService != null;
        mEthTxService = ethTxService;
        mAccountInfos = accountInfos;
        mFilterByContractAddress = filterByContractAddress;
        mReturnAll = returnAll;
        mTxInfos = new HashMap<String, TransactionInfo[]>();
        if (mFilterByContractAddress != null && !mFilterByContractAddress.isEmpty()) {
            mRopstenContractAddress = Utils.getRopstenContractAddress(mFilterByContractAddress);
        }
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

            mEthTxService.getAllTransactionInfo(accountInfo.address, allTxContext);
        }

        allTxMultiResponse.setWhenAllCompletedAction(() -> {
            for (AsyncUtils.GetAllTransactionInfoResponseContext allTxContext : allTxContexts) {
                ArrayList<TransactionInfo> newValue = new ArrayList<TransactionInfo>();
                for (TransactionInfo txInfo : allTxContext.txInfos) {
                    if (mReturnAll || txInfo.txStatus == TransactionStatus.UNAPPROVED) {
                        if (mFilterByContractAddress == null) {
                            // Don't filter by contract
                            newValue.add(txInfo);
                        } else if (!mFilterByContractAddress.isEmpty()) {
                            if (mFilterByContractAddress.toLowerCase(Locale.getDefault())
                                            .equals(txInfo.txData.baseData.to.toLowerCase(
                                                    Locale.getDefault()))) {
                                newValue.add(txInfo);
                            }
                            if (mRopstenContractAddress != null
                                    && !mRopstenContractAddress.isEmpty()
                                    && mRopstenContractAddress.toLowerCase(Locale.getDefault())
                                               .equals(txInfo.txData.baseData.to.toLowerCase(
                                                       Locale.getDefault()))) {
                                newValue.add(txInfo);
                            }

                        } else if (txInfo.txType != TransactionType.ERC20_APPROVE
                                && txInfo.txType != TransactionType.ERC20_TRANSFER) {
                            // Filter by ETH only
                            newValue.add(txInfo);
                        }
                    }
                }
                Collections.sort(newValue, new Comparator<TransactionInfo>() {
                    @Override
                    public int compare(TransactionInfo lhs, TransactionInfo rhs) {
                        // -1 - less than, 1 - greater than, 0 - equal, all inversed for descending
                        return lhs.createdTime.microseconds > rhs.createdTime.microseconds
                                ? -1
                                : (lhs.createdTime.microseconds < rhs.createdTime.microseconds) ? 1
                                                                                                : 0;
                    }
                });
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
