/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.util;

import static org.chromium.chrome.browser.crypto_wallet.util.Utils.warnWhenError;

import org.chromium.base.Callbacks;
import org.chromium.brave_wallet.mojom.AssetPrice;
import org.chromium.brave_wallet.mojom.AssetRatioService;
import org.chromium.brave_wallet.mojom.AssetTimePrice;
import org.chromium.brave_wallet.mojom.BlockchainRegistry;
import org.chromium.brave_wallet.mojom.BlockchainToken;
import org.chromium.brave_wallet.mojom.BraveWalletConstants;
import org.chromium.brave_wallet.mojom.JsonRpcService;
import org.chromium.brave_wallet.mojom.NetworkInfo;
import org.chromium.brave_wallet.mojom.SolanaFeeEstimation;
import org.chromium.brave_wallet.mojom.SolanaTxManagerProxy;
import org.chromium.brave_wallet.mojom.TransactionInfo;
import org.chromium.brave_wallet.mojom.TxService;

import java.util.HashMap;
import java.util.HashSet;

public class AsyncUtils {
    private static final String TAG = "AsyncUtils";

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

        public Runnable singleResponseComplete =
                new Runnable() {
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
        public Integer decimals;
        public String uiAmountString;
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

        // For GetSplTokenAccountBalance
        public void callSplBase(
                String amount,
                Integer decimals,
                String uiAmountString,
                Integer error,
                String errorMessage) {
            this.decimals = decimals;
            this.uiAmountString = uiAmountString;
            this.callBase(amount, error, errorMessage);
        }
    }

    public static class GetErc20TokenBalanceResponseContext extends GetBalanceResponseBaseContext
            implements JsonRpcService.GetErc20TokenBalance_Response {
        public GetErc20TokenBalanceResponseContext(Runnable responseCompleteCallback) {
            super(responseCompleteCallback);
        }

        @Override
        public void call(String balance, int error, String errorMessage) {
            warnWhenError(TAG, "getErc20TokenBalance", error, errorMessage);
            super.callBase(balance, error, errorMessage);
        }
    }

    public static class GetErc721TokenBalanceResponseContext extends GetBalanceResponseBaseContext
            implements JsonRpcService.GetErc721TokenBalance_Response {
        public GetErc721TokenBalanceResponseContext(Runnable responseCompleteCallback) {
            super(responseCompleteCallback);
        }

        @Override
        public void call(String balance, int error, String errorMessage) {
            warnWhenError(TAG, "getErc721TokenBalance", error, errorMessage);
            super.callBase(balance, error, errorMessage);
        }
    }

    public static class GetBalanceResponseContext extends GetBalanceResponseBaseContext
            implements JsonRpcService.GetBalance_Response {
        public GetBalanceResponseContext(Runnable responseCompleteCallback) {
            super(responseCompleteCallback);
        }

        @Override
        public void call(String balance, int error, String errorMessage) {
            warnWhenError(TAG, "getBalance", error, errorMessage);
            super.callBase(balance, error, errorMessage);
        }
    }

    public static class GetSolanaBalanceResponseContext extends GetBalanceResponseBaseContext
            implements JsonRpcService.GetSolanaBalance_Response {
        public GetSolanaBalanceResponseContext(Runnable responseCompleteCallback) {
            super(responseCompleteCallback);
        }

        // Returned balance is Long instead of String
        @Override
        public void call(long balance, int error, String errorMessage) {
            warnWhenError(TAG, "getSolanaBalance", error, errorMessage);
            super.callBase(String.valueOf(balance), error, errorMessage);
        }
    }

    public static class GetSplTokenAccountBalanceResponseContext
            extends GetBalanceResponseBaseContext
            implements JsonRpcService.GetSplTokenAccountBalance_Response {
        public GetSplTokenAccountBalanceResponseContext(Runnable responseCompleteCallback) {
            super(responseCompleteCallback);
        }

        @Override
        public void call(
                String amount,
                byte decimals,
                String uiAmountString,
                int error,
                String errorMessage) {
            warnWhenError(TAG, "getSplTokenAccountBalance", error, errorMessage);
            super.callSplBase(
                    amount, Integer.valueOf(decimals), uiAmountString, error, errorMessage);
        }
    }

    public static class GetPriceResponseContext extends SingleResponseBaseContext
            implements AssetRatioService.GetPrice_Response {
        public Boolean success;
        public AssetPrice[] prices;

        public GetPriceResponseContext(Runnable responseCompleteCallback) {
            super(responseCompleteCallback);
        }

        @Override
        public void call(boolean success, AssetPrice[] prices) {
            this.success = success;
            this.prices = prices;
            super.fireResponseCompleteCallback();
        }
    }

    public static class GetAllTransactionInfoResponseContext extends SingleResponseBaseContext
            implements TxService.GetAllTransactionInfo_Response {
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
        public void call(boolean success, AssetTimePrice[] timePrices) {
            this.success = success;
            this.timePrices = timePrices;
            super.fireResponseCompleteCallback();
        }
    }

    public static class FetchPricesResponseContext extends SingleResponseBaseContext
            implements Callbacks.Callback1<HashMap<String, Double>> {
        public HashMap<String, Double> assetPrices;

        public FetchPricesResponseContext(Runnable responseCompleteCallback) {
            super(responseCompleteCallback);
        }

        @Override
        public void call(HashMap<String, Double> assetPrices) {
            this.assetPrices = assetPrices;
            super.fireResponseCompleteCallback();
        }
    }

