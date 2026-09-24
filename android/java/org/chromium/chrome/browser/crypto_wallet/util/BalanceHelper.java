/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.util;

import androidx.annotation.NonNull;

import org.chromium.brave_wallet.mojom.AccountInfo;
import org.chromium.brave_wallet.mojom.BlockchainToken;
import org.chromium.brave_wallet.mojom.CoinType;
import org.chromium.brave_wallet.mojom.JsonRpcService;
import org.chromium.brave_wallet.mojom.NetworkInfo;
import org.chromium.brave_wallet.mojom.ProviderError;
import org.chromium.chrome.browser.crypto_wallet.util.AsyncUtils.GetBalanceResponseBaseContext;
import org.chromium.chrome.browser.crypto_wallet.util.AsyncUtils.GetBalanceResponseContext;
import org.chromium.chrome.browser.crypto_wallet.util.AsyncUtils.GetErc20TokenBalanceResponseContext;
import org.chromium.chrome.browser.crypto_wallet.util.AsyncUtils.GetErc721TokenBalanceResponseContext;
import org.chromium.chrome.browser.crypto_wallet.util.AsyncUtils.GetSolanaBalanceResponseContext;
import org.chromium.chrome.browser.crypto_wallet.util.AsyncUtils.GetSplTokenAccountBalanceResponseContext;
import org.chromium.chrome.browser.crypto_wallet.util.AsyncUtils.MultiResponseHandler;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Locale;

public class BalanceHelper {
    /** Get assets balances for all accounts on selected network. */
    public static void getNativeAssetsBalances(
            @NonNull final JsonRpcService jsonRpcService,
            @NonNull final NetworkInfo selectedNetwork,
            @NonNull final AccountInfo[] accounts,
            @NonNull final AsyncUtils.Callback2<Integer, HashMap<String, Double>> callback) {
        HashMap<String, Double> nativeAssetsBalances = new HashMap<>();

        MultiResponseHandler balancesMultiResponse = new MultiResponseHandler(accounts.length);
        ArrayList<GetBalanceResponseBaseContext> contexts = new ArrayList<>();

        if (selectedNetwork.coin == CoinType.BTC || selectedNetwork.coin == CoinType.ZEC) {
            callback.call(selectedNetwork.coin, nativeAssetsBalances);
            return;
        }

        // Native balances
        for (AccountInfo accountInfo : accounts) {
            if (accountInfo == null || accountInfo.accountId.coin != selectedNetwork.coin) continue;

            // Get CoinType SOL balances
            if (selectedNetwork.coin == CoinType.SOL) {
                GetSolanaBalanceResponseContext context =
                        addBalanceResponseContext(
                                contexts,
                                new GetSolanaBalanceResponseContext(
                                        balancesMultiResponse.singleResponseComplete),
                                accountInfo.address,
                                null);
                jsonRpcService.getSolanaBalance(
                        accountInfo.address, selectedNetwork.chainId, context);
            } else {
                GetBalanceResponseContext context =
                        addBalanceResponseContext(
                                contexts,
                                new GetBalanceResponseContext(
                                        balancesMultiResponse.singleResponseComplete),
                                accountInfo.address,
                                null);
                jsonRpcService.getBalance(
                        accountInfo.address,
                        accountInfo.accountId.coin,
                        selectedNetwork.chainId,
                        context);
            }
        }

        balancesMultiResponse.setWhenAllCompletedAction(
                () -> {
                    final int networkDecimals = selectedNetwork.decimals;
                    for (GetBalanceResponseBaseContext context : contexts) {
                        Double nativeAssetBalance =
                                (context.error == ProviderError.SUCCESS)
                                        ? Utils.getBalanceForCoinType(
                                                selectedNetwork.coin,
                                                networkDecimals,
                                                context.balance)
                                        : 0.0d;
                        nativeAssetsBalances.put(context.accountAddress, nativeAssetBalance);
                    }

                    callback.call(selectedNetwork.coin, nativeAssetsBalances);
                });
    }

