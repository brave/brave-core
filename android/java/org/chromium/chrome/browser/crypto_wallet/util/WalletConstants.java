package org.chromium.chrome.browser.crypto_wallet.util;

import org.chromium.brave_wallet.mojom.BraveWalletConstants;

import java.util.Arrays;
import java.util.List;

public final class WalletConstants {
    public static final long MILLI_SECOND = 1000;

    // URLs

    // Crypto
    public static final String URL_BRIDGE_RISKS = "https://ethereum.org/en/bridges/#bridge-risk";

    // Aurora
    public static final String URL_RAINBOW_AURORA = "https://rainbowbridge.app";

    // Android resources
    public static final String RESOURCE_ID = "id";

    // Chromium resources
    public static final String PERMISSION_DIALOG_POSITIVE_BUTTON_ID = "positive_button";

    // AdvanceTxSettingActivity
    public static final String ADVANCE_TX_SETTING_INTENT_TX_ID = "advance-tx-setting-intent-tx-id";
    public static final String ADVANCE_TX_SETTING_INTENT_TX_NONCE =
            "advance-tx-setting-intent-tx-nonce";
    public static final String ADVANCE_TX_SETTING_INTENT_RESULT_NONCE =
            "advance-tx-setting-intent-result-nonce";
    // BraveWalletAddNetworksFragment
    public static final String ADD_NETWORK_FRAGMENT_ARG_ACTIVE_NETWORK = "activeNetwork";
    public static final String ADD_NETWORK_FRAGMENT_ARG_CHAIN_ID = "chainId";

    // AssetDetailsActivity
    public static final String PREF_SHOW_BRIDGE_INFO_DIALOG = "pref_show_bridge_info_dialog";

    public static List<String> BUY_SUPPORTED_NETWORKS = Arrays.asList(
            BraveWalletConstants.MAINNET_CHAIN_ID, BraveWalletConstants.RINKEBY_CHAIN_ID,
            BraveWalletConstants.ROPSTEN_CHAIN_ID, BraveWalletConstants.GOERLI_CHAIN_ID,
            BraveWalletConstants.KOVAN_CHAIN_ID, BraveWalletConstants.LOCALHOST_CHAIN_ID,
            BraveWalletConstants.POLYGON_MAINNET_CHAIN_ID,
            BraveWalletConstants.BINANCE_SMART_CHAIN_MAINNET_CHAIN_ID,
            BraveWalletConstants.AVALANCHE_MAINNET_CHAIN_ID,
            BraveWalletConstants.CELO_MAINNET_CHAIN_ID,
            BraveWalletConstants.FANTOM_MAINNET_CHAIN_ID,
            BraveWalletConstants.OPTIMISM_MAINNET_CHAIN_ID,
            BraveWalletConstants.AURORA_MAINNET_CHAIN_ID);
    //            BraveWalletConstants.SOLANA_MAINNET, /* not yet supported */
    //            BraveWalletConstants.FILECOIN_MAINNET); , /* not yet supported */
}