    public static class GetNativeAssetsBalancesResponseContext extends SingleResponseBaseContext
            implements Callbacks.Callback2<Integer, HashMap<String, Double>> {
        public int coinType;
        public HashMap<String, Double> nativeAssetsBalances;

        public GetNativeAssetsBalancesResponseContext(Runnable responseCompleteCallback) {
            super(responseCompleteCallback);
        }

        @Override
        public void call(Integer coinType, HashMap<String, Double> nativeAssetsBalances) {
            this.coinType = coinType;
            this.nativeAssetsBalances = nativeAssetsBalances;
            super.fireResponseCompleteCallback();
        }
    }

    public static class GetBlockchainTokensBalancesResponseContext extends SingleResponseBaseContext
            implements Callbacks.Callback2<Integer, HashMap<String, HashMap<String, Double>>> {
        public HashMap<String, HashMap<String, Double>> blockchainTokensBalances;
        public int coinType;

        public GetBlockchainTokensBalancesResponseContext(Runnable responseCompleteCallback) {
            super(responseCompleteCallback);
        }

        @Override
        public void call(
                Integer coinType,
                HashMap<String, HashMap<String, Double>> blockchainTokensBalances) {
            this.coinType = coinType;
            this.blockchainTokensBalances = blockchainTokensBalances;
            super.fireResponseCompleteCallback();
        }
    }

    public static class GetTxExtraInfoResponseContext extends SingleResponseBaseContext
            implements Callbacks.Callback4<
                    HashMap<String, Double>,
                    BlockchainToken[],
                    HashMap<String, Double>,
                    HashMap<String, HashMap<String, Double>>> {
        public HashMap<String, Double> assetPrices;
        public BlockchainToken[] tokenList;
        public HashMap<String, Double> nativeAssetsBalances;
        public HashMap<String, HashMap<String, Double>> blockchainTokensBalances;

        public GetTxExtraInfoResponseContext(Runnable responseCompleteCallback) {
            super(responseCompleteCallback);
        }

        @Override
        public void call(
                HashMap<String, Double> assetPrices,
                BlockchainToken[] tokenList,
                HashMap<String, Double> nativeAssetsBalances,
                HashMap<String, HashMap<String, Double>> blockchainTokensBalances) {
            this.assetPrices = assetPrices;
            this.tokenList = tokenList;
            this.nativeAssetsBalances = nativeAssetsBalances;
            this.blockchainTokensBalances = blockchainTokensBalances;
            super.fireResponseCompleteCallback();
        }
    }

    public static class GetSolanaEstimatedTxFeeResponseContext extends SingleResponseBaseContext
            implements SolanaTxManagerProxy.GetSolanaTxFeeEstimation_Response {
        public Long fee;
        public Integer error;
        public String errorMessage;
        public String txMetaId;

        public GetSolanaEstimatedTxFeeResponseContext(Runnable responseCompleteCallback) {
            super(responseCompleteCallback);
        }

        @Override
        public void call(SolanaFeeEstimation fee, int error, String errorMessage) {
            this.fee =
                    fee.baseFee
                            + (((long) fee.computeUnits * fee.feePerComputeUnit)
                                    / BraveWalletConstants.MICRO_LAMPORTS_PER_LAMPORT);
            this.error = error;
            this.errorMessage = errorMessage;
            super.fireResponseCompleteCallback();
        }
    }

    public static class GetP3ABalancesContext extends SingleResponseBaseContext
            implements Callbacks.Callback1<HashMap<Integer, HashSet<String>>> {
        public HashMap<Integer, HashSet<String>> activeAddresses;

        public GetP3ABalancesContext(Runnable responseCompleteCallback) {
            super(responseCompleteCallback);
        }

        @Override
        public void call(HashMap<Integer, HashSet<String>> activeAddresses) {
            this.activeAddresses = activeAddresses;
            super.fireResponseCompleteCallback();
        }
    }

    public abstract static class BaseGetNftMetadataContext extends SingleResponseBaseContext {
        public BlockchainToken asset;
        public String tokenMetadata;
        public Integer errorCode;
        public String errorMessage;

        public BaseGetNftMetadataContext(Runnable responseCompleteCallback) {
            super(responseCompleteCallback);
        }
    }

    public static class GetNftSolanaMetadataContext extends BaseGetNftMetadataContext
            implements JsonRpcService.GetSolTokenMetadata_Response {
        public GetNftSolanaMetadataContext(Runnable responseCompleteCallback) {
            super(responseCompleteCallback);
        }

        @Override
        public void call(
                String tokenUrl, String tokenMetadata, int errorCode, String errorMessage) {
            this.tokenMetadata = tokenMetadata;
            this.errorCode = errorCode;
            this.errorMessage = errorMessage;
            super.fireResponseCompleteCallback();
        }
    }

    public static class GetNftErc721MetadataContext extends BaseGetNftMetadataContext
            implements JsonRpcService.GetErc721Metadata_Response {
        public GetNftErc721MetadataContext(Runnable responseCompleteCallback) {
            super(responseCompleteCallback);
        }

        @Override
        public void call(
                String tokenUrl, String erc721Metadata, int errorCode, String errorMessage) {
            this.tokenMetadata = erc721Metadata;
            this.errorCode = errorCode;
            this.errorMessage = errorMessage;
            super.fireResponseCompleteCallback();
        }
    }

    public static class GetNetworkAllTokensContext extends SingleResponseBaseContext
            implements BlockchainRegistry.GetAllTokens_Response {
        public BlockchainToken[] tokens;
        public NetworkInfo networkInfo;

        public GetNetworkAllTokensContext(
                Runnable responseCompleteCallback, NetworkInfo networkInfo) {
            super(responseCompleteCallback);
            this.networkInfo = networkInfo;
        }

        @Override
        public void call(BlockchainToken[] tokens) {
            this.tokens = tokens;
            super.fireResponseCompleteCallback();
        }
    }
}
