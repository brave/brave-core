/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.util;

import static org.chromium.chrome.browser.crypto_wallet.util.WalletConstants.SOLANA_TRANSACTION_TYPES;

import android.text.TextUtils;

import org.chromium.brave_wallet.mojom.AccountInfo;
import org.chromium.brave_wallet.mojom.AssetPrice;
import org.chromium.brave_wallet.mojom.BlockchainToken;
import org.chromium.brave_wallet.mojom.BtcTxData;
import org.chromium.brave_wallet.mojom.FilTxData;
import org.chromium.brave_wallet.mojom.NetworkInfo;
import org.chromium.brave_wallet.mojom.SolanaInstruction;
import org.chromium.brave_wallet.mojom.SolanaSystemInstruction;
import org.chromium.brave_wallet.mojom.SolanaTokenInstruction;
import org.chromium.brave_wallet.mojom.SolanaTxData;
import org.chromium.brave_wallet.mojom.TransactionInfo;
import org.chromium.brave_wallet.mojom.TransactionType;
import org.chromium.brave_wallet.mojom.TxData1559;
import org.chromium.brave_wallet.mojom.TxDataUnion;
import org.chromium.brave_wallet.mojom.ZecTxData;
import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;
import org.chromium.chrome.browser.crypto_wallet.presenters.SolanaInstructionPresenter;
import org.chromium.mojo_base.mojom.TimeDelta;

import java.math.BigDecimal;
import java.util.ArrayList;
import java.util.List;
import java.util.Locale;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/*
 * Transaction parser. Java version of
 * components/brave_wallet_ui/common/hooks/transaction-parser.ts.
 */
@NullMarked
public class ParsedTransaction extends ParsedTransactionFees {
    // Strings are initialized to empty string instead of null, as the default value from mojo
    // Common fields
    public double marketPrice;
    private String hash = "";
    private String nonce = "";
    @Nullable private TimeDelta createdTime;
    private int status; // mojo loses enum type info in struct
    private int type; // mojo loses enum type info in struct
    private String sender = "";
    private String recipient = "";
    private double fiatTotal;
    private double value;
    private String symbol = "";
    @Nullable private BlockchainToken token;
    private int decimals;
    @Nullable private BlockchainToken erc721BlockchainToken;
    private boolean isSwap;

    // ZCash
    private boolean shielded;

    // Swap
    @Nullable private BlockchainToken sellToken;
    private double sellAmount;
    @Nullable private BlockchainToken buyToken;
    private double minBuyAmount;

    // Solana
    public boolean isSolanaDappTransaction;
    private boolean solChangeOfOwnership;

    // There are too many fields to init here
    private ParsedTransaction(ParsedTransactionFees parsedTransactionFees) {
        super(
                parsedTransactionFees.getGasLimit(),
                parsedTransactionFees.getGasPrice(),
                parsedTransactionFees.getMaxPriorityFeePerGas(),
                parsedTransactionFees.getMaxFeePerGas(),
                parsedTransactionFees.getGasFee(),
                parsedTransactionFees.getGasFeeFiat(),
                parsedTransactionFees.getIsEIP1559Transaction(),
                parsedTransactionFees.getMissingGasLimitError(),
                parsedTransactionFees.getGasPremium(),
                parsedTransactionFees.getGasFeeCap());
    }

    @Nullable
    private static BlockchainToken findToken(
            BlockchainToken[] fullTokenList, String contractAddress) {
        if (contractAddress == null) return null;

        for (BlockchainToken token : fullTokenList) {
            if (token.contractAddress
                    .toLowerCase(Locale.getDefault())
                    .equals(contractAddress.toLowerCase(Locale.getDefault()))) return token;
        }
        return null;
    }

