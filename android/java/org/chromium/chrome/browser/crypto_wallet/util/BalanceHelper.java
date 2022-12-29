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
import org.chromium.brave_wallet.mojom.BlockchainRegistry;
import org.chromium.brave_wallet.mojom.BlockchainToken;
import org.chromium.brave_wallet.mojom.BraveWalletService;
import org.chromium.brave_wallet.mojom.CoinType;
import org.chromium.brave_wallet.mojom.JsonRpcService;
import org.chromium.brave_wallet.mojom.KeyringService;
import org.chromium.brave_wallet.mojom.NetworkInfo;
import org.chromium.brave_wallet.mojom.ProviderError;
import org.chromium.chrome.browser.BraveConfig;
import org.chromium.chrome.browser.BraveLocalState;
import org.chromium.chrome.browser.crypto_wallet.activities.BraveWalletBaseActivity;
import org.chromium.chrome.browser.crypto_wallet.util.AssetUtils;
import org.chromium.chrome.browser.preferences.BravePref;
import org.chromium.chrome.browser.preferences.BravePrefServiceBridge;
import org.chromium.mojo.bindings.Callbacks;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Locale;
import java.util.Map;

public class BalanceHelper {
    private static String TAG = "BalanceHelper";

    /**
     * Get assets balances for all accounts on selected network.
     */
    public static void getNativeAssetsBalances(JsonRpcService jsonRpcService,
            NetworkInfo selectedNetwork, AccountInfo[] accountInfos,
            Callbacks.Callback2<Integer, HashMap<String, Double>> callback) {
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
                        ? Utils.getBalanceForCoinType(
                                selectedNetwork.coin, networkDecimals, context.balance)
                        : 0.0d;
                nativeAssetsBalances.put(context.accountAddress, nativeAssetBalance);
            }

