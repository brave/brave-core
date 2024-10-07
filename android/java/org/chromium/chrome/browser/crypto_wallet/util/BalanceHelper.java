/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.util;

import androidx.annotation.NonNull;

import org.chromium.base.Callbacks;
import org.chromium.brave_wallet.mojom.AccountInfo;
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
import org.chromium.chrome.browser.crypto_wallet.util.AsyncUtils.GetBalanceResponseBaseContext;
import org.chromium.chrome.browser.crypto_wallet.util.AsyncUtils.GetBalanceResponseContext;
import org.chromium.chrome.browser.crypto_wallet.util.AsyncUtils.GetBlockchainTokensBalancesResponseContext;
import org.chromium.chrome.browser.crypto_wallet.util.AsyncUtils.GetErc20TokenBalanceResponseContext;
import org.chromium.chrome.browser.crypto_wallet.util.AsyncUtils.GetErc721TokenBalanceResponseContext;
import org.chromium.chrome.browser.crypto_wallet.util.AsyncUtils.GetNativeAssetsBalancesResponseContext;
import org.chromium.chrome.browser.crypto_wallet.util.AsyncUtils.GetSolanaBalanceResponseContext;
import org.chromium.chrome.browser.crypto_wallet.util.AsyncUtils.GetSplTokenAccountBalanceResponseContext;
import org.chromium.chrome.browser.crypto_wallet.util.AsyncUtils.MultiResponseHandler;
import org.chromium.chrome.browser.preferences.BravePref;

import java.lang.ref.WeakReference;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Locale;
import java.util.Map;