    public static ParsedTransaction parseTransaction(
            TransactionInfo txInfo,
            NetworkInfo txNetwork,
            AccountInfo[] accounts,
            List<AssetPrice> assetPrices,
            long solFeeEstimatesFee,
            BlockchainToken[] fullTokenList) {
        BlockchainToken nativeAsset = Utils.makeNetworkAsset(txNetwork);
        Double networkSpotPrice = AssetsPricesHelper.getPriceForAsset(assetPrices, nativeAsset);

        final ParsedTransactionFees feeDetails =
                ParsedTransactionFees.parseTransactionFees(
                        txInfo, txNetwork, networkSpotPrice, solFeeEstimatesFee);
        TxDataUnion txDataUnion = txInfo.txDataUnion;
        TxData1559 txData =
                txDataUnion.which() == TxDataUnion.Tag.EthTxData1559
                        ? txDataUnion.getEthTxData1559()
                        : null;
        SolanaTxData solTxData =
                txDataUnion.which() == TxDataUnion.Tag.SolanaTxData
                        ? txDataUnion.getSolanaTxData()
                        : null;
        FilTxData filTxData =
                txDataUnion.which() == TxDataUnion.Tag.FilTxData
                        ? txDataUnion.getFilTxData()
                        : null;
        BtcTxData btcTxData =
                txDataUnion.which() == TxDataUnion.Tag.BtcTxData
                        ? txDataUnion.getBtcTxData()
                        : null;
        ZecTxData zecTxData =
                txDataUnion.which() == TxDataUnion.Tag.ZecTxData
                        ? txDataUnion.getZecTxData()
                        : null;

        final boolean isSPLTransaction =
                txInfo.txType == TransactionType.SOLANA_SPL_TOKEN_TRANSFER
                        || txInfo.txType
                                == TransactionType
                                        .SOLANA_SPL_TOKEN_TRANSFER_WITH_ASSOCIATED_TOKEN_ACCOUNT_CREATION;
        final boolean isSolTransaction = SOLANA_TRANSACTION_TYPES.contains(txInfo.txType);

        String value = "";
        if (isSPLTransaction && solTxData != null) {
            value = String.valueOf(solTxData.amount);
        } else if (isSolTransaction && solTxData != null) {
            value = String.valueOf(solTxData.lamports);
        } else if (filTxData != null && !TextUtils.isEmpty(filTxData.value)) {
            value = Utils.toHex(filTxData.value);
        } else if (btcTxData != null) {
            value = String.valueOf(btcTxData.amount);
        } else if (zecTxData != null) {
            value = String.valueOf(zecTxData.amount);
        } else if (txData != null) {
            value = txData.baseData.value;
        }

        String to = "";
        if (isSolTransaction && solTxData != null) {
            to = solTxData.toWalletAddress;
        } else if (filTxData != null) {
            to = filTxData.to;
        } else if (btcTxData != null) {
            to = btcTxData.to;
        } else if (zecTxData != null) {
            to = zecTxData.to;
        } else if (txData != null) {
            to = txData.baseData.to;
        }

        final String nonce = txData != null ? txData.baseData.nonce : "";
        AccountInfo account = Utils.findAccount(accounts, txInfo.fromAccountId);
        final String sender = account != null ? account.address : "";
        assert account != null;
        final BlockchainToken token =
                isSPLTransaction
                        ? findToken(fullTokenList, solTxData != null ? solTxData.tokenAddress : "")
                        : findToken(fullTokenList, to);
        ParsedTransaction parsedTransaction = new ParsedTransaction(feeDetails);
        // Common fields
        parsedTransaction.hash = txInfo.txHash;
        parsedTransaction.type = txInfo.txType;
        parsedTransaction.nonce = nonce;
        parsedTransaction.token = token;
        parsedTransaction.createdTime = txInfo.createdTime;
        parsedTransaction.status = txInfo.txStatus;
        parsedTransaction.sender = sender;
        parsedTransaction.isSolanaDappTransaction =
                WalletConstants.SOLANA_DAPPS_TRANSACTION_TYPES.contains(txInfo.txType);
        parsedTransaction.marketPrice = networkSpotPrice;
        if (zecTxData != null && zecTxData.useShieldedPool) {
            parsedTransaction.shielded = true;
        }

        int txType = txInfo.txType;
        if (txType == TransactionType.SOLANA_DAPP_SIGN_TRANSACTION
                || txType == TransactionType.SOLANA_DAPP_SIGN_AND_SEND_TRANSACTION
                || txType == TransactionType.SOLANA_SWAP
                || (txType == TransactionType.OTHER && solTxData != null)) {
            if (solTxData == null) {
                parsedTransaction.recipient = "";
                parsedTransaction.fiatTotal = 0;
                parsedTransaction.decimals = 0;
                parsedTransaction.symbol = "";
                return parsedTransaction;
            }
            BigDecimal lamportTransferredAmount = new BigDecimal(value);
            for (SolanaInstruction solanaInstruction : solTxData.instructions) {
                SolanaInstructionPresenter presenter =
                        new SolanaInstructionPresenter(solanaInstruction);
                String lamport = presenter.getLamportAmount();
                Integer instructionType = presenter.getInstructionType();
                if (instructionType != null
                        && (instructionType == SolanaSystemInstruction.ASSIGN_WITH_SEED
                                || instructionType == SolanaSystemInstruction.ASSIGN)) {
                    parsedTransaction.solChangeOfOwnership = true;
                }
                boolean isInsExists = instructionType != null;
                if (isInsExists
                        && (instructionType == SolanaSystemInstruction.TRANSFER
                                || instructionType == SolanaSystemInstruction.TRANSFER_WITH_SEED
                                || (presenter.isTokenInstruction()
                                        && instructionType == SolanaTokenInstruction.TRANSFER))) {
                    String fromPubKey = presenter.fromPubKey();
                    String toPubKey = presenter.toPubKey();
                    // only show lamports as transferred if the amount is going to a different
                    // pubKey
                    if (!toPubKey.equals(fromPubKey)) {
                        lamportTransferredAmount =
                                lamportTransferredAmount.add(new BigDecimal(lamport));
                    }
                } else if (isInsExists
                        && (instructionType == SolanaSystemInstruction.WITHDRAW_NONCE_ACCOUNT)) {
                    String noncePubKey =
                            presenter.getAccountPerParamKey(WalletConstants.SOL_DAPP_NONCE_ACCOUNT);
                    String toPubKey = presenter.toPubKey();
                    if (noncePubKey != null && noncePubKey.equals(account.address)) {
                        lamportTransferredAmount =
                                lamportTransferredAmount.add(new BigDecimal(lamport));
                    } else if (!TextUtils.isEmpty(toPubKey) && toPubKey.equals(account.address)) {
                        lamportTransferredAmount =
                                lamportTransferredAmount.subtract(new BigDecimal(lamport));
                    }
                } else if (isInsExists
                        && (instructionType == SolanaSystemInstruction.CREATE_ACCOUNT
                                || instructionType
                                        == SolanaSystemInstruction.CREATE_ACCOUNT_WITH_SEED)) {
                    String fromPubKey = presenter.fromPubKey();
                    String newAccountPubKey =
                            presenter.getAccountPerParamKey(WalletConstants.SOL_DAPP_NEW_ACCOUNT);
                    if (TextUtils.isEmpty(to)) {
                        to = newAccountPubKey;
                    }
                    if (!TextUtils.isEmpty(fromPubKey) && fromPubKey.equals(account.address)) {
                        lamportTransferredAmount =
                                lamportTransferredAmount.add(new BigDecimal(lamport));
                    }
                } else {
                    lamportTransferredAmount =
                            lamportTransferredAmount.add(new BigDecimal(lamport));
                }
                if (TextUtils.isEmpty(to) && presenter.toPubKey() != null) {
                    to = presenter.toPubKey();
                }
            }
            final int decimals = token != null ? token.decimals : Utils.SOL_DEFAULT_DECIMALS;
            final double price = AssetsPricesHelper.getPriceForAsset(assetPrices, token);
            final double sendAmount =
                    Utils.getBalanceForCoinType(
                            TransactionUtils.getCoinFromTxDataUnion(txDataUnion),
                            decimals,
                            lamportTransferredAmount.toPlainString());
            final double sendAmountFiat = sendAmount * price;
            final double totalAmountFiat = parsedTransaction.getGasFeeFiat() + sendAmountFiat;

            parsedTransaction.recipient = to;
            parsedTransaction.fiatTotal = totalAmountFiat;
            parsedTransaction.value = sendAmount;
            parsedTransaction.symbol = token != null ? token.symbol : "";
            parsedTransaction.decimals = Utils.SOL_DEFAULT_DECIMALS;
            parsedTransaction.isSwap = txType == TransactionType.SOLANA_SWAP;
            if (parsedTransaction.isSwap) {
                parsedTransaction.sellToken = nativeAsset;
                parsedTransaction.buyToken = nativeAsset;
            }
        } else if (txInfo.txType == TransactionType.ERC20_TRANSFER && txInfo.txArgs.length > 1) {
            final String address = txInfo.txArgs[0];
            final String amount = txInfo.txArgs[1];
            final int decimals = token != null ? token.decimals : Utils.ETH_DEFAULT_DECIMALS;
            final double price = AssetsPricesHelper.getPriceForAsset(assetPrices, token);
            final double sendAmount = Utils.fromHexWei(amount, decimals);
            final double sendAmountFiat = sendAmount * price;

            final double totalAmountFiat = parsedTransaction.getGasFeeFiat() + sendAmountFiat;

            parsedTransaction.recipient = address;
            parsedTransaction.fiatTotal = totalAmountFiat;
            parsedTransaction.value = sendAmount;
            parsedTransaction.symbol = token != null ? token.symbol : "";
            parsedTransaction.decimals = decimals;
        } else if ((txInfo.txType == TransactionType.ERC721_TRANSFER_FROM
                        || txInfo.txType == TransactionType.ERC721_SAFE_TRANSFER_FROM)
                && txInfo.txArgs.length > 2) {
            final String toAddress = txInfo.txArgs[1];

            final double totalAmountFiat = parsedTransaction.getGasFeeFiat();

            parsedTransaction.recipient = toAddress;
            parsedTransaction.fiatTotal = totalAmountFiat;
            parsedTransaction.value = 1; // Can only send 1 erc721 at a time
            parsedTransaction.symbol = token != null ? token.symbol : "";
            parsedTransaction.decimals = 0;
            parsedTransaction.erc721BlockchainToken = token;
        } else if (txInfo.txType == TransactionType.ERC20_APPROVE && txInfo.txArgs.length > 1) {
            final String amount = txInfo.txArgs[1];

            final int decimals = token != null ? token.decimals : Utils.ETH_DEFAULT_DECIMALS;
            final double totalAmountFiat = parsedTransaction.getGasFeeFiat();
            parsedTransaction.recipient = to;
            parsedTransaction.fiatTotal = totalAmountFiat;
            parsedTransaction.value = Utils.fromHexWei(amount, decimals);
            parsedTransaction.symbol = token != null ? token.symbol : "";
            parsedTransaction.decimals = decimals;
        } else if (txInfo.txType == TransactionType.SOLANA_SPL_TOKEN_TRANSFER
                || txInfo.txType
                        == TransactionType
                                .SOLANA_SPL_TOKEN_TRANSFER_WITH_ASSOCIATED_TOKEN_ACCOUNT_CREATION) {
            final int decimals = token != null ? token.decimals : Utils.SOL_DEFAULT_DECIMALS;
            final double price = AssetsPricesHelper.getPriceForAsset(assetPrices, token);
            final double sendAmount = Utils.fromWei(value, decimals);
            final double sendAmountFiat = sendAmount * price;

            final double totalAmountFiat = parsedTransaction.getGasFeeFiat() + sendAmountFiat;

            parsedTransaction.recipient = to;
            parsedTransaction.fiatTotal = totalAmountFiat;
            parsedTransaction.value = sendAmount;
            parsedTransaction.symbol = token != null ? token.symbol : "";
            parsedTransaction.decimals = decimals;
        } else if (txInfo.txType == TransactionType.ETH_SWAP && txInfo.txArgs.length > 2) {
            final String fillPath = txInfo.txArgs[0];
            final String sellAmountArg = txInfo.txArgs[1];
            final String minBuyAmountArg = txInfo.txArgs[1];

            final Pattern pattern = Pattern.compile("(.{1,40})");
            final Matcher matcher = pattern.matcher(fillPath.substring(2));
            List<BlockchainToken> fillTokensList = new ArrayList<>();
            while (matcher.find()) {
                String address = "0x" + matcher.group();
                BlockchainToken thisToken = findToken(fullTokenList, address);
                fillTokensList.add(
                        address.equals(Utils.ETHEREUM_CONTRACT_FOR_SWAP)
                                ? nativeAsset
                                : thisToken != null ? thisToken : nativeAsset);
            }
            BlockchainToken[] fillTokens = fillTokensList.toArray(new BlockchainToken[0]);

            final BlockchainToken sellToken = fillTokens.length == 1 ? nativeAsset : fillTokens[0];
            final double price = AssetsPricesHelper.getPriceForAsset(assetPrices, sellToken);
            final String sellAmountWei =
                    sellAmountArg != null && !sellAmountArg.isEmpty() ? sellAmountArg : value;
            final double sellAmountFiat =
                    Utils.fromHexWei(sellAmountWei, sellToken.decimals) * price;

            final BlockchainToken buyToken = fillTokens[fillTokens.length - 1];
            final double buyAmount = Utils.fromHexWei(minBuyAmountArg, buyToken.decimals);

            final double totalAmountFiat = parsedTransaction.getGasFeeFiat() + sellAmountFiat;

            final double sellAmount = Utils.fromHexWei(sellAmountWei, sellToken.decimals);

            parsedTransaction.recipient = to;
            parsedTransaction.fiatTotal = totalAmountFiat;
            parsedTransaction.value = sellAmount;
            parsedTransaction.symbol = sellToken.symbol;
            parsedTransaction.decimals = sellToken.decimals;
            parsedTransaction.isSwap = true;
            parsedTransaction.sellToken = sellToken;
            parsedTransaction.sellAmount = sellAmount;
            parsedTransaction.buyToken = buyToken;
            parsedTransaction.minBuyAmount = buyAmount;
        } else {
            // The rest cases falls through to default
            final double price = AssetsPricesHelper.getPriceForAsset(assetPrices, nativeAsset);
            double sendAmount;
            if (txInfo.txType == TransactionType.SOLANA_SYSTEM_TRANSFER || zecTxData != null) {
                sendAmount = Utils.fromWei(value, txNetwork.decimals);
            } else {
                sendAmount = Utils.fromHexWei(value, txNetwork.decimals);
            }

            final double sendAmountFiat = sendAmount * price;

            final double totalAmountFiat = parsedTransaction.getGasFeeFiat() + sendAmountFiat;

            parsedTransaction.recipient = to;
            parsedTransaction.fiatTotal = totalAmountFiat;
            parsedTransaction.value = sendAmount;
            parsedTransaction.symbol = txNetwork.symbol;
            parsedTransaction.decimals = txNetwork.decimals;
            parsedTransaction.isSwap =
                    to.toLowerCase(Locale.getDefault()).equals(Utils.SWAP_EXCHANGE_PROXY);
        }

        return parsedTransaction;
    }

