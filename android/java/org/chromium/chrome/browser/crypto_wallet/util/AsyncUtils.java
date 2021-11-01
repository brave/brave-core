/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.util;

import org.chromium.brave_wallet.mojom.AssetPrice;
import org.chromium.brave_wallet.mojom.AssetRatioController;
import org.chromium.brave_wallet.mojom.AssetTimePrice;
import org.chromium.brave_wallet.mojom.ErcToken;
import org.chromium.brave_wallet.mojom.EthJsonRpcController;
import org.chromium.brave_wallet.mojom.EthTxController;
import org.chromium.brave_wallet.mojom.TransactionInfo;

public class AsyncUtils {
    // Helper to track multiple wallet controllers responses
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
        public Boolean success;
        public String balance;
        public ErcToken userAsset;

        public GetBalanceResponseBaseContext(Runnable responseCompleteCallback) {
            super(responseCompleteCallback);
        }

        public void callBase(Boolean success, String balance) {
            this.success = success;
            this.balance = balance;
            super.fireResponseCompleteCallback();
        }
    }

    public static class GetErc20TokenBalanceResponseContext extends GetBalanceResponseBaseContext
            implements EthJsonRpcController.GetErc20TokenBalanceResponse {
        public GetErc20TokenBalanceResponseContext(Runnable responseCompleteCallback) {
            super(responseCompleteCallback);
        }

        @Override
        public void call(Boolean success, String balance) {
            super.callBase(success, balance);
        }
    }

    public static class GetBalanceResponseContext extends GetBalanceResponseBaseContext
            implements EthJsonRpcController.GetBalanceResponse {
        public GetBalanceResponseContext(Runnable responseCompleteCallback) {
            super(responseCompleteCallback);
        }

        @Override
        public void call(Boolean success, String balance) {
            super.callBase(success, balance);
        }
    }

    public static class GetPriceResponseContext
            extends SingleResponseBaseContext implements AssetRatioController.GetPriceResponse {
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
            implements EthTxController.GetAllTransactionInfoResponse {
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
            implements AssetRatioController.GetPriceHistoryResponse {
        public Boolean success;
        public AssetTimePrice[] timePrices;
        public ErcToken userAsset;

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
