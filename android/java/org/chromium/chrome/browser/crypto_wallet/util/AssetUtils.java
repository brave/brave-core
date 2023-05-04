/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.util;

import static java.util.stream.Collectors.toMap;

import android.text.TextUtils;
import android.util.Log;

import androidx.annotation.Nullable;

import org.chromium.brave_wallet.mojom.BlockchainToken;
import org.chromium.brave_wallet.mojom.BraveWalletConstants;
import org.chromium.brave_wallet.mojom.CoinType;
import org.chromium.brave_wallet.mojom.NetworkInfo;

import java.util.Arrays;
import java.util.HashMap;
import java.util.List;
import java.util.Locale;
import java.util.Map;
import java.util.function.Function;
import java.util.stream.IntStream;

public class AssetUtils {
    private static final String TAG = "AssetUtils";

    public static String AURORA_SUPPORTED_CONTRACT_ADDRESSES[] = {
            "0x7fc66500c84a76ad7e9c93437bfc5ac33e2ddae9", // AAVE
            "0xaaaaaa20d9e0e2461697782ef11675f668207961", // AURORA
            "0xba100000625a3754423978a60c9317c58a424e3d", // BAL
            "0x0d8775f648430679a709e98d2b0cb6250d2887ef", // BAT
            "0xc00e94cb662c3520282e6f5717214004a7f26888", // COMP
            "0x2ba592f78db6436527729929aaf6c908497cb200", // CREAM
            "0x6b175474e89094c44da98b954eedeac495271d0f", // DAI
            "0x43dfc4159d86f3a37a5a4b3d4580b888ad7d4ddd", // DODO
            "0x3ea8ea4237344c9931214796d9417af1a1180770", // FLX
            "0x853d955acef822db058eb8505911ed77f175b99e", // FRAX
            "0x3432b6a60d23ca0dfca7761b7ab56459d9c964d0", // FXS
            "0xd9c2d319cd7e6177336b0a9c93c21cb48d84fb54", // HAPI
            "0x514910771af9ca656af840dff83e8264ecf986ca", // LINK
            "0x9f8f72aa9304c8b593d555f12ef6589cc3a579a2", // MKR
            "0x1117ac6ad6cdf1a3bc543bad3b133724620522d5", // MODA
            "0xf5cfbc74057c610c8ef151a439252680ac68c6dc", // OCT
            "0x9aeb50f542050172359a0e1a25a9933bc8c01259", // OIN
            "0xea7cc765ebc94c4805e3bff28d7e4ae48d06468a", // PAD
            "0x429881672b9ae42b8eba0e26cd9c73711b891ca5", // PICKLE
            "0x408e41876cccdc0f92210600ef50372656052a38", // REN
            "0xc011a73ee8576fb46f5e1c5751ca3b9fe0af2a6f", // SNX
            "0x6b3595068778dd592e39a122f4f5a5cf09c90fe2", // SUSHI
            "0x1f9840a85d5af5bf1d1762f925bdaddc4201f984", // UNI
            "0xa0b86991c6218b36c1d19d4a2e9eb0ce3606eb48", // USDC
            "0xdac17f958d2ee523a2206206994597c13d831ec7", // USDT
            "0x2260fac5e5542a773aa44fbcfedf7c193bc2c599", // WBTC
            "0x4691937a7508860f876c9c0a2a617e7d9e945d4b", // WOO
            "0x0bc529c00c6401aef6d220be8c6ea1667f6ad93e" // YFI
    };

    public static class Filters {
        public static boolean isSameToken(BlockchainToken token1, BlockchainToken token2) {
            return token1.symbol.equals(token2.symbol)
                    && token1.contractAddress.equalsIgnoreCase(token2.contractAddress)
                    && token1.chainId.equals(token2.chainId);
        }

        public static boolean isSameToken(
                BlockchainToken token, String symbol, String contractAddress, String chainId) {
            return token.symbol.equals(symbol)
                    && token.contractAddress.equalsIgnoreCase(contractAddress)
                    && token.chainId.equals(chainId);
        }

