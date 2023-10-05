/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/constants/webui_url_constants.h"
#include "brave/components/ipfs/buildflags/buildflags.h"
#include "build/build_config.h"

const char kAdblockHost[] = "adblock";
const char kAdblockInternalsHost[] = "adblock-internals";
const char kAdblockJS[] = "brave_adblock.js";
const char kSkusInternalsHost[] = "skus-internals";
#if BUILDFLAG(ENABLE_IPFS_INTERNALS_WEBUI)
const char kIPFSWebUIHost[] = "ipfs-internals";
const char kIPFSWebUIURL[] = "chrome://ipfs-internals/";
#endif
const char kWebcompatReporterHost[] = "webcompat";
const char kRewardsPageHost[] = "rewards";
const char kRewardsInternalsHost[] = "rewards-internals";
const char kWelcomeHost[] = "welcome";
const char kWelcomeJS[] = "brave_welcome.js";
const char kBraveRewardsPanelURL[] = "chrome://rewards-panel.top-chrome";
const char kBraveRewardsPanelHost[] = "rewards-panel.top-chrome";
const char kBraveTipPanelURL[] = "chrome://tip-panel.top-chrome";
const char kBraveTipPanelHost[] = "tip-panel.top-chrome";
const char kBraveNewTabJS[] = "brave_new_tab.js";
const char kBraveNewsInternalsHost[] = "news-internals";
const char kBraveUIRewardsURL[] = "chrome://rewards/";
const char kBraveUIAdblockURL[] = "chrome://adblock/";
const char kBraveUIWebcompatReporterURL[] = "chrome://webcompat/";
const char kBraveUIWalletURL[] = "chrome://wallet/";
const char kBraveUIWalletOnboardingURL[] = "brave://wallet/crypto/onboarding";
const char kBraveUIWalletAccountCreationURL[] =
    "brave://wallet/crypto/accounts/add-account/create/";
const char kBraveUIWalletPanelURL[] = "chrome://wallet-panel.top-chrome/";
const char kWalletPanelHost[] = "wallet-panel.top-chrome";
const char kVPNPanelURL[] = "chrome-untrusted://vpn-panel.top-chrome/";
const char kVPNPanelHost[] = "vpn-panel.top-chrome";
const char kBraveUIWalletPageURL[] = "chrome://wallet/";
const char kWalletPageHost[] = "wallet";
#if BUILDFLAG(IS_ANDROID)
const char kWalletPagePaths[300][100] = {
    // WalletOrigin
    "chrome://wallet",
    // fund wallet page
    "/crypto/fund-wallet",
    "/crypto/deposit-funds",
    "/crypto/deposit-funds/account",
    // market
    "/crypto/market",
    // accounts
    "/crypto/accounts",
    // add account modals
    "/crypto/accounts/add-account",
    "/crypto/accounts/add-account/create/",
    // import account modals
    "/crypto/accounts/add-account/import/",
    // wallet backup
    "/crypto/backup-wallet",
    "/crypto/backup-wallet/explain-recovery-phrase",
    "/crypto/backup-wallet/backup-recovery-phrase",
    "/crypto/backup-wallet/verify-recovery-phrase",
    // wallet management
    "/crypto/restore-wallet",
    "/crypto/unlock",
    // Activity (Transactions)
    "/crypto/activity",
    // portfolio
    "/crypto/portfolio",
    "/crypto/portfolio/assets",
    "/crypto/portfolio/nfts",
    // portfolio asset modals
    "/crypto/portfolio/add-asset",
    // swap
    "/swap",
    // send
    "/send",
    // dev bitcoin screen
    "/dev-bitcoin",
    // NFT Pining
    "/crypto/local-ipfs-node",
    "/crypto/inspect-nfts",
    
};
const char kWalletPagePath[] = "/crypto/portfolio/assets";
const char kWalletBuyPagePath[] = "/fund-wallet";
const char kWalletSendPagePath[] = "/send";
const char kWalletSwapPagePath[] = "/swap";
const char kWalletDepositPagePath[] = "/deposit-funds";
#endif  // BUILDFLAG(IS_ANDROID)
const char kExtensionSettingsURL[] = "brave://settings/extensions";
const char kWalletSettingsURL[] = "brave://settings/wallet";
const char kBraveSyncPath[] = "braveSync";
const char kBraveSyncSetupPath[] = "braveSync/setup";
const char kTorInternalsHost[] = "tor-internals";
const char kUntrustedLedgerHost[] = "ledger-bridge";
const char kUntrustedLedgerURL[] = "chrome-untrusted://ledger-bridge/";
const char kUntrustedNftHost[] = "nft-display";
const char kUntrustedNftURL[] = "chrome-untrusted://nft-display/";
const char kUntrustedMarketHost[] = "market-display";
const char kUntrustedMarketURL[] = "chrome-untrusted://market-display/";
const char kUntrustedTrezorHost[] = "trezor-bridge";
const char kUntrustedTrezorURL[] = "chrome-untrusted://trezor-bridge/";
const char kShieldsPanelURL[] = "chrome://brave-shields.top-chrome";
const char kShieldsPanelHost[] = "brave-shields.top-chrome";
const char kCookieListOptInHost[] = "cookie-list-opt-in.top-chrome";
const char kCookieListOptInURL[] = "chrome://cookie-list-opt-in.top-chrome";
const char kFederatedInternalsURL[] = "brave://federated-internals";
const char kFederatedInternalsHost[] = "federated-internals";
const char kContentFiltersPath[] = "shields/filters";
const char kPlaylistHost[] = "playlist";
const char kPlaylistURL[] = "chrome-untrusted://playlist/";
const char kPlaylistPlayerHost[] = "playlist-player";
const char kPlaylistPlayerURL[] = "chrome-untrusted://playlist-player/";
const char kSpeedreaderPanelURL[] = "chrome://brave-speedreader.top-chrome";
const char kSpeedreaderPanelHost[] = "brave-speedreader.top-chrome";
const char kShortcutsURL[] = "chrome://settings/system/shortcuts";
const char kChatUIURL[] = "chrome-untrusted://chat/";
const char kChatUIHost[] = "chat";
