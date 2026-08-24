/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.util;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.annotation.Config;

import org.chromium.base.test.BaseRobolectricTestRunner;
import org.chromium.brave_wallet.mojom.AccountId;
import org.chromium.brave_wallet.mojom.AccountInfo;
import org.chromium.brave_wallet.mojom.BlockchainToken;
import org.chromium.brave_wallet.mojom.BtcTxData;
import org.chromium.brave_wallet.mojom.CoinType;
import org.chromium.brave_wallet.mojom.NetworkInfo;
import org.chromium.brave_wallet.mojom.TransactionInfo;
import org.chromium.brave_wallet.mojom.TransactionType;
import org.chromium.brave_wallet.mojom.TxData;
import org.chromium.brave_wallet.mojom.TxData1559;
import org.chromium.brave_wallet.mojom.TxDataUnion;

import java.util.ArrayList;

/** Tests for the balance checks of {@link TransactionUtils}. */
@RunWith(BaseRobolectricTestRunner.class)
@Config(manifest = Config.NONE)
public class TransactionUtilsTest {
    private static final String ACCOUNT_ADDRESS = "0xd8da6bf26964af9d7eed9e03e53415d37aa96045";
    private static final String RECIPIENT_ADDRESS = "0xbc7c0ec6b0e1a4f4f66b6c58f1a3e2d5a0a3f5f2";
    // 1 ETH in wei.
    private static final String ONE_ETH = "0xde0b6b3a7640000";
    // 21000 gas at 1 Gwei, that is 0.000021 ETH.
    private static final String GAS_LIMIT = "0x5208";
    private static final String GAS_PRICE = "0x3b9aca00";
    private static final double GAS_FEE = 0.000021d;

    @Test
    public void hasInsufficientBalance_wholeNativeBalanceSent_isInsufficient() {
        // The fee is paid with the asset being sent, so sending the whole balance leaves nothing
        // to pay it with.
        TransactionInfo txInfo = createEthTransaction(TransactionType.ETH_SEND, ONE_ETH);
        ParsedTransaction parsedTx = parse(txInfo, createEthNetwork());

        assertTrue(TransactionUtils.hasInsufficientBalance(txInfo, parsedTx, 1.0d, null));
    }

    @Test
    public void hasInsufficientBalance_nativeBalanceCoversAmountAndFee_isSufficient() {
        TransactionInfo txInfo = createEthTransaction(TransactionType.ETH_SEND, ONE_ETH);
        ParsedTransaction parsedTx = parse(txInfo, createEthNetwork());

        assertFalse(
                TransactionUtils.hasInsufficientBalance(txInfo, parsedTx, 1.0d + GAS_FEE, null));
    }

    @Test
    public void hasInsufficientBalance_unknownNativeBalance_isSufficient() {
        TransactionInfo txInfo = createEthTransaction(TransactionType.ETH_SEND, ONE_ETH);
        ParsedTransaction parsedTx = parse(txInfo, createEthNetwork());

        assertFalse(TransactionUtils.hasInsufficientBalance(txInfo, parsedTx, null, null));
    }

    @Test
    public void hasInsufficientBalance_tokenTransferAboveTokenBalance_isInsufficient() {
        TransactionInfo txInfo = createEthTransaction(TransactionType.ERC20_TRANSFER, "0x0");
        txInfo.txArgs = new String[] {RECIPIENT_ADDRESS, ONE_ETH};
        ParsedTransaction parsedTx = parse(txInfo, createEthNetwork());

        assertTrue(TransactionUtils.hasInsufficientBalance(txInfo, parsedTx, 1.0d, 0.5d));
        assertFalse(TransactionUtils.hasInsufficientBalance(txInfo, parsedTx, 1.0d, 1.0d));
    }

    @Test
    public void hasInsufficientBalance_tokenApprovalAboveTokenBalance_isSufficient() {
        // Spending can be approved for more tokens than the account holds.
        TransactionInfo txInfo = createEthTransaction(TransactionType.ERC20_APPROVE, "0x0");
        txInfo.txArgs = new String[] {RECIPIENT_ADDRESS, ONE_ETH};
        ParsedTransaction parsedTx = parse(txInfo, createEthNetwork());

        assertFalse(TransactionUtils.hasInsufficientBalance(txInfo, parsedTx, 1.0d, 0d));
    }

    @Test
    public void hasInsufficientBalance_utxoTransaction_isSufficient() {
        // UTXO based transactions are created only once inputs covering both the amount and the
        // fee have been found.
        TransactionInfo txInfo = createBtcTransaction();
        ParsedTransaction parsedTx = parse(txInfo, createBtcNetwork());

        assertFalse(TransactionUtils.hasInsufficientBalance(txInfo, parsedTx, 0d, null));
        assertFalse(TransactionUtils.hasInsufficientBalanceForGas(txInfo, parsedTx, 0d));
    }

