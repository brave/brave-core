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

public class TokenUtils {
    private static boolean shouldTokenBeFilteredOut(ErcToken ercToken) {
        return ercToken.isErc721;
    }

    private static ErcToken[] filterOut(ErcToken[] tokens) {
        ArrayList<ErcToken> arrayTokens = new ArrayList<>(Arrays.asList(tokens));
        Utils.removeIf(arrayTokens, t -> TokenUtils.shouldTokenBeFilteredOut(t));

        return arrayTokens.toArray(new ErcToken[0]);
    }

    public static void getUserAssetsFiltered(BraveWalletService braveWalletService, String chainId,
            BraveWalletService.GetUserAssetsResponse callback) {
        braveWalletService.getUserAssets(chainId, (ErcToken[] tokens) -> {
            ErcToken[] filteredTokens = filterOut(tokens);
            callback.call(filteredTokens);
        });
    }

    public static void getAllTokensFiltered(
            ErcTokenRegistry ercTokenRegistry, ErcTokenRegistry.GetAllTokensResponse callback) {
        ercTokenRegistry.getAllTokens((ErcToken[] tokens) -> {
            ErcToken[] filteredTokens = filterOut(tokens);
            callback.call(filteredTokens);
        });
    }

    public static void getBuyTokensFiltered(
            ErcTokenRegistry ercTokenRegistry, ErcTokenRegistry.GetAllTokensResponse callback) {
        ercTokenRegistry.getBuyTokens((ErcToken[] tokens) -> {
            ErcToken[] filteredTokens = filterOut(tokens);
            callback.call(filteredTokens);
        });
    }
}
