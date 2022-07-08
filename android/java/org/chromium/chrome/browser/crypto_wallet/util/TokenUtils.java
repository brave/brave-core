/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.util;

import org.chromium.brave_wallet.mojom.BlockchainRegistry;
import org.chromium.brave_wallet.mojom.BlockchainToken;
import org.chromium.brave_wallet.mojom.BraveWalletConstants;
import org.chromium.brave_wallet.mojom.BraveWalletService;
import org.chromium.brave_wallet.mojom.CoinType;
import org.chromium.brave_wallet.mojom.OnRampProvider;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;

import java.lang.UnsupportedOperationException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.List;

public class TokenUtils {
    // For  convenience, ERC20 also means ETH
    // SOL for Solana
    public enum TokenType { ERC20, ERC721, SOL, ALL }
    ;

    private static BlockchainToken[] filterTokens(
            BlockchainToken[] tokens, TokenType tokenType, boolean keepVisibleOnly) {
        ArrayList<BlockchainToken> arrayTokens = new ArrayList<>(Arrays.asList(tokens));
        Utils.removeIf(arrayTokens, t -> {
            boolean typeFilter;
            switch (tokenType) {
                case ERC20:
                    typeFilter = !t.isErc20 && !"ETH".equals(t.symbol);
                    break;
                case ERC721:
                    typeFilter = !t.isErc721;
                    break;
                case SOL:
                    typeFilter = false;
                    break;
                case ALL:
                    typeFilter = false;
                    break;
                default:
                    throw new UnsupportedOperationException("Token type not supported.");
            }
            return typeFilter || (keepVisibleOnly && !t.visible);
        });

        return arrayTokens.toArray(new BlockchainToken[0]);
    }

    public static void getUserAssetsFiltered(BraveWalletService braveWalletService, String chainId,
            TokenType tokenType, int coinType, BraveWalletService.GetUserAssets_Response callback) {
        braveWalletService.getUserAssets(chainId, coinType, (BlockchainToken[] tokens) -> {
            BlockchainToken[] filteredTokens = filterTokens(tokens, tokenType, true);
            callback.call(filteredTokens);
        });
    }

    public static void getAllTokensFiltered(BraveWalletService braveWalletService,
            BlockchainRegistry blockchainRegistry, String chainId, TokenType tokenType,
            BlockchainRegistry.GetAllTokens_Response callback) {
        blockchainRegistry.getAllTokens(BraveWalletConstants.MAINNET_CHAIN_ID,
                tokenType == TokenType.SOL ? CoinType.SOL : CoinType.ETH,
                (BlockchainToken[] tokens) -> {
                    braveWalletService.getUserAssets(chainId,
                            tokenType == TokenType.SOL ? CoinType.SOL : CoinType.ETH,
                            (BlockchainToken[] userTokens) -> {
                                BlockchainToken[] filteredTokens = filterTokens(
                                        concatenateTwoArrays(tokens, userTokens), tokenType, false);
                                callback.call(filteredTokens);
                            });
                });
    }

    public static void getBuyTokensFiltered(BlockchainRegistry blockchainRegistry,
            TokenType tokenType, BlockchainRegistry.GetAllTokens_Response callback) {
        blockchainRegistry.getBuyTokens(OnRampProvider.WYRE, BraveWalletConstants.MAINNET_CHAIN_ID,
                (BlockchainToken[] tokens) -> {
                    BlockchainToken[] filteredTokens = filterTokens(tokens, tokenType, false);
                    callback.call(filteredTokens);
                });
    }

    public static void isCustomToken(BlockchainToken token, BlockchainRegistry blockchainRegistry,
            org.chromium.mojo.bindings.Callbacks.Callback1<Boolean> callback) {
        blockchainRegistry.getAllTokens(
                BraveWalletConstants.MAINNET_CHAIN_ID, CoinType.ETH, (BlockchainToken[] tokens) -> {
                    boolean isCustom = true;
                    for (BlockchainToken tokenFromAll : tokens) {
                        if (token.contractAddress.equals(tokenFromAll.contractAddress)) {
                            isCustom = false;
                            break;
                        }
                    }
                    callback.call(isCustom);
                });
    }

    private static BlockchainToken[] concatenateTwoArrays(
            BlockchainToken[] arrayFirst, BlockchainToken[] arraySecond) {
        List<BlockchainToken> both = new ArrayList<>();

        Collections.addAll(both, arrayFirst);
        for (BlockchainToken tokenSecond : arraySecond) {
            boolean add = true;
            for (BlockchainToken tokenFirst : arrayFirst) {
                if (tokenFirst.contractAddress.equals(tokenSecond.contractAddress)) {
                    add = false;
                    break;
                }
            }
            if (add) {
                both.add(tokenSecond);
            }
        }

        return both.toArray(new BlockchainToken[both.size()]);
    }

    public static boolean isSameToken(BlockchainToken token1, BlockchainToken token2) {
        if (token1.chainId.equals(token2.chainId) && token1.symbol.equals(token2.symbol)
                && token1.name.equals(token2.name)
                && ((token1.tokenId.isEmpty() && token2.tokenId.isEmpty())
                        || token1.tokenId.equals(token2.tokenId))
                && token1.contractAddress.equals(token2.contractAddress)
                && token1.decimals == token2.decimals) {
            return true;
        }

        return false;
    }
}
