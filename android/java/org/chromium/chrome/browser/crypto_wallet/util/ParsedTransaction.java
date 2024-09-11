/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.util;

import static org.chromium.chrome.browser.crypto_wallet.util.WalletConstants.SOLANA_TRANSACTION_TYPES;

import android.content.Context;
import android.text.TextUtils;
import android.util.Pair;

import org.chromium.brave_wallet.mojom.AccountInfo;
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
import org.chromium.chrome.R;
import org.chromium.chrome.browser.crypto_wallet.presenters.SolanaInstructionPresenter;
import org.chromium.mojo_base.mojom.TimeDelta;

import java.math.BigDecimal;
import java.math.BigInteger;
import java.text.DateFormat;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Date;
import java.util.HashMap;
import java.util.List;
import java.util.Locale;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/*
 * Transaction parser. Java version of
 * components/brave_wallet_ui/common/hooks/transaction-parser.ts.
 */
public class ParsedTransaction extends ParsedTransactionFees {
    // Strings are initialized to empty string instead of null, as the default value from mojo
    // Common fields
    private String hash = "";
    private String nonce = "";
    private TimeDelta createdTime;
    private int status; // mojo loses enum type info in struct
    private int type; // mojo loses enum type info in struct
    private String sender = "";
    private String senderLabel = "";
    private String recipient = "";
    private String recipientLabel = "";
    private double fiatValue;
    private double fiatTotal;
    private double nativeCurrencyTotal; // Cannot format directly because format is in R
    private double value;
    private String symbol = "";
    private BlockchainToken token;
    private int decimals;
    private boolean insufficientFundsForGasError;
    private boolean insufficientFundsError;
    private String contractAddressError = "";
    private String sameAddressError = "";
    private BlockchainToken erc721BlockchainToken;
    private String erc721TokenId = "";
    private boolean isSwap;
    public double marketPrice;

    // Token approvals
    private String approvalTarget = "";
    private String approvalTargetLabel = "";
    private boolean isApprovalUnlimited;

    // Swap
    private BlockchainToken sellToken;
    private double sellAmount;
    private BlockchainToken buyToken;
    private double minBuyAmount;

    // Solana
    public boolean isSolanaDappTransaction;

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

    /** Checks if a given address is a known contract address from our token registry. */
    private static String checkForContractAddressError(BlockchainToken[] fullTokenList, String to) {
        BlockchainToken token = findToken(fullTokenList, to);
        return token != null ? "braveWalletContractAddressError" : null;
    }

    /** Checks if a given set of sender and recipient addresses are the same. */
    private static String checkForSameAddressError(String from, String to) {
        if (from == null || to == null) {
            return null;
        }
        return to.toLowerCase(Locale.getDefault()).equals(from.toLowerCase(Locale.getDefault()))
                ? "braveWalletSameAddressError"
                : null;
    }

