/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.util;

import androidx.lifecycle.LiveData;
import androidx.lifecycle.MutableLiveData;

import org.chromium.brave_wallet.mojom.AccountInfo;
import org.chromium.brave_wallet.mojom.CoinType;
import org.chromium.brave_wallet.mojom.TransactionInfo;
import org.chromium.brave_wallet.mojom.TransactionStatus;
import org.chromium.brave_wallet.mojom.TransactionType;
import org.chromium.brave_wallet.mojom.TxService;
import org.chromium.chrome.browser.crypto_wallet.observers.TxServiceObserver;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.Comparator;
import java.util.HashMap;
import java.util.List;
import java.util.Locale;

public class PendingTxHelper implements TxServiceObserver {
    private TxService mTxService;
    private AccountInfo[] mAccountInfos;
    private HashMap<String, TransactionInfo[]> mTxInfos;
    private boolean mReturnAll;
    private String mFilterByContractAddress;
    private String mRopstenContractAddress;
    private final List<TransactionInfo> mTransactionInfos;
    private final List<TransactionCacheRecord> mCacheTransactionInfos;
    private boolean isFetchingTx;
    private final MutableLiveData<TransactionInfo> _mSelectedPendingRequest;
    private final MutableLiveData<Boolean> _mHasNoPendingTxAfterProcessing;
    private final MutableLiveData<List<TransactionInfo>> _mTransactionInfos;
    public LiveData<List<TransactionInfo>> mTransactionInfoLd;
    public LiveData<TransactionInfo> mSelectedPendingRequest;
    public LiveData<Boolean> mHasNoPendingTxAfterProcessing;

    public PendingTxHelper(TxService txService, AccountInfo[] accountInfos, boolean returnAll) {
        assert txService != null;
        mTxService = txService;
        mAccountInfos = accountInfos;
        mReturnAll = returnAll;
        mTxInfos = new HashMap<String, TransactionInfo[]>();

        mTransactionInfos = new ArrayList<>();
        mCacheTransactionInfos = new ArrayList<>();
        _mSelectedPendingRequest = new MutableLiveData<>();
        _mHasNoPendingTxAfterProcessing = new MutableLiveData<>();
        _mTransactionInfos = new MutableLiveData<>(Collections.emptyList());
        mTransactionInfoLd = _mTransactionInfos;
        mSelectedPendingRequest = _mSelectedPendingRequest;
        mHasNoPendingTxAfterProcessing = _mHasNoPendingTxAfterProcessing;
    }

    public PendingTxHelper(TxService txService, AccountInfo[] accountInfos, boolean returnAll,
            boolean shouldObserveTxUpdates) {
        this(txService, accountInfos, returnAll);
        if (shouldObserveTxUpdates) {
            txService.addObserver(this);
        }
    }

    public HashMap<String, TransactionInfo[]> getTransactions() {
        return mTxInfos;
    }