    @Test
    public void hasInsufficientBalanceForGas_nativeBalanceBelowFee_isInsufficient() {
        TransactionInfo txInfo = createEthTransaction(TransactionType.ERC20_TRANSFER, "0x0");
        txInfo.txArgs = new String[] {RECIPIENT_ADDRESS, ONE_ETH};
        ParsedTransaction parsedTx = parse(txInfo, createEthNetwork());

        assertTrue(TransactionUtils.hasInsufficientBalanceForGas(txInfo, parsedTx, GAS_FEE / 2.0d));
        assertFalse(TransactionUtils.hasInsufficientBalanceForGas(txInfo, parsedTx, GAS_FEE));
    }

    private static ParsedTransaction parse(TransactionInfo txInfo, NetworkInfo network) {
        return ParsedTransaction.parseTransaction(
                txInfo,
                network,
                new AccountInfo[] {createAccount(network.coin)},
                new ArrayList<>(),
                /* solFeeEstimatesFee= */ 0,
                new BlockchainToken[0]);
    }

    private static TransactionInfo createEthTransaction(
            @TransactionType.EnumType int txType, String value) {
        TxData baseData = new TxData();
        baseData.chainId = "0x1";
        baseData.nonce = "0x1";
        baseData.gasPrice = GAS_PRICE;
        baseData.gasLimit = GAS_LIMIT;
        baseData.to = RECIPIENT_ADDRESS;
        baseData.value = value;
        baseData.data = new byte[0];

        TxData1559 txData1559 = new TxData1559();
        txData1559.baseData = baseData;
        txData1559.maxPriorityFeePerGas = "";
        txData1559.maxFeePerGas = "";

        TxDataUnion txDataUnion = new TxDataUnion();
        txDataUnion.setEthTxData1559(txData1559);

        return createTransaction(CoinType.ETH, "0x1", txType, txDataUnion);
    }

    private static TransactionInfo createBtcTransaction() {
        BtcTxData btcTxData = new BtcTxData();
        btcTxData.to = RECIPIENT_ADDRESS;
        btcTxData.amount = 100000L;
        btcTxData.fee = 1000L;

        TxDataUnion txDataUnion = new TxDataUnion();
        txDataUnion.setBtcTxData(btcTxData);

        return createTransaction(
                CoinType.BTC, "bitcoin_mainnet", TransactionType.OTHER, txDataUnion);
    }

    private static TransactionInfo createTransaction(
            @CoinType.EnumType int coin,
            String chainId,
            @TransactionType.EnumType int txType,
            TxDataUnion txDataUnion) {
        TransactionInfo txInfo = new TransactionInfo();
        txInfo.id = "tx-id";
        txInfo.txHash = "";
        txInfo.chainId = chainId;
        txInfo.txType = txType;
        txInfo.txArgs = new String[0];
        txInfo.txParams = new String[0];
        txInfo.txDataUnion = txDataUnion;
        txInfo.fromAccountId = createAccountId(coin);
        return txInfo;
    }

    private static AccountInfo createAccount(@CoinType.EnumType int coin) {
        AccountInfo accountInfo = new AccountInfo();
        accountInfo.accountId = createAccountId(coin);
        accountInfo.address = ACCOUNT_ADDRESS;
        accountInfo.name = "Account 1";
        return accountInfo;
    }

    private static AccountId createAccountId(@CoinType.EnumType int coin) {
        AccountId accountId = new AccountId();
        accountId.coin = coin;
        accountId.address = ACCOUNT_ADDRESS;
        accountId.uniqueKey = "unique-key";
        return accountId;
    }

    private static NetworkInfo createEthNetwork() {
        return createNetwork("0x1", "Ethereum Mainnet", "ETH", 18, CoinType.ETH);
    }

    private static NetworkInfo createBtcNetwork() {
        return createNetwork("bitcoin_mainnet", "Bitcoin Mainnet", "BTC", 8, CoinType.BTC);
    }

    private static NetworkInfo createNetwork(
            String chainId,
            String chainName,
            String symbol,
            int decimals,
            @CoinType.EnumType int coin) {
        NetworkInfo network = new NetworkInfo();
        network.chainId = chainId;
        network.chainName = chainName;
        network.symbol = symbol;
        network.symbolName = chainName;
        network.decimals = decimals;
        network.coin = coin;
        network.blockExplorerUrls = new String[0];
        network.iconUrls = new String[0];
        network.supportedKeyrings = new int[0];
        return network;
    }
}