        public static boolean isSameNFT(
                BlockchainToken token, String tokenId, String contractAddress) {
            if (token.isErc721) {
                boolean isSameContractAddress = TextUtils.isEmpty(token.contractAddress)
                        && TextUtils.isEmpty(contractAddress);
                if (!isSameContractAddress && !TextUtils.isEmpty(token.contractAddress)
                        && !TextUtils.isEmpty(contractAddress)) {
                    isSameContractAddress = token.contractAddress.equalsIgnoreCase(contractAddress);
                }
                return token.tokenId.equals(tokenId) && isSameContractAddress;
            } else if (token.isNft) { // Solana
                return token.contractAddress.equals(contractAddress);
            }
            return false;
        }

        public static boolean isSameNFT(BlockchainToken token1, BlockchainToken token2) {
            return isSameNFT(token1, token2.tokenId, token2.contractAddress);
        }
    }

    public static boolean isAuroraAddress(String contractAddress, String chainId) {
        boolean isEthereumBridgeAddress = false;
        boolean isNativeAsset = TextUtils.isEmpty(contractAddress);

        if (!isNativeAsset) {
            for (String address : AURORA_SUPPORTED_CONTRACT_ADDRESSES) {
                isEthereumBridgeAddress = address.equalsIgnoreCase(contractAddress);
                if (isEthereumBridgeAddress) {
                    break;
                }
            }
        }
        return (isEthereumBridgeAddress || isNativeAsset)
                && chainId.equals(BraveWalletConstants.MAINNET_CHAIN_ID);
    }

    public static String getKeyring(@CoinType.EnumType int coinType, @Nullable String chainId) {
        if (coinType == CoinType.FIL) {
            switch (chainId) {
                case BraveWalletConstants.FILECOIN_MAINNET:
                case BraveWalletConstants.FILECOIN_ETHEREUM_MAINNET_CHAIN_ID:
                    return BraveWalletConstants.FILECOIN_KEYRING_ID;

                case BraveWalletConstants.FILECOIN_TESTNET:
                case BraveWalletConstants.FILECOIN_ETHEREUM_TESTNET_CHAIN_ID:
                    return BraveWalletConstants.FILECOIN_TESTNET_KEYRING_ID;

                default:
                    throw new IllegalStateException(
                            String.format("No keyring found for chain Id %s.", chainId));
            }
        } else {
            return getKeyringForEthOrSolOnly(coinType);
        }
    }

    /**
     * Gets keyring Id only for coin types Ethereum and Solana.
     * @param coinType Coin type Ethereum or Solana.
     * @return Keyring Id for coin tpye. If coin type does not belong to Ethereum or Solana it
     *         defaults to {@link BraveWalletConstants.DEFAULT_KEYRING_ID}.
     */
    public static String getKeyringForEthOrSolOnly(int coinType) {
        switch (coinType) {
            case CoinType.ETH:
                return BraveWalletConstants.DEFAULT_KEYRING_ID;
            case CoinType.SOL:
                return BraveWalletConstants.SOLANA_KEYRING_ID;
            case CoinType.FIL:
                Log.e(TAG,
                        "Keyring Id for Filecoin cannot be obtained by coin type. Returning default keyring Id. Consider using the method \"AssetUtils.getKeyring(coinType, chainId)\".");
                return BraveWalletConstants.DEFAULT_KEYRING_ID;
            default:
                Log.e(TAG,
                        String.format(
                                "Keyring Id for coin type %d cannot be found. Returning default keyring Id.",
                                coinType));
                return BraveWalletConstants.DEFAULT_KEYRING_ID;
        }
    }

    public static @CoinType.EnumType int getCoinForKeyring(String keyringId) {
        int coin = CoinType.ETH; // For default keyring
        switch (keyringId) {
            case BraveWalletConstants.SOLANA_KEYRING_ID:
                coin = CoinType.SOL;
                break;
            case BraveWalletConstants.FILECOIN_KEYRING_ID:
            case BraveWalletConstants.FILECOIN_TESTNET_KEYRING_ID:
                coin = CoinType.FIL;
                break;
            default:
                // Do nothing
        }
        return coin;
    }

