/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.util;

import org.chromium.base.Log;
import org.chromium.brave_wallet.mojom.AccountInfo;
import org.chromium.brave_wallet.mojom.AssetPrice;
import org.chromium.brave_wallet.mojom.AssetPriceTimeframe;
import org.chromium.brave_wallet.mojom.AssetRatioService;
import org.chromium.brave_wallet.mojom.BlockchainToken;
import org.chromium.brave_wallet.mojom.CoinType;
import org.chromium.brave_wallet.mojom.JsonRpcService;
import org.chromium.brave_wallet.mojom.NetworkInfo;
import org.chromium.brave_wallet.mojom.ProviderError;
import org.chromium.mojo.bindings.Callbacks;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Locale;
import java.util.stream.Stream;

public class BalanceHelper {
    private static String TAG = "BalanceHelper";

    /**
     * Get assets balances for all accounts on selected network.
     */
    public static void getNativeAssetsBalances(JsonRpcService jsonRpcService,
            NetworkInfo selectedNetwork, AccountInfo[] accountInfos,
            Callbacks.Callback1<HashMap<String, Double>> callback) {
        if (jsonRpcService == null) return;
        HashMap<String, Double> nativeAssetsBalances = new HashMap<String, Double>();

        AsyncUtils.MultiResponseHandler balancesMultiResponse =
                new AsyncUtils.MultiResponseHandler(accountInfos.length);
        ArrayList<AsyncUtils.GetBalanceResponseBaseContext> contexts =
                new ArrayList<AsyncUtils.GetBalanceResponseBaseContext>();

        // Native balances
        for (AccountInfo accountInfo : accountInfos) {
            if (accountInfo.coin != selectedNetwork.coin) continue;

            // Get CoinType SOL balances
            if (selectedNetwork.coin == CoinType.SOL) {
                AsyncUtils.GetSolanaBalanceResponseContext context =
                        addBalanceResponseContext(contexts,
                                new AsyncUtils.GetSolanaBalanceResponseContext(
                                        balancesMultiResponse.singleResponseComplete),
                                accountInfo.address, null);
                jsonRpcService.getSolanaBalance(
                        accountInfo.address, selectedNetwork.chainId, context);
            } else if (accountInfo.coin == CoinType.FIL) {
                // TODO: FIL placeholder
            } else {
                AsyncUtils.GetBalanceResponseContext context = addBalanceResponseContext(contexts,
                        new AsyncUtils.GetBalanceResponseContext(
                                balancesMultiResponse.singleResponseComplete),
                        accountInfo.address, null);
                jsonRpcService.getBalance(
                        accountInfo.address, accountInfo.coin, selectedNetwork.chainId, context);
            }
        }

        balancesMultiResponse.setWhenAllCompletedAction(() -> {
            final int networkDecimals = selectedNetwork.decimals;
            for (AsyncUtils.GetBalanceResponseBaseContext context : contexts) {
                Double nativeAssetBalance = (context.error == ProviderError.SUCCESS)
                        ? Utils.fromHexWei(context.balance, networkDecimals)
                        : 0.0d;
                nativeAssetsBalances.put(context.accountAddress, nativeAssetBalance);
            }

            callback.call(nativeAssetsBalances);
        });
    }

