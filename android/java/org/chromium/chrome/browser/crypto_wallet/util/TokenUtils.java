/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.util;

import org.chromium.brave_wallet.mojom.BlockchainRegistry;
import org.chromium.brave_wallet.mojom.BlockchainToken;
import org.chromium.brave_wallet.mojom.BraveWalletConstants;
import org.chromium.brave_wallet.mojom.BraveWalletService;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.List;

public class TokenUtils {
    private static boolean shouldTokenBeFilteredOut(BlockchainToken blockchainToken) {
        return blockchainToken.isErc721;
    }

    private static boolean shouldUserVisibleTokenBeFilteredOut(BlockchainToken blockchainToken) {
        return blockchainToken.isErc721 || !blockchainToken.visible;
    }

    private static BlockchainToken[] filterOut(BlockchainToken[] tokens, boolean allTokens) {
        ArrayList<BlockchainToken> arrayTokens = new ArrayList<>(Arrays.asList(tokens));
        if (allTokens) {
            Utils.removeIf(arrayTokens, t -> TokenUtils.shouldTokenBeFilteredOut(t));
        } else {
            Utils.removeIf(arrayTokens, t -> TokenUtils.shouldUserVisibleTokenBeFilteredOut(t));
        }

        return arrayTokens.toArray(new BlockchainToken[0]);
    }

    public static void getUserAssetsFiltered(BraveWalletService braveWalletService, String chainId,
            BraveWalletService.GetUserAssets_Response callback) {
        braveWalletService.getUserAssets(chainId, (BlockchainToken[] tokens) -> {
            BlockchainToken[] filteredTokens = filterOut(tokens, false);
            callback.call(filteredTokens);
        });
    }

    public static void getAllTokensFiltered(BraveWalletService braveWalletService,
            BlockchainRegistry blockchainRegistry, String chainId,
            BlockchainRegistry.GetAllTokens_Response callback) {
        blockchainRegistry.getAllTokens(
                BraveWalletConstants.MAINNET_CHAIN_ID, (BlockchainToken[] tokens) -> {
                    braveWalletService.getUserAssets(chainId, (BlockchainToken[] userTokens) -> {
                        BlockchainToken[] filteredTokens =
                                filterOut(concatenateTwoArrays(tokens, userTokens), true);
                        callback.call(filteredTokens);
                    });
                });
    }

    public static void getBuyTokensFiltered(BlockchainRegistry blockchainRegistry,
            BlockchainRegistry.GetAllTokens_Response callback) {
        blockchainRegistry.getBuyTokens(
                BraveWalletConstants.MAINNET_CHAIN_ID, (BlockchainToken[] tokens) -> {
                    BlockchainToken[] filteredTokens = filterOut(tokens, true);
                    callback.call(filteredTokens);
                });
    }

    public static void isCustomToken(BlockchainToken token, BlockchainRegistry blockchainRegistry,
            org.chromium.mojo.bindings.Callbacks.Callback1<Boolean> callback) {
        blockchainRegistry.getAllTokens(
                BraveWalletConstants.MAINNET_CHAIN_ID, (BlockchainToken[] tokens) -> {
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
}
