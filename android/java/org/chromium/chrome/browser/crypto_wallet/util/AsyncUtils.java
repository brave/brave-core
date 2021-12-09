/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.util;

import static org.chromium.chrome.browser.crypto_wallet.util.Utils.warnWhenError;

import org.chromium.brave_wallet.mojom.AssetPrice;
import org.chromium.brave_wallet.mojom.AssetRatioService;
import org.chromium.brave_wallet.mojom.AssetTimePrice;
import org.chromium.brave_wallet.mojom.BlockchainToken;
import org.chromium.brave_wallet.mojom.EthTxService;
import org.chromium.brave_wallet.mojom.JsonRpcService;
import org.chromium.brave_wallet.mojom.TransactionInfo;

public class AsyncUtils {
    private final static String TAG = "AsyncUtils";

    // Helper to track multiple wallet services responses
    public static class MultiResponseHandler {
        private Runnable mWhenAllCompletedRunnable;
        private int mTotalElements;
        private int mCurrentElements;
        private Object mLock = new Object();

        public MultiResponseHandler(int totalElements) {
            synchronized (mLock) {
                mCurrentElements = 0;
                mTotalElements = totalElements;
            }
        }

        // TODO(AlexeyBarabash): add runnable to handle the timeout case
        public void setWhenAllCompletedAction(Runnable whenAllCompletedRunnable) {
            synchronized (mLock) {
                assert this.mWhenAllCompletedRunnable == null;
                this.mWhenAllCompletedRunnable = whenAllCompletedRunnable;
                checkAndRunCompletedAction();
            }
        }

        public Runnable singleResponseComplete = new Runnable() {
            @Override
            public void run() {
                synchronized (mLock) {
                    mCurrentElements++;
                    assert mCurrentElements <= mTotalElements;
                    checkAndRunCompletedAction();
                }
            }
        };

        private void checkAndRunCompletedAction() {
            if (mCurrentElements == mTotalElements && mWhenAllCompletedRunnable != null) {
                mWhenAllCompletedRunnable.run();
            }
        }
    }

    public static class SingleResponseBaseContext {
        private Runnable mResponseCompleteCallback;

        public SingleResponseBaseContext(Runnable responseCompleteCallback) {
            mResponseCompleteCallback = responseCompleteCallback;
        }

        public void fireResponseCompleteCallback() {
            assert mResponseCompleteCallback != null;
            mResponseCompleteCallback.run();
        }
    }

    public static class GetBalanceResponseBaseContext extends SingleResponseBaseContext {
        public String balance;
        public Integer error;
        public String errorMessage;
        public BlockchainToken userAsset;
        public String accountAddress;

        public GetBalanceResponseBaseContext(Runnable responseCompleteCallback) {
            super(responseCompleteCallback);
        }

        public void callBase(String balance, Integer error, String errorMessage) {
            this.balance = balance;
            this.error = error;
            this.errorMessage = errorMessage;
            super.fireResponseCompleteCallback();
        }
    }

    public static class GetErc20TokenBalanceResponseContext extends GetBalanceResponseBaseContext
            implements JsonRpcService.GetErc20TokenBalance_Response {
        public GetErc20TokenBalanceResponseContext(Runnable responseCompleteCallback) {
            super(responseCompleteCallback);
        }

        @Override
        public void call(String balance, Integer error, String errorMessage) {
            warnWhenError(TAG, "getErc20TokenBalance", error, errorMessage);
            super.callBase(balance, error, errorMessage);
        }
    }

    public static class GetBalanceResponseContext
            extends GetBalanceResponseBaseContext implements JsonRpcService.GetBalance_Response {
        public GetBalanceResponseContext(Runnable responseCompleteCallback) {
            super(responseCompleteCallback);
        }

        @Override
        public void call(String balance, Integer error, String errorMessage) {
            warnWhenError(TAG, "getBalance", error, errorMessage);
            super.callBase(balance, error, errorMessage);
        }
    }

    public static class GetPriceResponseContext
            extends SingleResponseBaseContext implements AssetRatioService.GetPrice_Response {
        public Boolean success;
        public AssetPrice[] prices;

        public GetPriceResponseContext(Runnable responseCompleteCallback) {
            super(responseCompleteCallback);
        }

        @Override
        public void call(Boolean success, AssetPrice[] prices) {
            this.success = success;
            this.prices = prices;
            super.fireResponseCompleteCallback();
        }
    }

    public static class GetAllTransactionInfoResponseContext extends SingleResponseBaseContext
            implements EthTxService.GetAllTransactionInfo_Response {
        public TransactionInfo[] txInfos;
        public String name;

        public GetAllTransactionInfoResponseContext(
                Runnable responseCompleteCallback, String name) {
            super(responseCompleteCallback);
            this.name = name;
        }

        @Override
        public void call(TransactionInfo[] txInfos) {
            this.txInfos = txInfos;
            super.fireResponseCompleteCallback();
        }
    }

    public static class GetPriceHistoryResponseContext extends SingleResponseBaseContext
            implements AssetRatioService.GetPriceHistory_Response {
        public Boolean success;
        public AssetTimePrice[] timePrices;
        public BlockchainToken userAsset;

        public GetPriceHistoryResponseContext(Runnable responseCompleteCallback) {
            super(responseCompleteCallback);
        }

        @Override
        public void call(Boolean success, AssetTimePrice[] timePrices) {
            this.success = success;
            this.timePrices = timePrices;
            super.fireResponseCompleteCallback();
        }
    }
}