            callback.call(selectedNetwork.coin, nativeAssetsBalances);
        });
    }

    /**
     * Get assets balances for a list of tokens on all accounts.
     * Only collect balances for current network. Return a nested map that is grouped by accounts
     * first and then tokens. Native tokens (ETH, SOL, FIL) will be excluded.
     */
    public static void getBlockchainTokensBalances(JsonRpcService jsonRpcService,
            NetworkInfo selectedNetwork, AccountInfo[] accountInfos, BlockchainToken[] tokens,
            Callbacks.Callback2<Integer, HashMap<String, HashMap<String, Double>>> callback) {
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
                        : (context.userAsset.coin == CoinType.SOL ? context.decimals
                                                                  : selectedNetwork.decimals);
                Double tokenBalance =
                        (context.error == ProviderError.SUCCESS && context.balance != null
                                && !context.balance.isEmpty())
                        ? Utils.getBalanceForCoinType(
                                selectedNetwork.coin, decimals, context.balance)
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

            callback.call(selectedNetwork.coin, blockchainTokensBalances);
        });
    }

    public static void getP3ABalances(BraveWalletBaseActivity activity, NetworkInfo[] allNetworks,
            NetworkInfo selectedNetwork,
            Callbacks.Callback1<HashMap<Integer, HashSet<String>>> callback) {
        KeyringService keyringService = activity.getKeyringService();
        BraveWalletService braveWalletService = activity.getBraveWalletService();
        BlockchainRegistry blockchainRegistry = activity.getBlockchainRegistry();
        JsonRpcService jsonRpcService = activity.getJsonRpcService();
        assert braveWalletService != null && blockchainRegistry != null && keyringService != null
                && jsonRpcService != null;

        boolean P3AEnabled =
                BraveConfig.P3A_ENABLED && BraveLocalState.get().getBoolean(BravePref.P3A_ENABLED);

        HashMap<Integer, HashSet<String>> activeAddresses = new HashMap<Integer, HashSet<String>>();
        for (int coinType : Utils.P3ACoinTypes)
            activeAddresses.put(coinType, new HashSet<String>());

        if (!P3AEnabled) {
            callback.call(activeAddresses);
            return;
        } else {
            Utils.getP3ANetworks(allNetworks, relevantNetworks -> {
                // Exclude selectedNetwork if in relevantNetworks, also sort by CoinType
                HashMap<Integer, ArrayList<NetworkInfo>> sortedNetworks =
                        filterAndSortNetworksP3A(relevantNetworks, selectedNetwork);
                int numNetworks = 0;
                for (int coinType : Utils.P3ACoinTypes)
                    numNetworks += sortedNetworks.get(coinType).size();

                AsyncUtils.MultiResponseHandler multiResponse =
                        new AsyncUtils.MultiResponseHandler(numNetworks * 2);
                ArrayList<AsyncUtils.GetNativeAssetsBalancesResponseContext>
                        nativeAssetsBalancesResponses =
                                new ArrayList<AsyncUtils.GetNativeAssetsBalancesResponseContext>();
                ArrayList<AsyncUtils.GetBlockchainTokensBalancesResponseContext>
                        blockchainTokensBalancesResponses = new ArrayList<
                                AsyncUtils.GetBlockchainTokensBalancesResponseContext>();

                for (int coinType : Utils.P3ACoinTypes) {
                    keyringService.getKeyringInfo(AssetUtils.getKeyringForCoinType(coinType), keyringInfo -> {
                        for (NetworkInfo network : sortedNetworks.get(coinType)) {
                            TokenUtils.getUserOrAllTokensFiltered(braveWalletService,
                                    blockchainRegistry, network, coinType, TokenUtils.TokenType.ALL,
                                    true, tokens -> {
                                        AsyncUtils.GetNativeAssetsBalancesResponseContext
                                                getNativeAssetsBalancesContext =
                                                new AsyncUtils
                                                        .GetNativeAssetsBalancesResponseContext(
                                                                multiResponse
                                                                        .singleResponseComplete);
                                        getNativeAssetsBalances(jsonRpcService, network,
                                                keyringInfo.accountInfos,
                                                getNativeAssetsBalancesContext);
                                        nativeAssetsBalancesResponses.add(
                                                getNativeAssetsBalancesContext);
                                        AsyncUtils.GetBlockchainTokensBalancesResponseContext
                                                getBlockchainTokensBalancesContext =
                                                new AsyncUtils
                                                        .GetBlockchainTokensBalancesResponseContext(
                                                                multiResponse
                                                                        .singleResponseComplete);
                                        getBlockchainTokensBalances(jsonRpcService, network,
                                                keyringInfo.accountInfos, tokens,
                                                getBlockchainTokensBalancesContext);
                                        blockchainTokensBalancesResponses.add(
                                                getBlockchainTokensBalancesContext);
                                    });
                        }
                    });
                }

                multiResponse.setWhenAllCompletedAction(() -> {
                    updateActiveAddresses(
                            nativeAssetsBalancesResponses.toArray(
                                    new AsyncUtils.GetNativeAssetsBalancesResponseContext[0]),
                            blockchainTokensBalancesResponses.toArray(
                                    new AsyncUtils.GetBlockchainTokensBalancesResponseContext[0]),
                            activeAddresses);
                    callback.call(activeAddresses);
                });
            });
        }
    }

    private static <T extends AsyncUtils.GetBalanceResponseBaseContext> T addBalanceResponseContext(
            ArrayList<AsyncUtils.GetBalanceResponseBaseContext> contexts, T context,
            String accountAddress, BlockchainToken token) {
        context.accountAddress = accountAddress.toLowerCase(Locale.getDefault());
        if (token != null) context.userAsset = token;
        contexts.add(context);
        return context;
    }

    private static HashMap<Integer, ArrayList<NetworkInfo>> filterAndSortNetworksP3A(
            NetworkInfo[] relevantNetworks, NetworkInfo selectedNetwork) {
        HashMap<Integer, ArrayList<NetworkInfo>> networksPerCoin =
                new HashMap<Integer, ArrayList<NetworkInfo>>();
        networksPerCoin.put(CoinType.ETH, new ArrayList<NetworkInfo>());
        networksPerCoin.put(CoinType.SOL, new ArrayList<NetworkInfo>());
        networksPerCoin.put(CoinType.FIL, new ArrayList<NetworkInfo>());
        for (NetworkInfo network : relevantNetworks) {
            if (network.chainId.equals(selectedNetwork.chainId)) continue;
            switch (network.coin) {
                case CoinType.ETH:
                    networksPerCoin.get(CoinType.ETH).add(network);
                    break;
                case CoinType.SOL:
                    networksPerCoin.get(CoinType.SOL).add(network);
                    break;
                case CoinType.FIL:
                    networksPerCoin.get(CoinType.FIL).add(network);
                    break;
            }
        }
        return networksPerCoin;
    }

    public static void updateActiveAddresses(
            AsyncUtils.GetNativeAssetsBalancesResponseContext[] nativeAssetsBalancesResponses,
            AsyncUtils
                    .GetBlockchainTokensBalancesResponseContext[] blockchainTokensBalancesResponses,
            HashMap<Integer, HashSet<String>> activeAddresses) {
        for (AsyncUtils.GetNativeAssetsBalancesResponseContext ctx :
                nativeAssetsBalancesResponses) {
            for (Map.Entry<String, Double> nativeEntry : ctx.nativeAssetsBalances.entrySet()) {
                if (nativeEntry.getValue() > 0.0d)
                    activeAddresses.get(ctx.coinType).add(nativeEntry.getKey());
            }
        }
        for (AsyncUtils.GetBlockchainTokensBalancesResponseContext ctx :
                blockchainTokensBalancesResponses) {
            for (Map.Entry<String, HashMap<String, Double>> accEntry :
                    ctx.blockchainTokensBalances.entrySet()) {
                for (Map.Entry<String, Double> tokenEntry : accEntry.getValue().entrySet()) {
                    if (tokenEntry.getValue() > 0.0d)
                        activeAddresses.get(ctx.coinType).add(accEntry.getKey());
                }
            }
        }
    }
}
