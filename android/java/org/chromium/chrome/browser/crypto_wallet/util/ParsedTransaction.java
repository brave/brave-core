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

/**
 * Parses a {@link TransactionInfo} into a UI-friendly representation containing resolved token
 * metadata, formatted values, fiat totals, and fee breakdowns. Supports Ethereum (including
 * ERC-20/721/1155 transfers, approvals, and Uniswap swaps), Solana (SPL transfers, dApp
 * instructions), Filecoin, Bitcoin, ZCash, and Cardano transaction types.
 *
 * <p>Instances are created via the static factory method {@link #parseTransaction}.
 *
 * <p>This is the Java counterpart of the TypeScript {@code ParsedTransaction} interface defined in
 * {@code components/brave_wallet_ui/utils/tx-utils.ts}.
 */
@NullMarked
public class ParsedTransaction extends ParsedTransactionFees {
    // Strings are initialized to empty string instead of null, as the default value from mojo
    // Common fields
    public double marketPrice;
    private String mHash = "";
    private String mNonce = "";
    @Nullable private TimeDelta mCreatedTime;
    private int mStatus; // mojo loses enum type info in struct
    private int mType; // mojo loses enum type info in struct
    private String mSender = "";
    private String mRecipient = "";
    private double mFiatTotal;
    private double mValue;
    private String mSymbol = "";
    @Nullable private BlockchainToken mToken;
    private int mDecimals;
    @Nullable private BlockchainToken mErc721BlockchainToken;
    private boolean mIsSwap;

    // ZCash
    private boolean mShielded;

    // Swap
    @Nullable private BlockchainToken mSellToken;
    private double mSellAmount;
    @Nullable private BlockchainToken mBuyToken;
    private double mMinBuyAmount;

    // Solana
    public boolean isSolanaDappTransaction;
    private boolean mSolChangeOfOwnership;

