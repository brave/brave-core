/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_CONSTANTS_WEBUI_URL_CONSTANTS_H_
#define BRAVE_COMPONENTS_CONSTANTS_WEBUI_URL_CONSTANTS_H_

#include "brave/components/ipfs/buildflags/buildflags.h"
#include "build/build_config.h"

extern const char kAdblockHost[];
extern const char kAdblockInternalsHost[];
extern const char kAdblockJS[];
extern const char kSkusInternalsHost[];
#if BUILDFLAG(ENABLE_IPFS_INTERNALS_WEBUI)
extern const char kIPFSWebUIHost[];
extern const char kIPFSWebUIURL[];
#endif
extern const char kWebcompatReporterHost[];
extern const char kRewardsPageHost[];
extern const char kRewardsInternalsHost[];
extern const char kWelcomeHost[];
extern const char kWelcomeJS[];
extern const char kBraveNewTabJS[];
extern const char kBraveNewsInternalsHost[];
extern const char kBraveRewardsPanelURL[];
extern const char kBraveRewardsPanelHost[];
extern const char kBraveTipPanelURL[];
extern const char kBraveTipPanelHost[];
extern const char kBraveUIRewardsURL[];
extern const char kBraveUIAdblockURL[];
extern const char kBraveUIWebcompatReporterURL[];
extern const char kBraveUIWalletURL[];
extern const char kBraveUIWalletOnboardingURL[];
extern const char kBraveUIWalletAccountCreationURL[];
extern const char kBraveUIWalletPanelURL[];
extern const char kWalletPanelHost[];
extern const char kVPNPanelURL[];
extern const char kVPNPanelHost[];
extern const char kBraveUIWalletPageURL[];
extern const char kWalletPageHost[];
#if BUILDFLAG(IS_ANDROID)
extern const char kWalletPagePaths[300][100];
extern const char kWalletPagePath[];
extern const char kWalletBuyPagePath[];
extern const char kWalletSendPagePath[];
extern const char kWalletSwapPagePath[];
extern const char kWalletDepositPagePath[];
#endif  // BUILDFLAG(IS_ANDROID)
extern const char kExtensionSettingsURL[];
extern const char kWalletSettingsURL[];
extern const char kBraveSyncPath[];
extern const char kBraveSyncSetupPath[];
extern const char kTorInternalsHost[];
extern const char kUntrustedLedgerHost[];
extern const char kUntrustedLedgerURL[];
extern const char kUntrustedNftHost[];
extern const char kUntrustedNftURL[];
extern const char kUntrustedMarketHost[];
extern const char kUntrustedMarketURL[];
extern const char kUntrustedTrezorHost[];
extern const char kUntrustedTrezorURL[];
extern const char kShieldsPanelURL[];
extern const char kShieldsPanelHost[];
extern const char kCookieListOptInHost[];
extern const char kCookieListOptInURL[];
extern const char kFederatedInternalsURL[];
extern const char kFederatedInternalsHost[];
extern const char kContentFiltersPath[];
extern const char kPlaylistHost[];
extern const char kPlaylistURL[];
extern const char kPlaylistPlayerHost[];
extern const char kPlaylistPlayerURL[];
extern const char kSpeedreaderPanelURL[];
extern const char kSpeedreaderPanelHost[];
extern const char kShortcutsURL[];
extern const char kChatUIURL[];
extern const char kChatUIHost[];

#endif  // BRAVE_COMPONENTS_CONSTANTS_WEBUI_URL_CONSTANTS_H_
