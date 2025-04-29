/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.util;

import android.text.TextUtils;

import org.chromium.base.Log;
import org.chromium.brave_wallet.mojom.AccountInfo;
import org.chromium.brave_wallet.mojom.BlockchainToken;
import org.chromium.brave_wallet.mojom.BraveWalletConstants;
import org.chromium.brave_wallet.mojom.CoinType;
import org.chromium.brave_wallet.mojom.KeyringId;
import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;

import java.util.Arrays;
import java.util.Locale;

@NullMarked
public class AssetUtils {
    private static final String TAG = "AssetUtils";

    public static @KeyringId.EnumType int getKeyring(
            @CoinType.EnumType int coinType, @Nullable String chainId) {
        if (coinType == CoinType.ZEC) {
            if (BraveWalletConstants.Z_CASH_MAINNET.equals(chainId)) {
                return KeyringId.Z_CASH_MAINNET;
            }
            if (BraveWalletConstants.Z_CASH_TESTNET.equals(chainId)) {
                return KeyringId.Z_CASH_TESTNET;
            }
            throw new IllegalStateException(
                    String.format("No ZCash keyring found for chain Id %s.", chainId));
        } else if (coinType == CoinType.FIL) {
            assert chainId != null;
            switch (chainId) {
                case BraveWalletConstants.FILECOIN_MAINNET:
                case BraveWalletConstants.LOCALHOST_CHAIN_ID:
                    return KeyringId.FILECOIN;

                case BraveWalletConstants.FILECOIN_TESTNET:
                    return KeyringId.FILECOIN_TESTNET;

                default:
                    throw new IllegalStateException(
                            String.format("No Filecoin keyring found for chain Id %s.", chainId));
            }
        } else if (coinType == CoinType.BTC) {
            if (BraveWalletConstants.BITCOIN_MAINNET.equals(chainId)) {
                return KeyringId.BITCOIN84;
            }
            if (BraveWalletConstants.BITCOIN_TESTNET.equals(chainId)) {
                return KeyringId.BITCOIN84_TESTNET;
            }
            throw new IllegalStateException(
                    String.format("No Bitcoin keyring found for chain Id %s.", chainId));
        } else {
            return getKeyringForEthOrSolOnly(coinType);
        }
    }

    @SuppressWarnings("NoStreams")
    public static AccountInfo[] filterAccountsByNetwork(
            AccountInfo[] accounts, @CoinType.EnumType int coinType, @Nullable String chainId) {
        @KeyringId.EnumType int keyringId = AssetUtils.getKeyring(coinType, chainId);

        return Arrays.stream(accounts)
                .filter(acc -> acc.accountId.keyringId == keyringId)
                .toArray(AccountInfo[]::new);
    }

    /**
     * Gets keyring Id only for coin types Ethereum and Solana.
     *
     * @param coinType Coin type Ethereum or Solana.
     * @return Keyring Id for coin type. If coin type does not belong to Ethereum or Solana it
     *     defaults to {@code KeyringId.DEFAULT}.
     */
    public static @KeyringId.EnumType int getKeyringForEthOrSolOnly(
            @CoinType.EnumType int coinType) {
        switch (coinType) {
            case CoinType.ETH:
                return KeyringId.DEFAULT;
            case CoinType.SOL:
                return KeyringId.SOLANA;
            case CoinType.FIL:
            case CoinType.BTC:
            case CoinType.ZEC:
            default:
                Log.e(
                        TAG,
                        String.format(
                                Locale.ENGLISH,
                                "Keyring Id for coin type %d cannot be found. Returning default"
                                        + " keyring Id.",
                                coinType));
                return KeyringId.DEFAULT;
        }
    }

    public static String assetRatioId(final BlockchainToken token) {
        if (!TextUtils.isEmpty(token.coingeckoId)) {
            return token.coingeckoId;
        }
        if (BraveWalletConstants.MAINNET_CHAIN_ID.equals(token.chainId)
                || TextUtils.isEmpty(token.contractAddress)) {
            return token.symbol;
        }
        return token.contractAddress;
    }
}