    public static ParsedTransaction parseTransaction(
            TransactionInfo txInfo,
            NetworkInfo txNetwork,
            AccountInfo[] accounts,
            HashMap<String, Double> assetPrices,
            long solFeeEstimatesFee,
            BlockchainToken[] fullTokenList,
            HashMap<String, Double> nativeAssetsBalances,
            HashMap<String, HashMap<String, Double>> blockchainTokensBalances) {
        BlockchainToken nativeAsset = Utils.makeNetworkAsset(txNetwork);
        Double networkSpotPrice =
                Utils.getOrDefault(
                        assetPrices, txNetwork.symbol.toLowerCase(Locale.getDefault()), 0.0d);

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

        final boolean isFilTransaction = filTxData != null;
        final boolean isSPLTransaction =
                txInfo.txType == TransactionType.SOLANA_SPL_TOKEN_TRANSFER
                        || txInfo.txType
                                == TransactionType
                                        .SOLANA_SPL_TOKEN_TRANSFER_WITH_ASSOCIATED_TOKEN_ACCOUNT_CREATION;
        final boolean isSolTransaction = SOLANA_TRANSACTION_TYPES.contains(txInfo.txType);
        final boolean isBtcTransaction = btcTxData != null;

        final String value =
                isSPLTransaction
                        ? solTxData != null ? String.valueOf(solTxData.amount) : ""
                        : isSolTransaction
                                ? solTxData != null ? String.valueOf(solTxData.lamports) : ""
                                : isFilTransaction
                                        ? filTxData.value != null
                                                ? Utils.toHex(filTxData.value)
                                                : ""
                                        : isBtcTransaction
                                                ? String.valueOf(btcTxData.amount)
                                                : txData != null ? txData.baseData.value : "";

        String to =
                isSolTransaction
                        ? solTxData != null ? solTxData.toWalletAddress : ""
                        : isFilTransaction
                                ? filTxData.to
                                : isBtcTransaction ? btcTxData.to : txData.baseData.to;

        final String nonce = txData != null ? txData.baseData.nonce : "";
        AccountInfo account = Utils.findAccount(accounts, txInfo.fromAccountId);

        final String accountAddressLower =
                account != null ? account.address.toLowerCase(Locale.getDefault()) : "";
        final BlockchainToken token =
                isSPLTransaction
                        ? findToken(fullTokenList, solTxData != null ? solTxData.tokenAddress : "")
                        : findToken(fullTokenList, to);
        final String tokenSymbolLower =
                token != null ? token.symbol.toLowerCase(Locale.getDefault()) : "";
        final double accountNativeBalance =
                Utils.getOrDefault(nativeAssetsBalances, accountAddressLower, 0.0d);
        final double accountTokenBalance =
                Utils.getOrDefault(
                        Utils.getOrDefault(
                                blockchainTokensBalances,
                                accountAddressLower,
                                new HashMap<String, Double>()),
                        Utils.tokenToString(token),
                        0.0d);

        ParsedTransaction parsedTransaction = new ParsedTransaction(feeDetails);
        // Common fields
        parsedTransaction.hash = txInfo.txHash;
        parsedTransaction.type = txInfo.txType;
        parsedTransaction.nonce = nonce;
        parsedTransaction.token = token;
        parsedTransaction.createdTime = txInfo.createdTime;
        parsedTransaction.status = txInfo.txStatus;
        parsedTransaction.sender = txInfo.fromAddress != null ? txInfo.fromAddress : "";
        parsedTransaction.senderLabel = account != null ? account.name : "";
        parsedTransaction.isSolanaDappTransaction =
                WalletConstants.SOLANA_DAPPS_TRANSACTION_TYPES.contains(txInfo.txType);
        parsedTransaction.marketPrice = networkSpotPrice;

        int txType = txInfo.txType;
        if (txType == TransactionType.SOLANA_DAPP_SIGN_TRANSACTION
                || txType == TransactionType.SOLANA_DAPP_SIGN_AND_SEND_TRANSACTION
                || txType == TransactionType.SOLANA_SWAP
                || txType == TransactionType.OTHER && solTxData != null) {
            if (solTxData == null) {
                parsedTransaction.recipient = "";
                parsedTransaction.recipientLabel = "";
                parsedTransaction.fiatTotal = 0;
                parsedTransaction.fiatValue = 0;
                parsedTransaction.decimals = 0;
                parsedTransaction.symbol = "";
                return parsedTransaction;
            }
            assert (txInfo.fromAddress != null);
            assert (txInfo.fromAddress.equals(account.address));
            BigDecimal lamportTransferredAmount = new BigDecimal(value);
            for (SolanaInstruction solanaInstruction : solTxData.instructions) {
                SolanaInstructionPresenter presenter =
                        new SolanaInstructionPresenter(solanaInstruction);
                String lamport = presenter.getLamportAmount();
                Integer instructionType = presenter.getInstructionType();
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
                    if (noncePubKey != null && noncePubKey.equals(txInfo.fromAddress)) {
                        lamportTransferredAmount =
                                lamportTransferredAmount.add(new BigDecimal(lamport));
                    } else if (!TextUtils.isEmpty(toPubKey)
                            && toPubKey.equals(txInfo.fromAddress)) {
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
                    if (!TextUtils.isEmpty(fromPubKey) && fromPubKey.equals(txInfo.fromAddress)) {
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
            final double price = Utils.getOrDefault(assetPrices, tokenSymbolLower, 0.0d);
            final double sendAmount =
                    Utils.getBalanceForCoinType(
                            TransactionUtils.getCoinFromTxDataUnion(txDataUnion),
                            decimals,
                            lamportTransferredAmount.toPlainString());
            final double sendAmountFiat = sendAmount * price;
            final double totalAmountFiat = parsedTransaction.getGasFeeFiat() + sendAmountFiat;
            final boolean insufficientNativeFunds =
                    parsedTransaction.getGasFee() > accountNativeBalance;
            final boolean insufficientTokenFunds = sendAmount > accountTokenBalance;

            parsedTransaction.recipient = to;
            parsedTransaction.recipientLabel = getAddressLabel(accounts, to);
            parsedTransaction.fiatValue = sendAmountFiat;
            parsedTransaction.fiatTotal = totalAmountFiat;
            parsedTransaction.nativeCurrencyTotal = sendAmountFiat / networkSpotPrice;
            parsedTransaction.value = sendAmount;
            parsedTransaction.symbol = token != null ? token.symbol : "";
            parsedTransaction.decimals = Utils.SOL_DEFAULT_DECIMALS;
            parsedTransaction.insufficientFundsError = insufficientTokenFunds;
            parsedTransaction.insufficientFundsForGasError = insufficientNativeFunds;
            parsedTransaction.isSwap = txType == TransactionType.SOLANA_SWAP;
            parsedTransaction.contractAddressError =
                    checkForContractAddressError(
                            fullTokenList,
                            solTxData.toWalletAddress != null ? solTxData.toWalletAddress : to);
            if (parsedTransaction.isSwap) {
                parsedTransaction.sellToken = nativeAsset;
                parsedTransaction.buyToken = nativeAsset;
            }
        } else if (txInfo.txType == TransactionType.ERC20_TRANSFER && txInfo.txArgs.length > 1) {
            final String address = txInfo.txArgs[0];
            final String amount = txInfo.txArgs[1];
            final int decimals = token != null ? token.decimals : Utils.ETH_DEFAULT_DECIMALS;
            final double price = Utils.getOrDefault(assetPrices, tokenSymbolLower, 0.0d);
            final double sendAmount = Utils.fromHexWei(amount, decimals);
            final double sendAmountFiat = sendAmount * price;

            final double totalAmountFiat = parsedTransaction.getGasFeeFiat() + sendAmountFiat;

            final boolean insufficientNativeFunds =
                    parsedTransaction.getGasFee() > accountNativeBalance;
            final boolean insufficientTokenFunds = sendAmount > accountTokenBalance;

            parsedTransaction.recipient = address;
            parsedTransaction.recipientLabel = getAddressLabel(accounts, address);
            parsedTransaction.fiatValue = sendAmountFiat;
            parsedTransaction.fiatTotal = totalAmountFiat;
            parsedTransaction.nativeCurrencyTotal = sendAmountFiat / networkSpotPrice;
            parsedTransaction.value = sendAmount;
            parsedTransaction.symbol = token != null ? token.symbol : "";
            parsedTransaction.decimals = decimals;
            parsedTransaction.insufficientFundsError = insufficientTokenFunds;
            parsedTransaction.insufficientFundsForGasError = insufficientNativeFunds;
            parsedTransaction.contractAddressError =
                    checkForContractAddressError(fullTokenList, address);
            parsedTransaction.sameAddressError =
                    checkForSameAddressError(address, txInfo.fromAddress);
        } else if ((txInfo.txType == TransactionType.ERC721_TRANSFER_FROM
                        || txInfo.txType == TransactionType.ERC721_SAFE_TRANSFER_FROM)
                && txInfo.txArgs.length > 2) {
            final String owner = txInfo.txArgs[0];
            final String toAddress = txInfo.txArgs[1];
            final String tokenID = txInfo.txArgs[2];

            final double totalAmountFiat = parsedTransaction.getGasFeeFiat();

            final boolean insufficientNativeFunds =
                    parsedTransaction.getGasFee() > accountNativeBalance;

            parsedTransaction.recipient = toAddress;
            parsedTransaction.recipientLabel = getAddressLabel(accounts, toAddress);
            parsedTransaction.fiatValue = 0; // Display NFT values in the future
            parsedTransaction.fiatTotal = totalAmountFiat;
            parsedTransaction.nativeCurrencyTotal = totalAmountFiat / networkSpotPrice;
            parsedTransaction.value = 1; // Can only send 1 erc721 at a time
            parsedTransaction.symbol = token != null ? token.symbol : "";
            parsedTransaction.decimals = 0;
            parsedTransaction.insufficientFundsForGasError = insufficientNativeFunds;
            parsedTransaction.insufficientFundsError = false;
            parsedTransaction.erc721BlockchainToken = token;
            parsedTransaction.erc721TokenId = tokenID;
            parsedTransaction.contractAddressError =
                    checkForContractAddressError(fullTokenList, toAddress);
            parsedTransaction.sameAddressError = checkForSameAddressError(toAddress, owner);
        } else if (txInfo.txType == TransactionType.ERC20_APPROVE && txInfo.txArgs.length > 1) {
            final String address = txInfo.txArgs[0];
            final String amount = txInfo.txArgs[1];

            final int decimals = token != null ? token.decimals : Utils.ETH_DEFAULT_DECIMALS;
            final double totalAmountFiat = parsedTransaction.getGasFeeFiat();
            final BigInteger amountWrapped =
                    new BigInteger(amount.startsWith("0x") ? amount.substring(2) : amount, 16);
            final boolean insufficientNativeFunds =
                    parsedTransaction.getGasFee() > accountNativeBalance;

            parsedTransaction.recipient = to;
            parsedTransaction.recipientLabel = getAddressLabel(accounts, to);
            parsedTransaction.fiatValue = 0;
            parsedTransaction.fiatTotal = totalAmountFiat;
            parsedTransaction.nativeCurrencyTotal = 0;
            parsedTransaction.value = Utils.fromHexWei(amount, decimals);
            parsedTransaction.symbol = token != null ? token.symbol : "";
            parsedTransaction.decimals = decimals;
            parsedTransaction.approvalTarget = address;
            parsedTransaction.approvalTargetLabel = getAddressLabel(accounts, address);
            parsedTransaction.isApprovalUnlimited = amountWrapped.compareTo(Utils.MAX_UINT256) == 0;
            parsedTransaction.insufficientFundsForGasError = insufficientNativeFunds;
            parsedTransaction.insufficientFundsError = false;
            parsedTransaction.sameAddressError =
                    checkForSameAddressError(address, txInfo.fromAddress);
        } else if (txInfo.txType == TransactionType.SOLANA_SPL_TOKEN_TRANSFER
                || txInfo.txType
                        == TransactionType
                                .SOLANA_SPL_TOKEN_TRANSFER_WITH_ASSOCIATED_TOKEN_ACCOUNT_CREATION) {
            final int decimals = token != null ? token.decimals : Utils.SOL_DEFAULT_DECIMALS;
            final double price = Utils.getOrDefault(assetPrices, tokenSymbolLower, 0.0d);
            final double sendAmount = Utils.fromWei(value, decimals);
            final double sendAmountFiat = sendAmount * price;

            final double totalAmountFiat = parsedTransaction.getGasFeeFiat() + sendAmountFiat;

            final boolean insufficientNativeFunds =
                    parsedTransaction.getGasFee() > accountNativeBalance;
            final boolean insufficientTokenFunds = sendAmount > accountTokenBalance;

            parsedTransaction.recipient = to;
            parsedTransaction.recipientLabel = getAddressLabel(accounts, to);
            parsedTransaction.fiatValue = sendAmountFiat;
            parsedTransaction.fiatTotal = totalAmountFiat;
            parsedTransaction.nativeCurrencyTotal = sendAmountFiat / networkSpotPrice;
            parsedTransaction.value = sendAmount;
            parsedTransaction.symbol = token != null ? token.symbol : "";
            parsedTransaction.decimals = decimals;
            parsedTransaction.insufficientFundsError = insufficientTokenFunds;
            parsedTransaction.insufficientFundsForGasError = insufficientNativeFunds;
            parsedTransaction.contractAddressError =
                    checkForContractAddressError(
                            fullTokenList, solTxData != null ? solTxData.toWalletAddress : "");
            parsedTransaction.sameAddressError =
                    checkForSameAddressError(
                            solTxData != null ? solTxData.toWalletAddress : "", txInfo.fromAddress);
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
            final double price =
                    Utils.getOrDefault(
                            assetPrices, sellToken.symbol.toLowerCase(Locale.getDefault()), 0.0d);
            final String sellAmountWei =
                    sellAmountArg != null && !sellAmountArg.isEmpty() ? sellAmountArg : value;
            final double sellAmountFiat =
                    Utils.fromHexWei(sellAmountWei, sellToken.decimals) * price;

            final BlockchainToken buyToken = fillTokens[fillTokens.length - 1];
            final double buyAmount = Utils.fromHexWei(minBuyAmountArg, buyToken.decimals);

            final double totalAmountFiat = parsedTransaction.getGasFeeFiat() + sellAmountFiat;

            final boolean insufficientNativeFunds =
                    parsedTransaction.getGasFee() > accountNativeBalance;

            final double sellTokenBalance =
                    Utils.getOrDefault(
                            Utils.getOrDefault(
                                    blockchainTokensBalances,
                                    accountAddressLower,
                                    new HashMap<String, Double>()),
                            Utils.tokenToString(sellToken),
                            0.0d);

            final double sellAmount = Utils.fromHexWei(sellAmountWei, sellToken.decimals);
            final boolean insufficientTokenFunds = sellAmount > sellTokenBalance;

            parsedTransaction.recipient = to;
            parsedTransaction.recipientLabel = getAddressLabel(accounts, to);
            parsedTransaction.fiatValue = sellAmountFiat;
            parsedTransaction.fiatTotal = totalAmountFiat;
            parsedTransaction.nativeCurrencyTotal = sellAmountFiat / networkSpotPrice;
            parsedTransaction.value = sellAmount;
            parsedTransaction.symbol = sellToken.symbol;
            parsedTransaction.decimals = sellToken.decimals;
            parsedTransaction.insufficientFundsError = insufficientTokenFunds;
            parsedTransaction.insufficientFundsForGasError = insufficientNativeFunds;
            parsedTransaction.isSwap = true;
            parsedTransaction.sellToken = sellToken;
            parsedTransaction.sellAmount = sellAmount;
            parsedTransaction.buyToken = buyToken;
            parsedTransaction.minBuyAmount = buyAmount;
        } else {
            // The rest cases falls through to default
            final double price =
                    Utils.getOrDefault(
                            assetPrices, txNetwork.symbol.toLowerCase(Locale.ENGLISH), 0.0d);
            for (String k : assetPrices.keySet()) {
                String v = String.valueOf(assetPrices.get(k));
            }
            double sendAmount = 0;
            if (txInfo.txType == TransactionType.SOLANA_SYSTEM_TRANSFER) {
                sendAmount = Utils.fromWei(value, txNetwork.decimals);
            } else {
                sendAmount = Utils.fromHexWei(value, txNetwork.decimals);
            }

            final double sendAmountFiat = sendAmount * price;

            final double totalAmountFiat = parsedTransaction.getGasFeeFiat() + sendAmountFiat;

            parsedTransaction.recipient = to;
            parsedTransaction.recipientLabel = getAddressLabel(accounts, to);
            parsedTransaction.fiatValue = sendAmountFiat;
            parsedTransaction.fiatTotal = totalAmountFiat;
            parsedTransaction.nativeCurrencyTotal = sendAmountFiat / networkSpotPrice;
            parsedTransaction.value = sendAmount;
            parsedTransaction.symbol = txNetwork.symbol;
            parsedTransaction.decimals = txNetwork.decimals;
            parsedTransaction.insufficientFundsError =
                    parsedTransaction.value + parsedTransaction.getGasFee() > accountNativeBalance;
            parsedTransaction.insufficientFundsForGasError =
                    parsedTransaction.getGasFee() > accountNativeBalance;
            parsedTransaction.isSwap =
                    to.toLowerCase(Locale.getDefault()).equals(Utils.SWAP_EXCHANGE_PROXY);
        }

        return parsedTransaction;
    }

    private static String getAddressLabel(AccountInfo[] accounts, String address) {
        final AccountInfo account = Utils.findAccountByAddress(accounts, address);
        return account != null ? account.name : Utils.stripAccountAddress(address);
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

    public Pair<String, String> makeTxListItemTitles(Context context) {
        final DateFormat dateFormat =
                new SimpleDateFormat("yyyy-MM-dd hh:mm a", Locale.getDefault());
        final String strDate = dateFormat.format(new Date(this.createdTime.microseconds / 1000));

        String action = "";
        String detailInfo = "";
        String actionFiatValue = String.format(Locale.getDefault(), "%.2f", this.fiatValue);
        if (this.type == TransactionType.ERC20_TRANSFER) {
            action =
                    String.format(
                            context.getResources().getString(R.string.wallet_tx_info_sent),
                            this.senderLabel,
                            this.formatValueToDisplay(),
                            this.symbol,
                            actionFiatValue,
                            strDate);
            detailInfo = this.senderLabel + " -> " + this.recipientLabel;
        } else if (this.type == TransactionType.ERC721_TRANSFER_FROM
                || this.type == TransactionType.ERC721_SAFE_TRANSFER_FROM) {
            action =
                    String.format(
                            context.getResources().getString(R.string.wallet_tx_info_sent_erc721),
                            this.senderLabel,
                            this.symbol,
                            this.erc721TokenId,
                            strDate);
            detailInfo = this.senderLabel + " -> " + this.recipientLabel;
        } else if (this.type == TransactionType.ERC20_APPROVE) {
            action =
                    String.format(
                            context.getResources().getString(R.string.wallet_tx_info_approved),
                            this.senderLabel,
                            this.symbol,
                            strDate);
            detailInfo =
                    String.format(
                            context.getResources()
                                    .getString(R.string.wallet_tx_info_approved_unlimited),
                            this.symbol,
                            "0x Exchange Proxy"); // TODO: make unlimited work correctly
        } else if (this.isSwap) {
            action =
                    String.format(
                            context.getResources().getString(R.string.wallet_tx_info_swap),
                            this.senderLabel,
                            strDate);
            detailInfo = String.format(Locale.getDefault(), "%.4f", this.value);
            if (this.type == TransactionType.SOLANA_SWAP) {
                detailInfo += " SOL -> " + this.recipientLabel;
            } else {
                detailInfo += " ETH -> 0x Exchange Proxy";
            }
        } else {
            action =
                    String.format(
                            context.getResources().getString(R.string.wallet_tx_info_sent),
                            this.senderLabel,
                            this.formatValueToDisplay(),
                            this.symbol,
                            actionFiatValue,
                            strDate);
            detailInfo = this.senderLabel + " -> " + this.recipientLabel;
        }

        return new Pair<String, String>(action, detailInfo);
    }

    public String getHash() {
        return this.hash;
    }

    public String getNonce() {
        return this.nonce;
    }

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

    public String getSenderLabel() {
        return this.senderLabel;
    }

    public String getRecipient() {
        return this.recipient;
    }

    public String getRecipientLabel() {
        return this.recipientLabel;
    }

    public double getFiatValue() {
        return this.fiatValue;
    }

    public double getFiatTotal() {
        return this.fiatTotal;
    }

    public double getNativeCurrencyTotal() {
        return this.nativeCurrencyTotal;
    }

    public double getValue() {
        return this.value;
    }

    public String getSymbol() {
        return this.symbol;
    }

    public BlockchainToken getToken() {
        return this.token;
    }

    public int getDecimals() {
        return this.decimals;
    }

    public boolean getInsufficientFundsForGasError() {
        return this.insufficientFundsForGasError;
    }

    public boolean getInsufficientFundsError() {
        return this.insufficientFundsError;
    }

    public String getContractAddressError() {
        return this.contractAddressError;
    }

    public String getSameAddressError() {
        return this.sameAddressError;
    }

    public BlockchainToken getErc721BlockchainToken() {
        return this.erc721BlockchainToken;
    }

    public String getErc721TokenId() {
        return this.erc721TokenId;
    }

    public boolean getIsSwap() {
        return this.isSwap;
    }

    public String getApprovalTarget() {
        return this.approvalTarget;
    }

    public String getApprovalTargetLabel() {
        return this.approvalTargetLabel;
    }

    public boolean getIsApprovalUnlimited() {
        return this.isApprovalUnlimited;
    }

    public BlockchainToken getSellToken() {
        return this.sellToken;
    }

    public double getSellAmount() {
        return this.sellAmount;
    }

    public BlockchainToken getBuyToken() {
        return this.buyToken;
    }

    public double getMinBuyAmount() {
        return this.minBuyAmount;
    }
}