    public void fetchTransactions(Runnable runWhenDone) {
        isFetchingTx = true;
        mTransactionInfos.clear();
        mCacheTransactionInfos.clear();
        mTxInfos.clear();
        _mTransactionInfos.postValue(Collections.emptyList());
        _mSelectedPendingRequest.postValue(null);
        AsyncUtils.MultiResponseHandler allTxMultiResponse =
                new AsyncUtils.MultiResponseHandler(mAccountInfos.length);
        ArrayList<AsyncUtils.GetAllTransactionInfoResponseContext> allTxContexts =
                new ArrayList<AsyncUtils.GetAllTransactionInfoResponseContext>();
        for (AccountInfo accountInfo : mAccountInfos) {
            AsyncUtils.GetAllTransactionInfoResponseContext allTxContext =
                    new AsyncUtils.GetAllTransactionInfoResponseContext(
                            allTxMultiResponse.singleResponseComplete, accountInfo.name);
            allTxContexts.add(allTxContext);
            mTxService.getAllTransactionInfo(accountInfo.coin, accountInfo.address, allTxContext);
        }
        allTxMultiResponse.setWhenAllCompletedAction(() -> {
            for (AsyncUtils.GetAllTransactionInfoResponseContext allTxContext : allTxContexts) {
                ArrayList<TransactionInfo> newValue = new ArrayList<TransactionInfo>();
                for (TransactionInfo txInfo : allTxContext.txInfos) {
                    if (mReturnAll || txInfo.txStatus == TransactionStatus.UNAPPROVED) {
                        if (mFilterByContractAddress == null) {
                            // Don't filter by contract
                            newValue.add(txInfo);
                        } else if (txInfo.txType != TransactionType.ERC20_APPROVE
                                && txInfo.txType != TransactionType.ERC20_TRANSFER
                                && txInfo.txType != TransactionType.ERC721_TRANSFER_FROM
                                && txInfo.txType != TransactionType.ERC721_SAFE_TRANSFER_FROM) {
                            // TODO: Filter by ETH only
                            newValue.add(txInfo);
                        }
                    }
                }
                Collections.sort(newValue, sortByDateComparator);
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
            isFetchingTx = false;
            updateTransactionList();
            if (runWhenDone != null) {
                runWhenDone.run();
            }
        });
    }

    public void setAccountInfos(AccountInfo[] accountInfos) {
        this.mAccountInfos = accountInfos;
        fetchTransactions(null);
    }

    public void setAccountInfos(List<AccountInfo> accountInfos) {
        this.mAccountInfos = accountInfos.toArray(new AccountInfo[0]);
        fetchTransactions(null);
    }

    @Override
    public void onNewUnapprovedTx(TransactionInfo txInfo) {
        processTx(txInfo, TxActionType.NEW_UNAPPROVED_TRANSACTION);
    }

    @Override
    public void onUnapprovedTxUpdated(TransactionInfo txInfo) {
        processTx(txInfo, TxActionType.UNAPPROVED_TRANSACTION_UPDATED);
    }

    @Override
    public void onTransactionStatusChanged(TransactionInfo txInfo) {
        processTx(txInfo, TxActionType.TRANSACTION_STATUS_CHANGED);
    }

    public List<TransactionInfo> getPendingTransactions() {
        return mTransactionInfos;
    }

    public void setTxService(TxService txService) {
        this.mTxService = txService;
    }

    private void updateTransactionList() {
        for (TransactionInfo[] transactionInfoArr : mTxInfos.values()) {
            Collections.addAll(mTransactionInfos, transactionInfoArr);
        }
        processCachedTx();
        Collections.sort(mTransactionInfos, sortByDateComparator);
        _mTransactionInfos.postValue(mTransactionInfos);
        postTxUpdates();
    }

    private void processTx(TransactionInfo txInfo, TxActionType txActionType) {
        if (isFetchingTx) {
            mCacheTransactionInfos.add(new TransactionCacheRecord(txActionType, txInfo));
        } else {
            updateTransactionList(txInfo, txActionType);
            _mTransactionInfos.postValue(mTransactionInfos);
        }
    }

    private void processCachedTx() {
        if (!mCacheTransactionInfos.isEmpty()) {
            for (TransactionCacheRecord info : mCacheTransactionInfos) {
                updateTransactionList(info.getTransactionInfo(), info.getTxActionType());
            }
        }
        mCacheTransactionInfos.clear();
    }

    private void updateTransactionList(TransactionInfo txInfo, TxActionType txActionType) {
        if (txActionType == TxActionType.NEW_UNAPPROVED_TRANSACTION) {
            if (mTransactionInfos.size() == 0) {
                _mSelectedPendingRequest.postValue(txInfo);
            }
            mTransactionInfos.add(txInfo);
        } else {
            if (txActionType == TxActionType.UNAPPROVED_TRANSACTION_UPDATED) {
                for (int i = 0; i < mTransactionInfos.size(); i++) {
                    TransactionInfo info = mTransactionInfos.get(i);
                    if (info.id.equals(txInfo.id)) {
                        mTransactionInfos.set(i, txInfo);
                        break;
                    }
                }
                if (getSelectedPendingRequest() != null
                        && getSelectedPendingRequest().id.equals(txInfo.id)) {
                    _mSelectedPendingRequest.postValue(txInfo);
                }
            } else if (txActionType == TxActionType.TRANSACTION_STATUS_CHANGED) {
                List<TransactionInfo> newTransactionInfos = new ArrayList<>();
                for (int i = 0; i < mTransactionInfos.size(); i++) {
                    TransactionInfo info = mTransactionInfos.get(i);
                    if (!info.id.equals(txInfo.id)) {
                        newTransactionInfos.add(info);
                    }
                }
                if (txInfo.txStatus == TransactionStatus.UNAPPROVED) {
                    newTransactionInfos.add(txInfo);
                }
                mTransactionInfos.clear();
                mTransactionInfos.addAll(newTransactionInfos);
                Collections.sort(mTransactionInfos, sortByDateComparator);
                if ((_mSelectedPendingRequest.getValue() != null
                                    && _mSelectedPendingRequest.getValue().id.equals(txInfo.id)
                            || _mSelectedPendingRequest.getValue() == null)) {
                    postTxUpdates();
                }
            }
        }
    }

    public TransactionInfo getSelectedPendingRequest() {
        return mSelectedPendingRequest.getValue();
    }

    private void postTxUpdates() {
        if (mTransactionInfos.size() > 0) {
            _mSelectedPendingRequest.postValue(getFirstUnapprovedTx());
        } else {
            _mSelectedPendingRequest.postValue(null);
            _mHasNoPendingTxAfterProcessing.postValue(true);
        }
    }

    /*
     * Make sure to use get the first unapproved transaction as source list may not have one due to
     * returnAll set to true
     */
    private TransactionInfo getFirstUnapprovedTx() {
        for (TransactionInfo transactionInfo : mTransactionInfos) {
            if (transactionInfo.txStatus == TransactionStatus.UNAPPROVED) {
                return transactionInfo;
            }
        }
        return null;
    }

    private final Comparator<TransactionInfo> sortByDateComparator =
            (lhs, rhs) -> Long.compare(rhs.createdTime.microseconds, lhs.createdTime.microseconds);

    private static class TransactionCacheRecord {
        private final TxActionType mTxActionType;
        private final TransactionInfo mTransactionInfo;

        public TransactionCacheRecord(
                TxActionType mTxActionType, TransactionInfo mTransactionInfo) {
            this.mTxActionType = mTxActionType;
            this.mTransactionInfo = mTransactionInfo;
        }

        public TxActionType getTxActionType() {
            return mTxActionType;
        }

        public TransactionInfo getTransactionInfo() {
            return mTransactionInfo;
        }
    }

    private enum TxActionType {
        NEW_UNAPPROVED_TRANSACTION,
        UNAPPROVED_TRANSACTION_UPDATED,
        TRANSACTION_STATUS_CHANGED
    }
}
