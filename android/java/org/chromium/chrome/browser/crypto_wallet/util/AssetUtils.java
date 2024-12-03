/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.util;

import android.text.TextUtils;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import org.chromium.base.Log;
import org.chromium.brave_wallet.mojom.AccountInfo;
import org.chromium.brave_wallet.mojom.BlockchainToken;
import org.chromium.brave_wallet.mojom.BraveWalletConstants;
import org.chromium.brave_wallet.mojom.CoinType;
import org.chromium.brave_wallet.mojom.KeyringId;

import java.util.Arrays;
import java.util.List;
import java.util.Locale;
import java.util.Map;

public class AssetUtils {
    private static final String TAG = "AssetUtils";

    public static class Filters {
        public static boolean isSameToken(
                BlockchainToken token, String symbol, String contractAddress, String chainId) {
            return token.symbol.equals(symbol)
                    && token.contractAddress.equalsIgnoreCase(contractAddress)
                    && token.chainId.equals(chainId);
        }
    }

    public static @KeyringId.EnumType int getKeyring(
            @CoinType.EnumType int coinType, @Nullable String chainId) {
        if (coinType == CoinType.FIL) {
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
     * @return Keyring Id for coin tpye. If coin type does not belong to Ethereum or Solana it
     *     defaults to {@link BraveWalletConstants.DEFAULT_KEYRING_ID}.
     */
    public static @KeyringId.EnumType int getKeyringForEthOrSolOnly(
            @CoinType.EnumType int coinType) {
        switch (coinType) {
            case CoinType.ETH:
                return KeyringId.DEFAULT;
            case CoinType.SOL:
                return KeyringId.SOLANA;
            case CoinType.FIL:
                throw new IllegalArgumentException(
                        "Keyring Id for Filecoin cannot be obtained by coin type. Consider using"
                                + " the method \"AssetUtils.getKeyring(coinType, chainId)\".");
            case CoinType.BTC:
                throw new IllegalArgumentException(
                        "Keyring Id for Bitcoin cannot be obtained by coin type. Consider using the"
                                + " method \"AssetUtils.getKeyring(coinType, chainId)\".");
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

    public static boolean isNativeToken(BlockchainToken token) {
        List<String> nativeTokens = NATIVE_TOKENS_PER_CHAIN.get(token.chainId);
        if (nativeTokens != null) {
            return nativeTokens.contains(token.symbol.toLowerCase(Locale.ENGLISH));
        }
        return false;
    }

    public static boolean isBatToken(BlockchainToken token) {
        String symbol = token.symbol;
        return symbol.equalsIgnoreCase("bat")
                || symbol.equalsIgnoreCase("wbat")
                || symbol.equalsIgnoreCase("bat.e");
    }

    private static final Map<String, List<String>> NATIVE_TOKENS_PER_CHAIN =
            Map.ofEntries(
                    Map.entry(BraveWalletConstants.MAINNET_CHAIN_ID, List.of("eth")),
                    Map.entry(BraveWalletConstants.OPTIMISM_MAINNET_CHAIN_ID, List.of("eth")),
                    Map.entry(BraveWalletConstants.AURORA_MAINNET_CHAIN_ID, List.of("eth")),
                    Map.entry(BraveWalletConstants.POLYGON_MAINNET_CHAIN_ID, List.of("matic")),
                    Map.entry(BraveWalletConstants.FANTOM_MAINNET_CHAIN_ID, List.of("ftm")),
                    Map.entry(BraveWalletConstants.CELO_MAINNET_CHAIN_ID, List.of("celo")),
                    Map.entry(
                            BraveWalletConstants.BNB_SMART_CHAIN_MAINNET_CHAIN_ID, List.of("bnb")),
                    Map.entry(BraveWalletConstants.SOLANA_MAINNET, List.of("sol")),
                    Map.entry(BraveWalletConstants.FILECOIN_MAINNET, List.of("fil")),
                    Map.entry(
                            BraveWalletConstants.AVALANCHE_MAINNET_CHAIN_ID,
                            Arrays.asList("avax", "avaxc")),
                    // Testnets
                    Map.entry(BraveWalletConstants.SEPOLIA_CHAIN_ID, List.of("eth")),
                    Map.entry(BraveWalletConstants.SOLANA_TESTNET, List.of("sol")),
                    Map.entry(BraveWalletConstants.SOLANA_DEVNET, List.of("sol")),
                    Map.entry(BraveWalletConstants.FILECOIN_TESTNET, List.of("fil")));

    public static String assetRatioId(@NonNull final BlockchainToken token) {
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