public class BalanceHelper {
    /** Get assets balances for all accounts on selected network. */
    public static void getNativeAssetsBalances(
            @NonNull final JsonRpcService jsonRpcService,
            @NonNull final NetworkInfo selectedNetwork,
            @NonNull final AccountInfo[] accounts,
            @NonNull final Callbacks.Callback2<Integer, HashMap<String, Double>> callback) {
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

    public static void getP3ABalances(
            WeakReference<BraveWalletBaseActivity> activityRef,
            List<NetworkInfo> allNetworks,
            NetworkInfo selectedNetwork,
            Callbacks.Callback1<HashMap<Integer, HashSet<String>>> callback) {
        BraveWalletBaseActivity activity = activityRef.get();
        if (activity == null || activity.isFinishing()) return;
        KeyringService keyringService = activity.getKeyringService();
        BraveWalletService braveWalletService = activity.getBraveWalletService();
        BlockchainRegistry blockchainRegistry = activity.getBlockchainRegistry();
        JsonRpcService jsonRpcService = activity.getJsonRpcService();
        assert braveWalletService != null
                && blockchainRegistry != null
                && keyringService != null
                && jsonRpcService != null;
        if (JavaUtils.anyNull(
                braveWalletService, blockchainRegistry, keyringService, jsonRpcService)) return;

        boolean P3AEnabled =
                BraveConfig.P3A_ENABLED && BraveLocalState.get().getBoolean(BravePref.P3A_ENABLED);

        HashMap<Integer, HashSet<String>> activeAddresses = new HashMap<Integer, HashSet<String>>();
        for (int coinType : Utils.P3ACoinTypes)
            activeAddresses.put(coinType, new HashSet<String>());

        if (!P3AEnabled) {
            callback.call(activeAddresses);
            return;
        } else {
            Utils.getP3ANetworks(
                    allNetworks,
                    relevantNetworks -> {
                        // Exclude selectedNetwork if in relevantNetworks, also sort by CoinType
                        HashMap<Integer, ArrayList<NetworkInfo>> sortedNetworks =
                                filterAndSortNetworksP3A(relevantNetworks, selectedNetwork);
                        int numNetworks = 0;
                        for (int coinType : Utils.P3ACoinTypes) {
                            numNetworks += sortedNetworks.get(coinType).size();
                        }

                        MultiResponseHandler multiResponse =
                                new MultiResponseHandler(numNetworks * 2);
                        ArrayList<GetNativeAssetsBalancesResponseContext>
                                nativeAssetsBalancesResponses =
                                        new ArrayList<GetNativeAssetsBalancesResponseContext>();
                        ArrayList<GetBlockchainTokensBalancesResponseContext>
                                blockchainTokensBalancesResponses =
                                        new ArrayList<GetBlockchainTokensBalancesResponseContext>();

                        for (int coinType : Utils.P3ACoinTypes) {
                            processP3ACoinNetworks(
                                    coinType,
                                    sortedNetworks.get(coinType),
                                    keyringService,
                                    jsonRpcService,
                                    braveWalletService,
                                    blockchainRegistry,
                                    multiResponse,
                                    nativeAssetsBalancesResponses,
                                    blockchainTokensBalancesResponses);
                        }

                        multiResponse.setWhenAllCompletedAction(
                                () -> {
                                    updateActiveAddresses(
                                            nativeAssetsBalancesResponses.toArray(
                                                    new GetNativeAssetsBalancesResponseContext[0]),
                                            blockchainTokensBalancesResponses.toArray(
                                                    new GetBlockchainTokensBalancesResponseContext
                                                            [0]),
                                            activeAddresses);
                                    callback.call(activeAddresses);
                                });
                    });
        }
    }

    private static void processP3ACoinNetworks(
            @CoinType.EnumType int coinType,
            List<NetworkInfo> networks,
            KeyringService keyringService,
            JsonRpcService jsonRpcService,
            BraveWalletService braveWalletService,
            BlockchainRegistry blockchainRegistry,
            MultiResponseHandler multiResponse,
            ArrayList<GetNativeAssetsBalancesResponseContext> nativeAssetsBalancesResponses,
            ArrayList<GetBlockchainTokensBalancesResponseContext>
                    blockchainTokensBalancesResponses) {
        if (JavaUtils.anyNull(braveWalletService, blockchainRegistry, jsonRpcService)) return;

        keyringService.getAllAccounts(
                allAccounts -> {
                    for (NetworkInfo network : networks) {
                        AccountInfo[] accountInfoArray =
                                AssetUtils.filterAccountsByNetwork(
                                        allAccounts.accounts, network.coin, network.chainId);

                        TokenUtils.getVisibleUserAssetsFiltered(
                                braveWalletService,
                                network,
                                coinType,
                                TokenUtils.TokenType.ALL,
                                tokens -> {
                                    // Assets balances.
                                    GetNativeAssetsBalancesResponseContext
                                            getNativeAssetsBalancesContext =
                                                    new GetNativeAssetsBalancesResponseContext(
                                                            multiResponse.singleResponseComplete);
                                    getNativeAssetsBalances(
                                            jsonRpcService,
                                            network,
                                            accountInfoArray,
                                            getNativeAssetsBalancesContext);
                                    nativeAssetsBalancesResponses.add(
                                            getNativeAssetsBalancesContext);

                                    // Tokens balances.
                                    GetBlockchainTokensBalancesResponseContext
                                            getBlockchainTokensBalancesContext =
                                                    new GetBlockchainTokensBalancesResponseContext(
                                                            multiResponse.singleResponseComplete);
                                    getBlockchainTokensBalances(
                                            jsonRpcService,
                                            network,
                                            accountInfoArray,
                                            tokens,
                                            getBlockchainTokensBalancesContext);
                                    blockchainTokensBalancesResponses.add(
                                            getBlockchainTokensBalancesContext);
                                });
                    }
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

    private static HashMap<Integer, ArrayList<NetworkInfo>> filterAndSortNetworksP3A(
            List<NetworkInfo> relevantNetworks, NetworkInfo selectedNetwork) {
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
            GetNativeAssetsBalancesResponseContext[] nativeAssetsBalancesResponses,
            GetBlockchainTokensBalancesResponseContext[] blockchainTokensBalancesResponses,
            HashMap<Integer, HashSet<String>> activeAddresses) {
        for (GetNativeAssetsBalancesResponseContext ctx : nativeAssetsBalancesResponses) {
            for (Map.Entry<String, Double> nativeEntry : ctx.nativeAssetsBalances.entrySet()) {
                if (nativeEntry.getValue() > 0.0d)
                    activeAddresses.get(ctx.coinType).add(nativeEntry.getKey());
            }
        }
        for (GetBlockchainTokensBalancesResponseContext ctx : blockchainTokensBalancesResponses) {
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