    // TODO (Wengling): change it to reflect desktop Amount.format
    public String formatValueToDisplay() {
        if (this.type == TransactionType.ERC20_TRANSFER) {
            return String.format(Locale.getDefault(), "%.4f", this.value);
        } else if (this.type == TransactionType.ERC721_TRANSFER_FROM
                || this.type == TransactionType.ERC721_SAFE_TRANSFER_FROM) {
            return "1";
        } else if (this.type == TransactionType.ERC20_APPROVE) {
            return "0.0000";
        } else if (this.isSwap) {
            return String.format(Locale.getDefault(), "%.4f", this.value);
        } else {
            String sVal = String.format(Locale.getDefault(), "%.9f", value);
            // Show amount without trailing zeros
            return !sVal.contains(".") ? sVal : sVal.replaceAll("0*$", "").replaceAll("\\.$", "");
        }
    }

    public String getHash() {
        return this.hash;
    }

    public String getNonce() {
        return this.nonce;
    }

    @Nullable
    public TimeDelta getCreatedTime() {
        return this.createdTime;
    }

    public int getStatus() {
        return this.status;
    }

    public int getType() {
        return this.type;
    }

    public String getSender() {
        return this.sender;
    }

    public String getRecipient() {
        return this.recipient;
    }

    public double getFiatTotal() {
        return this.fiatTotal;
    }

    public double getValue() {
        return this.value;
    }

    public String getSymbol() {
        return this.symbol;
    }

    @Nullable
    public BlockchainToken getToken() {
        return this.token;
    }

    public int getDecimals() {
        return this.decimals;
    }

    @Nullable
    public BlockchainToken getErc721BlockchainToken() {
        return this.erc721BlockchainToken;
    }

    public boolean getIsSwap() {
        return this.isSwap;
    }

    @Nullable
    public BlockchainToken getSellToken() {
        return this.sellToken;
    }

    public double getSellAmount() {
        return this.sellAmount;
    }

    @Nullable
    public BlockchainToken getBuyToken() {
        return this.buyToken;
    }

    public double getMinBuyAmount() {
        return this.minBuyAmount;
    }

    public boolean isSolChangeOfOwnership() {
        return solChangeOfOwnership;
    }

    public boolean isShielded() {
        return shielded;
    }
}
