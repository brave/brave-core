/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.util;

import static org.chromium.chrome.browser.crypto_wallet.util.WalletConstants.SOLANA_TRANSACTION_TYPES;

import androidx.annotation.NonNull;

import org.chromium.brave_wallet.mojom.BtcTxData;
import org.chromium.brave_wallet.mojom.FilTxData;
import org.chromium.brave_wallet.mojom.NetworkInfo;
import org.chromium.brave_wallet.mojom.TransactionInfo;
import org.chromium.brave_wallet.mojom.TxData1559;
import org.chromium.brave_wallet.mojom.TxDataUnion;

import java.math.BigInteger;

/*
 * Transaction fees parser. Java version adapted from
 * components/brave_wallet_ui/utils/tx-utils.ts.
 */
public class ParsedTransactionFees {
    // Strings are initialized to empty string instead of null, as the default value from mojo
    private static String TAG = "ParsedTransactionFees";
    private String gasLimit = "";
    private String gasPrice = "";
    private String maxPriorityFeePerGas = "";
    private String maxFeePerGas = "";
    private double gasFee;
    private double gasFeeFiat;
    private boolean isEIP1559Transaction;
    private String missingGasLimitError = "";
    private double gasPremium;
    private double gasFeeCap;

    protected ParsedTransactionFees(
            String gasLimit,
            String gasPrice,
            String maxPriorityFeePerGas,
            String maxFeePerGas,
            double gasFee,
            double gasFeeFiat,
            boolean isEIP1559Transaction,
            String missingGasLimitError,
            double gasPremium,
            double gasFeeCap) {
        this.gasLimit = gasLimit;
        this.gasPrice = gasPrice;
        this.maxPriorityFeePerGas = maxPriorityFeePerGas;
        this.maxFeePerGas = maxFeePerGas;
        this.gasFee = gasFee;
        this.gasFeeFiat = gasFeeFiat;
        this.isEIP1559Transaction = isEIP1559Transaction;
        this.missingGasLimitError = missingGasLimitError;
        this.gasPremium = gasPremium;
        this.gasFeeCap = gasFeeCap;
    }

    public String getGasLimit() {
        return this.gasLimit;
    }

    public String getGasPrice() {
        return this.gasPrice;
    }

    public String getMaxPriorityFeePerGas() {
        return this.maxPriorityFeePerGas;
    }

    public String getMaxFeePerGas() {
        return this.maxFeePerGas;
    }

    public double getGasFee() {
        return this.gasFee;
    }

    public double getGasFeeFiat() {
        return this.gasFeeFiat;
    }

    public boolean getIsEIP1559Transaction() {
        return this.isEIP1559Transaction;
    }

    public String getMissingGasLimitError() {
        return this.missingGasLimitError;
    }

    public double getGasPremium() {
        return this.gasPremium;
    }

    public double getGasFeeCap() {
        return this.gasFeeCap;
    }

    private static String checkForMissingGasLimitError(String gasLimit) {
        return (gasLimit.isEmpty() || gasLimit.equals("0") || gasLimit.equals("0x0"))
                ? "braveWalletMissingGasLimitError"
                : null;
    }