    /**
     * Get assets balances for a list of tokens on all accounts.
     * Only collect balances for current network. Return a nested map that is grouped by accounts
     * first and then tokens. Native tokens (ETH, SOL, FIL) will be excluded.
     */
    public static void getBlockchainTokensBalances(JsonRpcService jsonRpcService,
            NetworkInfo selectedNetwork, AccountInfo[] accountInfos, BlockchainToken[] tokens,
            Callbacks.Callback1<HashMap<String, HashMap<String, Double>>> callback) {
        if (jsonRpcService == null) return;
        HashMap<String, HashMap<String, Double>> blockchainTokensBalances =
                new HashMap<String, HashMap<String, Double>>();
        // Remove native tokens
        List<BlockchainToken> tokensList = new ArrayList<BlockchainToken>();
        for (BlockchainToken token : tokens) {
            if (!Utils.isNativeToken(selectedNetwork, token)) tokensList.add(token);
        }
        tokens = tokensList.toArray(new BlockchainToken[0]);

        AsyncUtils.MultiResponseHandler balancesMultiResponse =
                new AsyncUtils.MultiResponseHandler(accountInfos.length * tokens.length);
        ArrayList<AsyncUtils.GetBalanceResponseBaseContext> contexts =
                new ArrayList<AsyncUtils.GetBalanceResponseBaseContext>();

        // Token balances
        for (AccountInfo accountInfo : accountInfos) {
            if (accountInfo.coin != selectedNetwork.coin) continue;

            for (BlockchainToken token : tokens) {
                if (accountInfo.coin == CoinType.ETH) {
                    if (selectedNetwork.chainId.equals(token.chainId)) {
                        if (token.isErc721) {
                            AsyncUtils.GetErc721TokenBalanceResponseContext context =
                                    addBalanceResponseContext(contexts,
                                            new AsyncUtils.GetErc721TokenBalanceResponseContext(
                                                    balancesMultiResponse.singleResponseComplete),
                                            accountInfo.address, token);
                            jsonRpcService.getErc721TokenBalance(token.contractAddress,
                                    token.tokenId != null ? token.tokenId : "", accountInfo.address,
                                    token.chainId, context);
                        } else {
                            AsyncUtils.GetErc20TokenBalanceResponseContext context =
                                    addBalanceResponseContext(contexts,
                                            new AsyncUtils.GetErc20TokenBalanceResponseContext(
                                                    balancesMultiResponse.singleResponseComplete),
                                            accountInfo.address, token);
                            jsonRpcService.getErc20TokenBalance(token.contractAddress,
                                    accountInfo.address, token.chainId, context);
                        }
                    }
                } else if (accountInfo.coin == CoinType.SOL) {
                    if (selectedNetwork.chainId.equals(token.chainId)) {
                        AsyncUtils.GetSplTokenAccountBalanceResponseContext context =
                                addBalanceResponseContext(contexts,
                                        new AsyncUtils.GetSplTokenAccountBalanceResponseContext(
                                                balancesMultiResponse.singleResponseComplete),
                                        accountInfo.address, token);
                        jsonRpcService.getSplTokenAccountBalance(
                                accountInfo.address, token.contractAddress, token.chainId, context);
                    }
                } else {
                    // TODO: FIL placeholder
                }
            }
        }

        balancesMultiResponse.setWhenAllCompletedAction(() -> {
            for (AsyncUtils.GetBalanceResponseBaseContext context : contexts) {
                final String tokenKey = Utils.tokenToString(context.userAsset);
                final int decimals = (context.userAsset.decimals != 0 || context.userAsset.isErc721)
                        ? context.userAsset.decimals
                        : selectedNetwork.decimals;
                Double tokenBalance =
                        (context.error == ProviderError.SUCCESS && context.balance != null
                                && !context.balance.isEmpty())
                        ? Utils.fromHexWei(context.balance, decimals)
                        : 0.0d;
                if (blockchainTokensBalances.containsKey(context.accountAddress)) {
                    blockchainTokensBalances.get(context.accountAddress)
                            .put(tokenKey, tokenBalance);
                } else {
                    HashMap<String, Double> perAccountBlockchainTokensBalances =
                            new HashMap<String, Double>();
                    perAccountBlockchainTokensBalances.put(tokenKey, tokenBalance);
                    blockchainTokensBalances.put(
                            context.accountAddress, perAccountBlockchainTokensBalances);
                }
            }

            callback.call(blockchainTokensBalances);
        });
    }

    private static <T extends AsyncUtils.GetBalanceResponseBaseContext> T addBalanceResponseContext(
            ArrayList<AsyncUtils.GetBalanceResponseBaseContext> contexts, T context,
            String accountAddress, BlockchainToken token) {
        context.accountAddress = accountAddress.toLowerCase(Locale.getDefault());
        if (token != null) context.userAsset = token;
        contexts.add(context);
        return context;
    }
}
