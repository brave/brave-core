/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.util;

import org.chromium.brave_wallet.mojom.BraveWalletConstants;
import org.chromium.brave_wallet.mojom.CoinType;
import org.chromium.brave_wallet.mojom.TransactionType;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.Map;

public final class WalletConstants {
    // Radius of the oval used to round the corners in density-independent pixels.
    public static final int RECT_SHARP_ROUNDED_CORNERS_DP = 12;

    public static final long MILLI_SECOND = 1000;

    // The maximum bitmap size used by {@code WebContents#downloadImage}.
    public static final int MAX_BITMAP_SIZE_FOR_DOWNLOAD = 2048;

    // Android
    public static final String LINE_SEPARATOR = "line.separator";

    // Brave
    public static final String URL_SIGN_TRANSACTION_REQUEST =
            "https://support.brave.com/hc/en-us/articles/4409513799693";

    // NFT Auto Discovery
    public static final String NFT_DISCOVERY_LEARN_MORE_LINK =
            "https://github.com/brave/brave-browser/wiki/NFT-Discovery";

    public static final String WALLET_HELP_CENTER =
            "https://support.brave.com/hc/en-us/categories/360001059151-Brave-Wallet";

    // Regex
    public static final String REGX_ANY_ETH_ADDRESS = ".*(0x[a-fA-F0-9]{40}).*";

    // Android resources
    public static final String RESOURCE_ID = "id";

    // Chromium resources
    public static final String PERMISSION_DIALOG_POSITIVE_BUTTON_ID = "positive_button";

    // AdvanceTxSettingActivity
    public static final String ADVANCE_TX_SETTING_INTENT_TX_ID = "advance-tx-setting-intent-tx-id";
    public static final String ADVANCE_TX_SETTING_INTENT_TX_CHAIN_ID =
            "advance-tx-setting-intent-tx-chain-id";
    public static final String ADVANCE_TX_SETTING_INTENT_TX_NONCE =
            "advance-tx-setting-intent-tx-nonce";
    public static final String ADVANCE_TX_SETTING_INTENT_RESULT_NONCE =
            "advance-tx-setting-intent-result-nonce";
    // BraveWalletAddNetworksFragment
    public static final String ADD_NETWORK_FRAGMENT_ARG_ACTIVE_NETWORK = "activeNetwork";
    public static final String ADD_NETWORK_FRAGMENT_ARG_CHAIN_ID = "chainId";

    // AssetDetailsActivity
    public static final String PREF_SHOW_BRIDGE_INFO_DIALOG = "pref_show_bridge_info_dialog";

    // To clear prefs while resetting wallet
    public static final String[] BRAVE_WALLET_PREFS = {PREF_SHOW_BRIDGE_INFO_DIALOG};

    public static List<String> SUPPORTED_TOP_LEVEL_CHAIN_IDS =
            Arrays.asList(
                    BraveWalletConstants.MAINNET_CHAIN_ID,
                    BraveWalletConstants.SOLANA_MAINNET,
                    BraveWalletConstants.FILECOIN_MAINNET,
                    BraveWalletConstants.BITCOIN_MAINNET);

    // Solana
    public static final String SOL = "SOL";
    public static final String SOL_LAMPORTS = "lamports";
    public static final String SOL_DAPP_FROM_ACCOUNT = "from_account";
    public static final String SOL_DAPP_TO_ACCOUNT = "to_account";
    public static final String SOL_DAPP_NONCE_ACCOUNT = "nonce_account";
    public static final String SOL_DAPP_NEW_ACCOUNT = "new_account";

    public static List<Integer> SOLANA_TRANSACTION_TYPES =
            Arrays.asList(
                    TransactionType.SOLANA_SYSTEM_TRANSFER,
                    TransactionType.SOLANA_SPL_TOKEN_TRANSFER,
                    TransactionType
                            .SOLANA_SPL_TOKEN_TRANSFER_WITH_ASSOCIATED_TOKEN_ACCOUNT_CREATION,
                    TransactionType.SOLANA_DAPP_SIGN_TRANSACTION,
                    TransactionType.SOLANA_DAPP_SIGN_AND_SEND_TRANSACTION,
                    TransactionType.SOLANA_SWAP);

    public static List<Integer> SOLANA_DAPPS_TRANSACTION_TYPES =
            Arrays.asList(
                    TransactionType.SOLANA_DAPP_SIGN_TRANSACTION,
                    TransactionType.SOLANA_DAPP_SIGN_AND_SEND_TRANSACTION);

    // Solana instruction types
    public static final String SOL_INS_SYSTEM = BraveWalletConstants.SOLANA_SYSTEM_PROGRAM_ID;
    public static final String SOL_INS_TOKEN = BraveWalletConstants.SOLANA_TOKEN_PROGRAM_ID;
    public static final String SOL_INS_CONFIG = "Config1111111111111111111111111111111111111";
    public static final String SOL_INS_STAKE = "Stake11111111111111111111111111111111111111";
    public static final String SOL_INS_VOTE = "Vote111111111111111111111111111111111111111";
    public static final String SOL_INS_BPF = "BPFLoaderUpgradeab1e11111111111111111111111";
    public static final String SOL_INS_SIG_VERIFY = "Ed25519SigVerify111111111111111111111111111";
    public static final String SOL_INS_SECP = "KeccakSecp256k11111111111111111111111111111";

    public static final Map<String, Integer> KNOWN_TEST_CHAINS_MAP =
            Map.of(
                    BraveWalletConstants.SEPOLIA_CHAIN_ID, CoinType.ETH, //
                    BraveWalletConstants.LOCALHOST_CHAIN_ID, CoinType.ETH, //
                    BraveWalletConstants.SOLANA_TESTNET, CoinType.SOL, //
                    BraveWalletConstants.SOLANA_DEVNET, CoinType.SOL, //
                    BraveWalletConstants.FILECOIN_TESTNET, CoinType.FIL, //
                    BraveWalletConstants.FILECOIN_ETHEREUM_TESTNET_CHAIN_ID, CoinType.ETH, //
                    BraveWalletConstants.BITCOIN_TESTNET, CoinType.BTC); //

    public static final List<String> KNOWN_TEST_CHAIN_IDS =
            new ArrayList<>(KNOWN_TEST_CHAINS_MAP.keySet());

    public static final List<Integer> SUPPORTED_COIN_TYPES_ON_DAPPS =
            Arrays.asList(CoinType.ETH, CoinType.SOL);
}
