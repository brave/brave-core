package org.chromium.chrome.browser.crypto_wallet.util;

import org.chromium.brave_wallet.mojom.BraveWalletConstants;
import org.chromium.chrome.R;

import java.util.Arrays;
import java.util.List;

public final class WalletConstants {
    public static final long MILLI_SECOND = 1000;

    // Android
    public static final String LINE_SEPARATOR = "line.separator";

    // Crypto
    public static final String URL_RAINBOW_BRIDGE_OVERVIEW =
            "https://doc.aurora.dev/bridge/bridge-overview/";
    public static final String URL_RAINBOW_BRIDGE_RISKS = "https://rainbowbridge.app/approvals";

    // Brave
    public static final String URL_SIGN_TRANSACTION_REQUEST =
            "https://support.brave.com/hc/en-us/articles/4409513799693";

    // Regex
    public static final String REGX_ANY_ETH_ADDRESS = ".*(0x[a-fA-F0-9]{40}).*";

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

    // To clear prefs while resetting wallet
    public static final String[] BRAVE_WALLET_PREFS = {PREF_SHOW_BRIDGE_INFO_DIALOG};

    public static List<String> BUY_SUPPORTED_NETWORKS =
            Arrays.asList(BraveWalletConstants.MAINNET_CHAIN_ID);
    //            BraveWalletConstants.RINKEBY_CHAIN_ID,
    //            BraveWalletConstants.ROPSTEN_CHAIN_ID, BraveWalletConstants.GOERLI_CHAIN_ID,
    //            BraveWalletConstants.KOVAN_CHAIN_ID, BraveWalletConstants.LOCALHOST_CHAIN_ID
    //            BraveWalletConstants.POLYGON_MAINNET_CHAIN_ID, /* not yet supported */
    //            BraveWalletConstants.AVALANCHE_MAINNET_CHAIN_ID, /* not yet supported */
    //            BraveWalletConstants.AURORA_MAINNET_CHAIN_ID, /* not yet supported */
    //            BraveWalletConstants.OPTIMISM_MAINNET_CHAIN_ID, /* disabled until Ramp support */
    //            BraveWalletConstants.CELO_MAINNET_CHAIN_ID, /* disabled until Ramp support */
    //            BraveWalletConstants.BINANCE_SMART_CHAIN_MAINNET_CHAIN_ID, /* disabled until Ramp
    //            support */ BraveWalletConstants.FANTOM_MAINNET_CHAIN_ID, /* not yet supported */
    //            BraveWalletConstants.SOLANA_MAINNET, /* not yet supported */
    //            BraveWalletConstants.FILECOIN_MAINNET); , /* not yet supported */

    public static List<String> SWAP_SUPPORTED_NETWORKS = Arrays.asList(
            BraveWalletConstants.MAINNET_CHAIN_ID, BraveWalletConstants.GOERLI_CHAIN_ID);

    // Solana
    public static final String SOL_LAMPORTS = "lamports";
    public static final String SOL = "SOL";

    // Solana instruction types
    public static final String SOL_INS_SYSTEM = BraveWalletConstants.SOLANA_SYSTEM_PROGRAM_ID;
    public static final String SOL_INS_TOKEN = BraveWalletConstants.SOLANA_TOKEN_PROGRAM_ID;
    public static final String SOL_INS_CONFIG = "Config1111111111111111111111111111111111111";
    public static final String SOL_INS_STAKE = "Stake11111111111111111111111111111111111111";
    public static final String SOL_INS_VOTE = "Vote111111111111111111111111111111111111111";
    public static final String SOL_INS_BPF = "BPFLoaderUpgradeab1e11111111111111111111111";
    public static final String SOL_INS_SIG_VERIFY = "Ed25519SigVerify111111111111111111111111111";
    public static final String SOL_INS_SECP = "KeccakSecp256k11111111111111111111111111111";
}
