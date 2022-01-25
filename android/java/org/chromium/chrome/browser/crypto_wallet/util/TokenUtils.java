/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.util;

import org.chromium.brave_wallet.mojom.BraveWalletService;
import org.chromium.brave_wallet.mojom.ErcToken;
import org.chromium.brave_wallet.mojom.ErcTokenRegistry;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.List;

public class TokenUtils {
    private static boolean shouldTokenBeFilteredOut(ErcToken ercToken) {
        return ercToken.isErc721;
    }

    private static boolean shouldUserVisibleTokenBeFilteredOut(ErcToken ercToken) {
        return ercToken.isErc721 || !ercToken.visible;
    }

    private static ErcToken[] filterOut(ErcToken[] tokens, boolean allTokens) {
        ArrayList<ErcToken> arrayTokens = new ArrayList<>(Arrays.asList(tokens));
        if (allTokens) {
            Utils.removeIf(arrayTokens, t -> TokenUtils.shouldTokenBeFilteredOut(t));
        } else {
            Utils.removeIf(arrayTokens, t -> TokenUtils.shouldUserVisibleTokenBeFilteredOut(t));
        }

        return arrayTokens.toArray(new ErcToken[0]);
    }

    public static void getUserAssetsFiltered(BraveWalletService braveWalletService, String chainId,
            BraveWalletService.GetUserAssets_Response callback) {
        braveWalletService.getUserAssets(chainId, (ErcToken[] tokens) -> {
            ErcToken[] filteredTokens = filterOut(tokens, false);
            callback.call(filteredTokens);
        });
    }

    public static void getAllTokensFiltered(BraveWalletService braveWalletService,
            ErcTokenRegistry ercTokenRegistry, String chainId,
            ErcTokenRegistry.GetAllTokens_Response callback) {
        ercTokenRegistry.getAllTokens((ErcToken[] tokens) -> {
            braveWalletService.getUserAssets(chainId, (ErcToken[] userTokens) -> {
                ErcToken[] filteredTokens =
                        filterOut(concatenateTwoArrays(tokens, userTokens), true);
                callback.call(filteredTokens);
            });
        });
    }

    public static void getBuyTokensFiltered(
            ErcTokenRegistry ercTokenRegistry, ErcTokenRegistry.GetAllTokens_Response callback) {
        ercTokenRegistry.getBuyTokens((ErcToken[] tokens) -> {
            ErcToken[] filteredTokens = filterOut(tokens, true);
            callback.call(filteredTokens);
        });
    }

    public static void isCustomToken(ErcToken token, ErcTokenRegistry ercTokenRegistry,
            org.chromium.mojo.bindings.Callbacks.Callback1<Boolean> callback) {
        ercTokenRegistry.getAllTokens((ErcToken[] tokens) -> {
            boolean isCustom = true;
            for (ErcToken tokenFromAll : tokens) {
                if (token.contractAddress.equals(tokenFromAll.contractAddress)) {
                    isCustom = false;
                    break;
                }
            }
            callback.call(isCustom);
        });
    }

    private static ErcToken[] concatenateTwoArrays(ErcToken[] arrayFirst, ErcToken[] arraySecond) {
        List<ErcToken> both = new ArrayList<>();

        Collections.addAll(both, arrayFirst);
        for (ErcToken tokenSecond : arraySecond) {
            boolean add = true;
            for (ErcToken tokenFirst : arrayFirst) {
                if (tokenFirst.contractAddress.equals(tokenSecond.contractAddress)) {
                    add = false;
                    break;
                }
            }
            if (add) {
                both.add(tokenSecond);
            }
        }

        return both.toArray(new ErcToken[both.size()]);
    }
}