    public static ParsedTransactionFees parseTransactionFees(
            TransactionInfo txInfo,
            NetworkInfo txNetwork,
            Double networkSpotPrice,
            long solFeeEstimatesFee) {
        TxDataUnion txDataUnion = txInfo.txDataUnion;
        TxData1559 txData =
                txDataUnion.which() == TxDataUnion.Tag.EthTxData1559
                        ? txDataUnion.getEthTxData1559()
                        : null;
        final FilTxData filTxData =
                txDataUnion.which() == TxDataUnion.Tag.FilTxData
                        ? txDataUnion.getFilTxData()
                        : null;
        final BtcTxData btcTxData =
                txDataUnion.which() == TxDataUnion.Tag.BtcTxData
                        ? txDataUnion.getBtcTxData()
                        : null;
        final int networkDecimals = txNetwork.decimals;
        final boolean isSolTransaction = SOLANA_TRANSACTION_TYPES.contains(txInfo.txType);
        final boolean isFilTransaction = filTxData != null;
        final boolean isBtcTransaction = btcTxData != null;

        final String gasLimit;
        final String gasPrice;
        final String maxFeePerGas;
        final String maxPriorityFeePerGas;
        if (isFilTransaction) {
            final String maxFeePerGasDecimal =
                    filTxData.gasFeeCap != null ? filTxData.gasFeeCap : "";
            final String maxPriorityFeePerGasDecimal =
                    filTxData.gasPremium != null ? filTxData.gasPremium : "";

            gasLimit = filTxData.gasLimit != null ? Utils.toHex(filTxData.gasLimit) : "";
            maxFeePerGas = Utils.toHex(maxFeePerGasDecimal);
            maxPriorityFeePerGas = Utils.toHex(maxPriorityFeePerGasDecimal);
            gasPrice = calculateFilGasPrice(maxFeePerGasDecimal, maxPriorityFeePerGasDecimal);
        } else {
            gasLimit = txData != null ? txData.baseData.gasLimit : "";
            gasPrice = txData != null ? txData.baseData.gasPrice : "";
            maxFeePerGas = txData != null ? txData.maxFeePerGas : "";
            maxPriorityFeePerGas = txData != null ? txData.maxPriorityFeePerGas : "";
        }
        final boolean isEIP1559Transaction =
                !maxPriorityFeePerGas.isEmpty() && !maxFeePerGas.isEmpty();
        final double[] gasFeeArr;
        if (isBtcTransaction) {
            final double gasFee = Utils.fromWei(Long.toString(btcTxData.fee), networkDecimals);
            final double gasFeeFiat = gasFee * networkSpotPrice;
            gasFeeArr = new double[] {gasFee, gasFeeFiat};
        } else {
            gasFeeArr =
                    calcGasFee(
                            txNetwork,
                            networkSpotPrice,
                            isEIP1559Transaction,
                            gasLimit,
                            gasPrice,
                            maxFeePerGas,
                            isSolTransaction,
                            solFeeEstimatesFee);
        }
        final double gasFee = gasFeeArr[0];
        final double gasFeeFiat = gasFeeArr[1];
        final String missingGasLimitError =
                isSolTransaction ? null : checkForMissingGasLimitError(gasLimit);
        final double gasPremium =
                isFilTransaction ? Utils.fromHexWei(filTxData.gasPremium, networkDecimals) : 0.0d;
        final double gasFeeCap =
                isFilTransaction ? Utils.fromHexWei(filTxData.gasFeeCap, networkDecimals) : 0.0d;

        return new ParsedTransactionFees(
                gasLimit,
                gasPrice,
                maxPriorityFeePerGas,
                maxFeePerGas,
                gasFee,
                gasFeeFiat,
                isEIP1559Transaction,
                missingGasLimitError,
                gasPremium,
                gasFeeCap);
    }

    @NonNull
    private static String calculateFilGasPrice(
            @NonNull final String maxFeePerGasDecimal,
            @NonNull final String maxPriorityFeePerGasDecimal) {
        if (maxFeePerGasDecimal.isEmpty() || maxPriorityFeePerGasDecimal.isEmpty()) {
            return "";
        } else {
            String result;
            try {
                BigInteger minuend = new BigInteger(maxFeePerGasDecimal);
                BigInteger subtrahend = new BigInteger(maxPriorityFeePerGasDecimal);
                BigInteger gasPrice = minuend.subtract(subtrahend);
                result = "0x" + gasPrice.toString(16);
            } catch (NumberFormatException nfe) {
                result = "";
            }
            return result;
        }
    }

    // Desktop doesn't seem to have slow/avg/fast MaxFeePerGas options,
    // so extracting this part as a separate function
    public static double[] calcGasFee(
            NetworkInfo selectedNetwork,
            double networkSpotPrice,
            boolean isEIP1559Transaction,
            String gasLimit,
            String gasPrice,
            String maxFeePerGas,
            boolean isSolTransaction,
            long solFeeEstimatesFee) {
        final int networkDecimals = selectedNetwork.decimals;
        double gasFee;
        if (isSolTransaction) {
            gasFee =
                    solFeeEstimatesFee != 0
                            ? Utils.fromWei(Long.toString(solFeeEstimatesFee), networkDecimals)
                            : 0.0d;
        } else {
            // Gas fee calculations for Eth and Fil use the same logic.
            gasFee =
                    Utils.fromHexWei(
                            isEIP1559Transaction
                                    ? Utils.multiplyHexBN(maxFeePerGas, gasLimit)
                                    : Utils.multiplyHexBN(gasPrice, gasLimit),
                            networkDecimals);
        }

        final double gasFeeFiat = gasFee * networkSpotPrice;

        return new double[] {gasFee, gasFeeFiat};
    }
}