    // Matches up to 40-character hex segments in a Uniswap encoded swap path (after stripping the
    // "0x" prefix). Each segment represents a token contract address.
    private static final Pattern UNISWAP_PATH_SEGMENT_PATTERN = Pattern.compile("(.{1,40})");

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
            BlockchainToken[] fullTokenList, @Nullable String contractAddress) {
        if (contractAddress == null) return null;

        for (BlockchainToken token : fullTokenList) {
            if (token.contractAddress.equalsIgnoreCase(contractAddress)) return token;
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
        Double networkSpotPrice = Utils.getPrice(
                assetPrices, nativeAsset.coin, nativeAsset.chainId, nativeAsset.contractAddress);

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
        parsedTransaction.mHash = txInfo.txHash;
        parsedTransaction.mType = txInfo.txType;
        parsedTransaction.mNonce = nonce;
        parsedTransaction.mToken = token;
        parsedTransaction.mCreatedTime = txInfo.createdTime;
        parsedTransaction.mStatus = txInfo.txStatus;
        parsedTransaction.mSender = sender;
        parsedTransaction.isSolanaDappTransaction =
                WalletConstants.SOLANA_DAPPS_TRANSACTION_TYPES.contains(txInfo.txType);
        parsedTransaction.marketPrice = networkSpotPrice;
        if (zecTxData != null && zecTxData.useShieldedPool) {
            parsedTransaction.mShielded = true;
        }

        int txType = txInfo.txType;
        if (txType == TransactionType.SOLANA_DAPP_SIGN_TRANSACTION
                || txType == TransactionType.SOLANA_DAPP_SIGN_AND_SEND_TRANSACTION
                || txType == TransactionType.SOLANA_SWAP
                || (txType == TransactionType.OTHER && solTxData != null)) {
            if (solTxData == null) {
                parsedTransaction.mRecipient = "";
                parsedTransaction.mFiatTotal = 0;
                parsedTransaction.mDecimals = 0;
                parsedTransaction.mSymbol = "";
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
                    parsedTransaction.mSolChangeOfOwnership = true;
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
            final double price = Utils.getPriceForAsset(assetPrices, token);
            final double sendAmount =
                    Utils.getBalanceForCoinType(
                            TransactionUtils.getCoinFromTxDataUnion(txDataUnion),
                            decimals,
                            lamportTransferredAmount.toPlainString());
            final double sendAmountFiat = sendAmount * price;
            final double totalAmountFiat = parsedTransaction.getGasFeeFiat() + sendAmountFiat;

            parsedTransaction.mRecipient = to;
            parsedTransaction.mFiatTotal = totalAmountFiat;
            parsedTransaction.mValue = sendAmount;
            parsedTransaction.mSymbol = token != null ? token.symbol : "";
            parsedTransaction.mDecimals = Utils.SOL_DEFAULT_DECIMALS;
            parsedTransaction.mIsSwap = txType == TransactionType.SOLANA_SWAP;
            if (parsedTransaction.mIsSwap) {
                parsedTransaction.mSellToken = nativeAsset;
                parsedTransaction.mBuyToken = nativeAsset;
            }
        } else if (txInfo.txType == TransactionType.ERC20_TRANSFER && txInfo.txArgs.length > 1) {
            final String address = txInfo.txArgs[0];
            final String amount = txInfo.txArgs[1];
            final int decimals = token != null ? token.decimals : Utils.ETH_DEFAULT_DECIMALS;
            final double price = Utils.getPriceForAsset(assetPrices, token);
            final double sendAmount = Utils.fromHexWei(amount, decimals);
            final double sendAmountFiat = sendAmount * price;

            final double totalAmountFiat = parsedTransaction.getGasFeeFiat() + sendAmountFiat;

            parsedTransaction.mRecipient = address;
            parsedTransaction.mFiatTotal = totalAmountFiat;
            parsedTransaction.mValue = sendAmount;
            parsedTransaction.mSymbol = token != null ? token.symbol : "";
            parsedTransaction.mDecimals = decimals;
        } else if ((txInfo.txType == TransactionType.ERC721_TRANSFER_FROM
                        || txInfo.txType == TransactionType.ERC721_SAFE_TRANSFER_FROM)
                && txInfo.txArgs.length > 2) {
            final String toAddress = txInfo.txArgs[1];

            final double totalAmountFiat = parsedTransaction.getGasFeeFiat();

            parsedTransaction.mRecipient = toAddress;
            parsedTransaction.mFiatTotal = totalAmountFiat;
            parsedTransaction.mValue = 1; // Can only send 1 erc721 at a time
            parsedTransaction.mSymbol = token != null ? token.symbol : "";
            parsedTransaction.mDecimals = 0;
            parsedTransaction.mErc721BlockchainToken = token;
        } else if (txInfo.txType == TransactionType.ERC20_APPROVE && txInfo.txArgs.length > 1) {
            final String amount = txInfo.txArgs[1];

            final int decimals = token != null ? token.decimals : Utils.ETH_DEFAULT_DECIMALS;
            final double totalAmountFiat = parsedTransaction.getGasFeeFiat();
            parsedTransaction.mRecipient = to;
            parsedTransaction.mFiatTotal = totalAmountFiat;
            parsedTransaction.mValue = Utils.fromHexWei(amount, decimals);
            parsedTransaction.mSymbol = token != null ? token.symbol : "";
            parsedTransaction.mDecimals = decimals;
        } else if (txInfo.txType == TransactionType.SOLANA_SPL_TOKEN_TRANSFER
                || txInfo.txType
                        == TransactionType
                                .SOLANA_SPL_TOKEN_TRANSFER_WITH_ASSOCIATED_TOKEN_ACCOUNT_CREATION) {
            final int decimals = token != null ? token.decimals : Utils.SOL_DEFAULT_DECIMALS;
            final double price = Utils.getPriceForAsset(assetPrices, token);
            final double sendAmount = Utils.fromWei(value, decimals);
            final double sendAmountFiat = sendAmount * price;

            final double totalAmountFiat = parsedTransaction.getGasFeeFiat() + sendAmountFiat;

            parsedTransaction.mRecipient = to;
            parsedTransaction.mFiatTotal = totalAmountFiat;
            parsedTransaction.mValue = sendAmount;
            parsedTransaction.mSymbol = token != null ? token.symbol : "";
            parsedTransaction.mDecimals = decimals;
        } else if (txInfo.txType == TransactionType.ETH_SWAP && txInfo.txArgs.length > 2) {
            final String fillPath = txInfo.txArgs[0];
            final String sellAmountArg = txInfo.txArgs[1];
            final String minBuyAmountArg = txInfo.txArgs[2];

            final List<BlockchainToken> fillTokensList =
                    getUniswapBlockchainTokens(
                            fullTokenList, fillPath, nativeAsset);

            final BlockchainToken sellToken =
                    fillTokensList.size() == 1
                            ? nativeAsset
                            : fillTokensList.get(0);
            final double price = Utils.getPrice(
                    assetPrices, sellToken.coin,
                    sellToken.chainId,
                    sellToken.contractAddress);
            final String sellAmountWei =
                    sellAmountArg != null && !sellAmountArg.isEmpty() ? sellAmountArg : value;
            final double sellAmountFiat =
                    Utils.fromHexWei(sellAmountWei, sellToken.decimals) * price;

            final BlockchainToken buyToken =
                    fillTokensList.get(fillTokensList.size() - 1);
            final double buyAmount =
                    Utils.fromHexWei(minBuyAmountArg, buyToken.decimals);

            final double totalAmountFiat = parsedTransaction.getGasFeeFiat() + sellAmountFiat;

            final double sellAmount = Utils.fromHexWei(sellAmountWei, sellToken.decimals);

            parsedTransaction.mRecipient = to;
            parsedTransaction.mFiatTotal = totalAmountFiat;
            parsedTransaction.mValue = sellAmount;
            parsedTransaction.mSymbol = sellToken.symbol;
            parsedTransaction.mDecimals = sellToken.decimals;
            parsedTransaction.mIsSwap = true;
            parsedTransaction.mSellToken = sellToken;
            parsedTransaction.mSellAmount = sellAmount;
            parsedTransaction.mBuyToken = buyToken;
            parsedTransaction.mMinBuyAmount = buyAmount;
        } else {
            // The rest cases falls through to default
            final double price = Utils.getPrice(
                    assetPrices, nativeAsset.coin,
                    nativeAsset.chainId,
                    nativeAsset.contractAddress);
            double sendAmount;
            if (txInfo.txType == TransactionType.SOLANA_SYSTEM_TRANSFER || zecTxData != null) {
                sendAmount = Utils.fromWei(value, txNetwork.decimals);
            } else {
                sendAmount = Utils.fromHexWei(value, txNetwork.decimals);
            }

            final double sendAmountFiat = sendAmount * price;

            final double totalAmountFiat = parsedTransaction.getGasFeeFiat() + sendAmountFiat;

            parsedTransaction.mRecipient = to;
            parsedTransaction.mFiatTotal = totalAmountFiat;
            parsedTransaction.mValue = sendAmount;
            parsedTransaction.mSymbol = txNetwork.symbol;
            parsedTransaction.mDecimals = txNetwork.decimals;
            parsedTransaction.mIsSwap =
                    to.toLowerCase(Locale.getDefault()).equals(Utils.SWAP_EXCHANGE_PROXY);
        }

        return parsedTransaction;
    }

    private static List<BlockchainToken> getUniswapBlockchainTokens(
            BlockchainToken[] fullTokenList,
            String fillPath,
            BlockchainToken nativeAsset) {
        final Matcher matcher = UNISWAP_PATH_SEGMENT_PATTERN.matcher(fillPath.substring(2));
        final List<BlockchainToken> fillTokensList = new ArrayList<>();
        while (matcher.find()) {
            String address = "0x" + matcher.group();
            final BlockchainToken thisToken = findToken(fullTokenList, address);
            final boolean isEthSwapContract = address.equals(Utils.ETHEREUM_CONTRACT_FOR_SWAP);
            final BlockchainToken tokenToAdd =
                    (isEthSwapContract || thisToken == null)
                            ? nativeAsset
                            : thisToken;
            fillTokensList.add(tokenToAdd);
        }
        return fillTokensList;
    }

    // TODO (Wengling): change it to reflect desktop Amount.format
    public String formatValueToDisplay() {
        if (this.mType == TransactionType.ERC20_TRANSFER) {
            return String.format(Locale.getDefault(), "%.4f", this.mValue);
        } else if (this.mType == TransactionType.ERC721_TRANSFER_FROM
                || this.mType == TransactionType.ERC721_SAFE_TRANSFER_FROM) {
            return "1";
        } else if (this.mType == TransactionType.ERC20_APPROVE) {
            return "0.0000";
        } else if (this.mIsSwap) {
            return String.format(Locale.getDefault(), "%.4f", this.mValue);
        } else {
            String sVal = String.format(Locale.getDefault(), "%.9f", mValue);
            // Show amount without trailing zeros
            return !sVal.contains(".") ? sVal : sVal.replaceAll("0*$", "").replaceAll("\\.$", "");
        }
    }

    public String getHash() {
        return this.mHash;
    }

    public String getNonce() {
        return this.mNonce;
    }

    @Nullable
    public TimeDelta getCreatedTime() {
        return this.mCreatedTime;
    }

    public int getStatus() {
        return this.mStatus;
    }

    public int getType() {
        return this.mType;
    }

    public String getSender() {
        return this.mSender;
    }

    public String getRecipient() {
        return this.mRecipient;
    }

    public double getFiatTotal() {
        return this.mFiatTotal;
    }

    public double getValue() {
        return this.mValue;
    }

    public String getSymbol() {
        return this.mSymbol;
    }

    @Nullable
    public BlockchainToken getToken() {
        return this.mToken;
    }

    public int getDecimals() {
        return this.mDecimals;
    }

    @Nullable
    public BlockchainToken getErc721BlockchainToken() {
        return this.mErc721BlockchainToken;
    }

    public boolean getIsSwap() {
        return this.mIsSwap;
    }

    @Nullable
    public BlockchainToken getSellToken() {
        return this.mSellToken;
    }

    public double getSellAmount() {
        return this.mSellAmount;
    }

    @Nullable
    public BlockchainToken getBuyToken() {
        return this.mBuyToken;
    }

    public double getMinBuyAmount() {
        return this.mMinBuyAmount;
    }

    public boolean isSolChangeOfOwnership() {
        return mSolChangeOfOwnership;
    }

    public boolean isShielded() {
        return mShielded;
    }
}
