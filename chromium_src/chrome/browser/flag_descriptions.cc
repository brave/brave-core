/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "../../../../chrome/browser/flag_descriptions.cc"

namespace flag_descriptions {
const char kUseDevUpdaterUrlName[] =
    "Use dev updater url";
const char kUseDevUpdaterUrlDescription[] =
    "Use the dev url for the component updater. "
    "This is for internal testing only.";
const char kBraveNTPBrandedWallpaperName[] =
    "New Tab Page Branded Wallpapers";
const char kBraveNTPBrandedWallpaperDescription[] =
    "Allow New Tab Page Branded Wallpapers and user preference.";
const char kBraveNTPBrandedWallpaperDemoName[] =
    "New Tab Page Demo Branded Wallpaper";
const char kBraveNTPBrandedWallpaperDemoDescription[] =
    "Force dummy data for the Branded Wallpaper New Tab Page Experience. "
    "View rate and user opt-in conditionals will still be followed to decide "
    "when to display the Branded Wallpaper.";
const char kBraveAdblockCnameUncloakingName[] = "Enable CNAME uncloaking";
const char kBraveAdblockCnameUncloakingDescription[] =
    "Take DNS CNAME records into account when making network request blocking "
    "decisions.";
const char kBraveAdblockCosmeticFilteringName[] = "Enable cosmetic filtering";
const char kBraveAdblockCosmeticFilteringDescription[] =
    "Enable support for cosmetic filtering";
const char kBraveAdblockCosmeticFilteringNativeName[] =
    "Use native implementation for cosmetic filtering";
const char kBraveAdblockCosmeticFilteringNativeDescription[] =
    "Uses native implementation for cosmetic filtering instead of extension";
const char kBraveAdblockCspRulesName[] = "Enable support for CSP rules";
const char kBraveAdblockCspRulesDescription[] =
    "Applies additional CSP rules to pages for which a $csp rule has been "
    "loaded from a filter list";
const char kBraveDomainBlockName[] = "Enable domain blocking";
const char kBraveDomainBlockDescription[] =
    "Enable support for blocking domains with an interstitial page";
const char kBraveExtensionNetworkBlockingName[] =
    "Enable extension network blocking";
const char kBraveExtensionNetworkBlockingDescription[] =
    "Enable blocking for network requests initiated by extensions";
const char kBraveSidebarName[] = "Enable Sidebar";
// TODO(simon): Use more better description.
const char kBraveSidebarDescription[] = "Enable Sidebar";
const char kBraveSpeedreaderName[] = "Enable SpeedReader";
const char kBraveSpeedreaderDescription[] =
    "Enables faster loading of simplified article-style web pages.";
const char kBraveSpeedreaderLegacyName[] =
    "Enable legacy adblock based backend for SpeedReader";
const char kBraveSpeedreaderLegacyDescription[] =
    "Enables the legacy backend for SpeedReader. Uses adblock rules to "
    "determine if pages are readable and distills using CSS selector rules.";
const char kBraveSyncName[] = "Enable Brave Sync v2";
const char kBraveSyncDescription[] =
    "Brave Sync v2 integrates with chromium sync engine with Brave specific "
    "authentication flow and enforce client side encryption";
const char kBraveIpfsName[] = "Enable IPFS";
const char kBraveIpfsDescription[] =
    "Enable native support of IPFS.";
const char kBraveRewardsVerboseLoggingName[] =
    "Enable Brave Rewards verbose logging";
const char kBraveRewardsVerboseLoggingDescription[] =
    "Enables detailed logging of Brave Rewards system events to a log file "
    "stored on your device. Please note that this log file could include "
    "information such as browsing history and credentials such as passwords "
    "and access tokens depending on your activity. Please do not share it "
    "unless asked to by Brave staff.";
const char kBraveRewardsBitflyerName[] = "Enable bitFlyer for Brave Rewards";
const char kBraveRewardsBitflyerDescription[] =
    "Enables support for bitFlyer as an external wallet provider for Brave "
    "Rewards users in Japan";
const char kNativeBraveWalletName[] = "Enable experimental Brave native wallet";
const char kNativeBraveWalletDescription[] =
    "Experimental native cryptocurrency wallet support without the use of "
    "extensions";
const char kBraveDecentralizedDnsName[] = "Enable decentralized DNS";
const char kBraveDecentralizedDnsDescription[] =
    "Enable decentralized DNS support, such as Unstoppable Domains and "
    "Ethereum Name Service (ENS).";
const char kBraveSuperReferralName[] = "Enable Brave Super Referral";
const char kBraveSuperReferralDescription[] =
    "Use custom theme for Brave Super Referral";
const char kBraveEphemeralStorageName[] = "Enable Ephemeral Storage";
const char kBraveEphemeralStorageDescription[] =
    "Use ephemeral storage for third-party frames";
const char kBraveEphemeralStorageKeepAliveName[] =
    "Ephemeral Storage Keep Alive";
const char kBraveEphemeralStorageKeepAliveDescription[] =
    "Keep ephemeral storage partitions alive for a specified time after all "
    "tabs for that origin are closed";
const char kBravePermissionLifetimeName[] = "Permission Lifetime";
const char kBravePermissionLifetimeDescription[] =
    "Enables the option to choose a time period after which a permission will "
    "be automatically revoked";
}  // namespace flag_descriptions
