/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_FLAG_DESCRIPTIONS_H_
#define BRAVE_BROWSER_FLAG_DESCRIPTIONS_H_

#include "brave/components/brave_rewards/common/buildflags/buildflags.h"

namespace flag_descriptions {

constexpr char kBraveAdblockCnameUncloakingName[] = "Enable CNAME uncloaking";
constexpr char kBraveAdblockCnameUncloakingDescription[] =
    "Take DNS CNAME records into account when making network request blocking "
    "decisions.";

constexpr char kBraveAdblockCollapseBlockedElementsName[] =
    "Collapse HTML elements with blocked source attributes";
constexpr char kBraveAdblockCollapseBlockedElementsDescription[] =
    "Cause iframe and img elements to be collapsed if the URL of their src "
    "attribute is blocked";

constexpr char kBraveAdblockCosmeticFilteringName[] =
    "Enable cosmetic filtering";
constexpr char kBraveAdblockCosmeticFilteringDescription[] =
    "Enable support for cosmetic filtering";

constexpr char kBraveAdblockCosmeticFilteringNativeName[] =
    "Use native implementation for cosmetic filtering";
constexpr char kBraveAdblockCosmeticFilteringNativeDescription[] =
    "Uses native implementation for cosmetic filtering instead of extension";

constexpr char kBraveAdblockCspRulesName[] = "Enable support for CSP rules";
constexpr char kBraveAdblockCspRulesDescription[] =
    "Applies additional CSP rules to pages for which a $csp rule has been "
    "loaded from a filter list";

constexpr char kBraveAdblockDefault1pBlockingName[] =
    "Shields first-party network blocking";
constexpr char kBraveAdblockDefault1pBlockingDescription[] =
    "Allow Brave Shields to block first-party network requests in Standard "
    "blocking mode";

constexpr char kBraveAdsCustomNotificationsName[] =
    "Enable Brave Ads custom notifications";
constexpr char kBraveAdsCustomNotificationsDescription[] =
    "Enable Brave Ads custom notifications to support rich media";

constexpr char kBraveDarkModeBlockName[] =
    "Enable dark mode blocking fingerprinting protection";
constexpr char kBraveDarkModeBlockDescription[] =
    "Always report light mode when fingerprinting protections set to Strict";

constexpr char kBraveDomainBlockName[] = "Enable domain blocking";
constexpr char kBraveDomainBlockDescription[] =
    "Enable support for blocking domains with an interstitial page";

constexpr char kBraveExtensionNetworkBlockingName[] =
    "Enable extension network blocking";
constexpr char kBraveExtensionNetworkBlockingDescription[] =
    "Enable blocking for network requests initiated by extensions";

constexpr char kBraveIpfsName[] = "Enable IPFS";
constexpr char kBraveIpfsDescription[] = "Enable native support of IPFS.";

constexpr char kBraveNTPBrandedWallpaperName[] =
    "New Tab Page Branded Wallpapers";
constexpr char kBraveNTPBrandedWallpaperDescription[] =
    "Allow New Tab Page Branded Wallpapers and user preference.";

constexpr char kBraveNTPBrandedWallpaperDemoName[] =
    "New Tab Page Demo Branded Wallpaper";
constexpr char kBraveNTPBrandedWallpaperDemoDescription[] =
    "Force dummy data for the Branded Wallpaper New Tab Page Experience. "
    "View rate and user opt-in conditionals will still be followed to decide "
    "when to display the Branded Wallpaper.";

constexpr char kBraveSidebarName[] = "Enable Sidebar";
// TODO(simon): Use better description.
constexpr char kBraveSidebarDescription[] = "Enable Sidebar";

constexpr char kBraveSpeedreaderName[] = "Enable SpeedReader";
constexpr char kBraveSpeedreaderDescription[] =
    "Enables faster loading of simplified article-style web pages.";

constexpr char kBraveSpeedreaderLegacyName[] =
    "Enable legacy adblock based backend for SpeedReader";
constexpr char kBraveSpeedreaderLegacyDescription[] =
    "Enables the legacy backend for SpeedReader. Uses adblock rules to "
    "determine if pages are readable and distills using CSS selector rules.";

constexpr char kBraveSyncName[] = "Enable Brave Sync v2";
constexpr char kBraveSyncDescription[] =
    "Brave Sync v2 integrates with chromium sync engine with Brave specific "
    "authentication flow and enforce client side encryption";

constexpr char kBraveVPNName[] = "Enable experimental Brave VPN";
constexpr char kBraveVPNDescription[] = "Experimental native VPN support";

constexpr char kBraveDecentralizedDnsName[] = "Enable decentralized DNS";
constexpr char kBraveDecentralizedDnsDescription[] =
    "Enable decentralized DNS support, such as Unstoppable Domains and "
    "Ethereum Name Service (ENS).";

constexpr char kBraveEphemeralStorageName[] = "Enable Ephemeral Storage";
constexpr char kBraveEphemeralStorageDescription[] =
    "Use ephemeral storage for third-party frames";

constexpr char kBraveEphemeralStorageKeepAliveName[] =
    "Ephemeral Storage Keep Alive";
constexpr char kBraveEphemeralStorageKeepAliveDescription[] =
    "Keep ephemeral storage partitions alive for a specified time after all "
    "tabs for that origin are closed";

#if BUILDFLAG(ENABLE_GEMINI_WALLET)
constexpr char kBraveRewardsGeminiName[] = "Enable Gemini for Brave Rewards";
constexpr char kBraveRewardsGeminiDescription[] =
    "Enables support for Gemini as an external wallet provider for Brave";
#endif

constexpr char kBraveRewardsVerboseLoggingName[] =
    "Enable Brave Rewards verbose logging";
constexpr char kBraveRewardsVerboseLoggingDescription[] =
    "Enables detailed logging of Brave Rewards system events to a log file "
    "stored on your device. Please note that this log file could include "
    "information such as browsing history and credentials such as passwords "
    "and access tokens depending on your activity. Please do not share it "
    "unless asked to by Brave staff.";

constexpr char kBraveSearchDefaultAPIName[] =
    "Enable Brave Search website default search provider API";
constexpr char kBraveSearchDefaultAPIDescription[] =
    "Enable javascript API only on "
    "Brave Search websites which will allow the user to make the search engine "
    "their default search provider.";

constexpr char kBraveSuperReferralName[] = "Enable Brave Super Referral";
constexpr char kBraveSuperReferralDescription[] =
    "Use custom theme for Brave Super Referral";

constexpr char kBraveTalkName[] = "Enable Brave Talk";
constexpr char kBraveTalkDescription[] =
    "Enables the Brave Talk integration on the new tab page. You can "
    "use Brave Talk to start a private video call with your friends "
    "and colleagues.";

constexpr char kNativeBraveWalletName[] =
    "Enable experimental Brave native wallet";
constexpr char kNativeBraveWalletDescription[] =
    "Experimental native cryptocurrency wallet support without the use of "
    "extensions";

constexpr char kUseDevUpdaterUrlName[] = "Use dev updater url";
constexpr char kUseDevUpdaterUrlDescription[] =
    "Use the dev url for the component updater. "
    "This is for internal testing only.";

}  // namespace flag_descriptions

#endif  // BRAVE_BROWSER_FLAG_DESCRIPTIONS_H_