    /**
     * Get assets balances for a list of tokens on all accounts. Only collect balances for current
     * network. Return a nested map that is grouped by accounts first and then tokens. Native tokens
     * (ETH, SOL, FIL) will be excluded.
     */
    public static void getBlockchainTokensBalances(
            JsonRpcService jsonRpcService,
            NetworkInfo selectedNetwork,
            AccountInfo[] accountInfos,
            BlockchainToken[] tokens,
            AsyncUtils.Callback2<Integer, HashMap<String, HashMap<String, Double>>> callback) {
        if (jsonRpcService == null) return;
        HashMap<String, HashMap<String, Double>> blockchainTokensBalances =
                new HashMap<String, HashMap<String, Double>>();
        // Remove native tokens
        List<BlockchainToken> tokensList = new ArrayList<BlockchainToken>();
        for (BlockchainToken token : tokens) {
            if (!Utils.isNativeToken(selectedNetwork, token)) tokensList.add(token);
        }
        tokens = tokensList.toArray(new BlockchainToken[0]);

        MultiResponseHandler balancesMultiResponse =
                new MultiResponseHandler(accountInfos.length * tokens.length);
        ArrayList<GetBalanceResponseBaseContext> contexts =
                new ArrayList<GetBalanceResponseBaseContext>();

        // Token balances
        for (AccountInfo accountInfo : accountInfos) {
            if (accountInfo.accountId.coin != selectedNetwork.coin) continue;

            for (BlockchainToken token : tokens) {
                if (accountInfo.accountId.coin == CoinType.ETH) {
                    if (selectedNetwork.chainId.equals(token.chainId)) {
                        if (token.isErc721) {
                            GetErc721TokenBalanceResponseContext context =
                                    addBalanceResponseContext(
                                            contexts,
                                            new GetErc721TokenBalanceResponseContext(
                                                    balancesMultiResponse.singleResponseComplete),
                                            accountInfo.address,
                                            token);
                            jsonRpcService.getErc721TokenBalance(
                                    token.contractAddress,
                                    token.tokenId != null ? token.tokenId : "",
                                    accountInfo.address,
                                    token.chainId,
                                    context);
                        } else {
                            GetErc20TokenBalanceResponseContext context =
                                    addBalanceResponseContext(
                                            contexts,
                                            new GetErc20TokenBalanceResponseContext(
                                                    balancesMultiResponse.singleResponseComplete),
                                            accountInfo.address,
                                            token);
                            jsonRpcService.getErc20TokenBalance(
                                    token.contractAddress,
                                    accountInfo.address,
                                    token.chainId,
                                    context);
                        }
                    }
                } else if (accountInfo.accountId.coin == CoinType.SOL) {
                    if (selectedNetwork.chainId.equals(token.chainId)) {
                        GetSplTokenAccountBalanceResponseContext context =
                                addBalanceResponseContext(
                                        contexts,
                                        new GetSplTokenAccountBalanceResponseContext(
                                                balancesMultiResponse.singleResponseComplete),
                                        accountInfo.address,
                                        token);
                        jsonRpcService.getSplTokenAccountBalance(
                                accountInfo.address, token.contractAddress, token.chainId, context);
                    }
                } else {
                    // TODO: FIL placeholder
                }
            }
        }

        balancesMultiResponse.setWhenAllCompletedAction(
                () -> {
                    for (GetBalanceResponseBaseContext context : contexts) {
                        final String tokenKey = Utils.tokenToString(context.userAsset);
                        final int decimals =
                                (context.userAsset.decimals != 0 || context.userAsset.isErc721)
                                        ? context.userAsset.decimals
                                        : (context.userAsset.coin == CoinType.SOL
                                                ? context.decimals
                                                : selectedNetwork.decimals);
                        Double tokenBalance =
                                (context.error == ProviderError.SUCCESS
                                                && context.balance != null
                                                && !context.balance.isEmpty())
                                        ? Utils.getBalanceForCoinType(
                                                selectedNetwork.coin, decimals, context.balance)
                                        : 0.0d;
                        if (blockchainTokensBalances.containsKey(context.accountAddress)) {
                            blockchainTokensBalances
                                    .get(context.accountAddress)
                                    .put(tokenKey, tokenBalance);
                        } else {
                            HashMap<String, Double> perAccountBlockchainTokensBalances =
                                    new HashMap<String, Double>();
                            perAccountBlockchainTokensBalances.put(tokenKey, tokenBalance);
                            blockchainTokensBalances.put(
                                    context.accountAddress, perAccountBlockchainTokensBalances);
                        }
                    }

                    callback.call(selectedNetwork.coin, blockchainTokensBalances);
                });
    }

    private static <T extends GetBalanceResponseBaseContext> T addBalanceResponseContext(
            ArrayList<GetBalanceResponseBaseContext> contexts,
            T context,
            String accountAddress,
            BlockchainToken token) {
        context.accountAddress = accountAddress.toLowerCase(Locale.ENGLISH);
        if (token != null) context.userAsset = token;
        contexts.add(context);
        return context;
    }
}
