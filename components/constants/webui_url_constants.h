/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_CONSTANTS_WEBUI_URL_CONSTANTS_H_
#define BRAVE_COMPONENTS_CONSTANTS_WEBUI_URL_CONSTANTS_H_

#include "brave/components/ipfs/buildflags/buildflags.h"
#include "build/build_config.h"

inline constexpr char kAdblockHost[] = "adblock";
inline constexpr char kAdblockInternalsHost[] = "adblock-internals";
inline constexpr char kAdblockJS[] = "brave_adblock.js";
inline constexpr char kSkusInternalsHost[] = "skus-internals";
#if BUILDFLAG(ENABLE_IPFS_INTERNALS_WEBUI)
inline constexpr char kIPFSWebUIHost[] = "ipfs-internals";
inline constexpr char kIPFSWebUIURL[] = "chrome://ipfs-internals/";
#endif
inline constexpr char kWebcompatReporterHost[] = "webcompat";
inline constexpr char kRewardsPageHost[] = "rewards";
inline constexpr char kRewardsInternalsHost[] = "rewards-internals";
inline constexpr char kWelcomeHost[] = "welcome";
inline constexpr char kWelcomeJS[] = "brave_welcome.js";
inline constexpr char kBraveRewardsPanelURL[] =
    "chrome://rewards-panel.top-chrome";
inline constexpr char kBraveRewardsPanelHost[] = "rewards-panel.top-chrome";
inline constexpr char kBraveTipPanelURL[] = "chrome://tip-panel.top-chrome";
inline constexpr char kBraveTipPanelHost[] = "tip-panel.top-chrome";
inline constexpr char kBraveNewTabJS[] = "brave_new_tab.js";
inline constexpr char kBraveNewsInternalsHost[] = "news-internals";
inline constexpr char16_t kBraveUIRewardsURL[] = u"chrome://rewards/";
inline constexpr char kBraveUIAdblockURL[] = "chrome://adblock/";
inline constexpr char kBraveUIWebcompatReporterURL[] = "chrome://webcompat/";
inline constexpr char kBraveUIWalletURL[] = "chrome://wallet/";
inline constexpr char kBraveUIWalletOnboardingURL[] =
    "brave://wallet/crypto/onboarding";
inline constexpr char kBraveUIWalletAccountCreationURL[] =
    "brave://wallet/crypto/accounts/add-account/create/";
inline constexpr char kBraveUIWalletPanelURL[] =
    "chrome://wallet-panel.top-chrome/";
inline constexpr char kWalletPanelHost[] = "wallet-panel.top-chrome";
inline constexpr char kVPNPanelURL[] =
    "chrome-untrusted://vpn-panel.top-chrome/";
inline constexpr char kVPNPanelHost[] = "vpn-panel.top-chrome";
inline constexpr char kBraveUIWalletPageURL[] = "chrome://wallet/";
inline constexpr char kWalletPageHost[] = "wallet";
inline constexpr char kExtensionSettingsURL[] = "brave://settings/extensions";
inline constexpr char kWalletSettingsURL[] = "brave://settings/wallet";
inline constexpr char kBraveSyncPath[] = "braveSync";
inline constexpr char kBraveSyncSetupPath[] = "braveSync/setup";
inline constexpr char kTorInternalsHost[] = "tor-internals";
inline constexpr char kUntrustedLedgerHost[] = "ledger-bridge";
inline constexpr char kUntrustedLedgerURL[] =
    "chrome-untrusted://ledger-bridge/";
inline constexpr char kUntrustedNftHost[] = "nft-display";
inline constexpr char kUntrustedNftURL[] = "chrome-untrusted://nft-display/";
inline constexpr char kUntrustedLineChartHost[] = "line-chart-display";
inline constexpr char kUntrustedLineChartURL[] =
    "chrome-untrusted://line-chart-display/";
inline constexpr char kUntrustedMarketHost[] = "market-display";
inline constexpr char kUntrustedMarketURL[] =
    "chrome-untrusted://market-display/";
inline constexpr char kUntrustedTrezorHost[] = "trezor-bridge";
inline constexpr char kUntrustedTrezorURL[] =
    "chrome-untrusted://trezor-bridge/";
inline constexpr char kShieldsPanelURL[] = "chrome://brave-shields.top-chrome";
inline constexpr char kShieldsPanelHost[] = "brave-shields.top-chrome";
inline constexpr char kCookieListOptInHost[] = "cookie-list-opt-in.top-chrome";
inline constexpr char kCookieListOptInURL[] =
    "chrome://cookie-list-opt-in.top-chrome";
inline constexpr char kFederatedInternalsURL[] = "brave://federated-internals";
inline constexpr char kFederatedInternalsHost[] = "federated-internals";
inline constexpr char kContentFiltersPath[] = "shields/filters";
inline constexpr char kPlaylistHost[] = "playlist";
inline constexpr char kPlaylistURL[] = "chrome-untrusted://playlist/";
inline constexpr char kPlaylistPlayerHost[] = "playlist-player";
inline constexpr char kPlaylistPlayerURL[] =
    "chrome-untrusted://playlist-player/";
inline constexpr char kSpeedreaderPanelURL[] =
    "chrome://brave-speedreader.top-chrome";
inline constexpr char kSpeedreaderPanelHost[] = "brave-speedreader.top-chrome";
inline constexpr char kShortcutsURL[] = "chrome://settings/system/shortcuts";
inline constexpr char kChatUIURL[] = "chrome-untrusted://chat/";
inline constexpr char kChatUIHost[] = "chat";

#endif  // BRAVE_COMPONENTS_CONSTANTS_WEBUI_URL_CONSTANTS_H_