    public static String mapToRampNetworkSymbol(
            String assetChainId, String assetSymbol, String contractAddress) {
        if (assetSymbol.equalsIgnoreCase("bat")
                && assetChainId.equals(BraveWalletConstants.MAINNET_CHAIN_ID)) {
            // BAT is the only token on Ethereum Mainnet with a prefix on Ramp.Network
            return "ETH_BAT";
        } else if (assetChainId.equals(BraveWalletConstants.AVALANCHE_MAINNET_CHAIN_ID)
                && TextUtils.isEmpty(contractAddress)) {
            // AVAX native token has no prefix
            return assetSymbol;
        } else {
            String rampNetworkPrefix = getRampNetworkPrefix(assetChainId);
            return TextUtils.isEmpty(rampNetworkPrefix)
                    ? assetSymbol.toUpperCase(Locale.ENGLISH)
                    : rampNetworkPrefix + "_" + assetSymbol.toUpperCase(Locale.ENGLISH);
        }
    }

    public static String getRampNetworkPrefix(String chainId) {
        switch (chainId) {
            case BraveWalletConstants.AVALANCHE_MAINNET_CHAIN_ID:
                return "AVAXC";
            case BraveWalletConstants.BINANCE_SMART_CHAIN_MAINNET_CHAIN_ID:
                return "BSC";
            case BraveWalletConstants.POLYGON_MAINNET_CHAIN_ID:
                return "MATIC";
            case BraveWalletConstants.SOLANA_MAINNET:
                return "SOLANA";
            case BraveWalletConstants.OPTIMISM_MAINNET_CHAIN_ID:
                return "OPTIMISM";
            case BraveWalletConstants.FILECOIN_MAINNET:
                return "FILECOIN";
            case BraveWalletConstants.MAINNET_CHAIN_ID:
            case BraveWalletConstants.CELO_MAINNET_CHAIN_ID:
            default:
                return "";
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
        return symbol.equalsIgnoreCase("bat") || symbol.equalsIgnoreCase("wbat")
                || symbol.equalsIgnoreCase("bat.e");
    }

    private static final Map<String, List<String>> NATIVE_TOKENS_PER_CHAIN = new HashMap<>() {
        {
            put(BraveWalletConstants.MAINNET_CHAIN_ID, Arrays.asList("eth"));
            put(BraveWalletConstants.OPTIMISM_MAINNET_CHAIN_ID, Arrays.asList("eth"));
            put(BraveWalletConstants.AURORA_MAINNET_CHAIN_ID, Arrays.asList("eth"));
            put(BraveWalletConstants.POLYGON_MAINNET_CHAIN_ID, Arrays.asList("matic"));
            put(BraveWalletConstants.FANTOM_MAINNET_CHAIN_ID, Arrays.asList("ftm"));
            put(BraveWalletConstants.CELO_MAINNET_CHAIN_ID, Arrays.asList("celo"));
            put(BraveWalletConstants.BINANCE_SMART_CHAIN_MAINNET_CHAIN_ID, Arrays.asList("bnb"));
            put(BraveWalletConstants.SOLANA_MAINNET, Arrays.asList("sol"));
            put(BraveWalletConstants.FILECOIN_MAINNET, Arrays.asList("fil"));
            put(BraveWalletConstants.AVALANCHE_MAINNET_CHAIN_ID, Arrays.asList("avax", "avaxc"));
            // Test Net
            put(BraveWalletConstants.GOERLI_CHAIN_ID, Arrays.asList("eth"));
            put(BraveWalletConstants.SEPOLIA_CHAIN_ID, Arrays.asList("eth"));
            put(BraveWalletConstants.SOLANA_TESTNET, Arrays.asList("sol"));
            put(BraveWalletConstants.SOLANA_DEVNET, Arrays.asList("sol"));
        }
    };
    public static String httpifyIpfsUrl(String url) {
        if (TextUtils.isEmpty(url)) return "";
        String trimedUrl = url.trim();
        return trimedUrl.startsWith("ipfs://")
                ? trimedUrl.replace("ipfs://", "https://ipfs.io/ipfs/")
                : trimedUrl;
    }

    public static String assetRatioId(BlockchainToken token) {
        if (!TextUtils.isEmpty(token.coingeckoId)) return token.coingeckoId;
        if (BraveWalletConstants.MAINNET_CHAIN_ID.equals(token.chainId)
                || TextUtils.isEmpty(token.contractAddress))
            return token.symbol;
        return token.contractAddress;
    }

    public static Map<String, Integer> toNetworkIndexMap(List<NetworkInfo> networks) {
        return IntStream.range(0, networks.size())
                .boxed()
                .collect(toMap(i
                        -> networks.get(i).chainId,
                        Function.identity(), (index1, index2) -> index1));
    }
}
