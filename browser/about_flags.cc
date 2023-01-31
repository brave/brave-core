/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/about_flags.h"

#include "base/strings/string_util.h"
#include "brave/browser/brave_features_internal_names.h"
#include "brave/browser/ethereum_remote_client/buildflags/buildflags.h"
#include "brave/browser/ethereum_remote_client/features.h"
#include "brave/components/brave_ads/common/features.h"
#include "brave/components/brave_component_updater/browser/features.h"
#include "brave/components/brave_federated/features.h"
#include "brave/components/brave_rewards/common/buildflags/buildflags.h"
#include "brave/components/brave_rewards/common/features.h"
#include "brave/components/brave_shields/common/features.h"
#include "brave/components/brave_sync/features.h"
#include "brave/components/brave_today/common/features.h"
#include "brave/components/brave_vpn/common/buildflags/buildflags.h"
#include "brave/components/brave_wallet/common/features.h"
#include "brave/components/de_amp/common/features.h"
#include "brave/components/debounce/common/features.h"
#include "brave/components/google_sign_in_permission/features.h"
#include "brave/components/ipfs/buildflags/buildflags.h"
#include "brave/components/ntp_background_images/browser/features.h"
#include "brave/components/playlist/common/buildflags/buildflags.h"
#include "brave/components/skus/browser/skus_utils.h"
#include "brave/components/skus/common/features.h"
#include "brave/components/speedreader/common/buildflags/buildflags.h"
#include "brave/components/translate/core/common/brave_translate_features.h"
#include "build/build_config.h"
#include "components/content_settings/core/common/features.h"
#include "components/flags_ui/flags_state.h"
#include "components/translate/core/browser/translate_prefs.h"
#include "net/base/features.h"
#include "third_party/blink/public/common/features.h"

#if BUILDFLAG(ENABLE_BRAVE_VPN)
#include "brave/components/brave_vpn/common/features.h"
#endif

#if BUILDFLAG(ENABLE_SPEEDREADER)
#include "brave/components/speedreader/common/features.h"
#endif

#if BUILDFLAG(ENABLE_IPFS)
#include "brave/components/ipfs/features.h"
#endif

#if BUILDFLAG(ENABLE_PLAYLIST)
#include "brave/components/playlist/common/features.h"
#endif

#if defined(TOOLKIT_VIEWS)
#include "brave/browser/ui/views/tabs/features.h"
#endif

#if BUILDFLAG(IS_ANDROID)
#include "brave/browser/android/preferences/features.h"
#include "brave/browser/android/safe_browsing/features.h"
#endif

using brave_shields::features::kBraveAdblockCnameUncloaking;
using brave_shields::features::kBraveAdblockCollapseBlockedElements;
using brave_shields::features::kBraveAdblockCookieListDefault;
using brave_shields::features::kBraveAdblockCookieListOptIn;
using brave_shields::features::kBraveAdblockCosmeticFiltering;
using brave_shields::features::kBraveAdblockCspRules;
using brave_shields::features::kBraveAdblockDefault1pBlocking;
using brave_shields::features::kBraveAdblockMobileNotificationsListDefault;
using brave_shields::features::kBraveDarkModeBlock;
using brave_shields::features::kBraveDomainBlock;
using brave_shields::features::kBraveDomainBlock1PES;
using brave_shields::features::kBraveExtensionNetworkBlocking;
using brave_shields::features::kBraveReduceLanguage;
using brave_shields::features::kCosmeticFilteringSyncLoad;

using de_amp::features::kBraveDeAMP;
using debounce::features::kBraveDebounce;
using google_sign_in_permission::features::kBraveGoogleSignInPermission;

using ntp_background_images::features::kBraveNTPBrandedWallpaperDemo;
using ntp_background_images::features::kBraveNTPSuperReferralWallpaper;

#if BUILDFLAG(IS_ANDROID)
using preferences::features::kBraveBackgroundVideoPlayback;
using safe_browsing::features::kBraveAndroidSafeBrowsing;
#endif

namespace flag_descriptions {

namespace {

constexpr char kBraveAdblockCnameUncloakingName[] = "Enable CNAME uncloaking";
constexpr char kBraveAdblockCnameUncloakingDescription[] =
    "Take DNS CNAME records into account when making network request blocking "
    "decisions.";

constexpr char kBraveAdblockCollapseBlockedElementsName[] =
    "Collapse HTML elements with blocked source attributes";
constexpr char kBraveAdblockCollapseBlockedElementsDescription[] =
    "Cause iframe and img elements to be collapsed if the URL of their src "
    "attribute is blocked";

constexpr char kBraveAdblockCookieListDefaultName[] =
    "Treat 'Easylist-Cookie List' as a default list source";
constexpr char kBraveAdblockCookieListDefaultDescription[] =
    "Enables the 'Easylist-Cookie List' regional list if its toggle "
    "in brave://adblock hasn't otherwise been modified";

constexpr char kBraveAdblockCookieListOptInName[] =
    "Show an opt-in bubble for the 'Easylist-Cookie List' filter";
constexpr char kBraveAdblockCookieListOptInDescription[] =
    "When enabled, a bubble will be displayed inviting the user to enable the "
    "'Easylist-Cookie List' filter for blocking cookie consent dialogs";

constexpr char kBraveAdblockCosmeticFilteringName[] =
    "Enable cosmetic filtering";
constexpr char kBraveAdblockCosmeticFilteringDescription[] =
    "Enable support for cosmetic filtering";

constexpr char kBraveAdblockCspRulesName[] = "Enable support for CSP rules";
constexpr char kBraveAdblockCspRulesDescription[] =
    "Applies additional CSP rules to pages for which a $csp rule has been "
    "loaded from a filter list";

constexpr char kBraveAdblockDefault1pBlockingName[] =
    "Shields first-party network blocking";
constexpr char kBraveAdblockDefault1pBlockingDescription[] =
    "Allow Brave Shields to block first-party network requests in Standard "
    "blocking mode";

constexpr char kBraveAdblockMobileNotificationsListDefaultName[] =
    "Treat 'Fanboy's Mobile Notifications List' as a default list source";
constexpr char kBraveAdblockMobileNotificationsListDefaultDescription[] =
    "Enables the 'Fanboy's Mobile Notifications List' regional list if its "
    "toggle in brave://adblock hasn't otherwise been modified";

constexpr char kBraveAdsCustomNotificationsName[] =
    "Enable Brave Ads custom push notifications";
constexpr char kBraveAdsCustomNotificationsDescription[] =
    "Enable Brave Ads custom push notifications to support rich media";

constexpr char kBraveAdsCustomNotificationsFallbackName[] =
    "Allow Brave Ads to fallback from native to custom push notifications";
constexpr char kBraveAdsCustomNotificationsFallbackDescription[] =
    "Allow Brave Ads to fallback from native to custom push notifications on "
    "operating systems which do not support native notifications";

constexpr char kBraveDarkModeBlockName[] =
    "Enable dark mode blocking fingerprinting protection";
constexpr char kBraveDarkModeBlockDescription[] =
    "Always report light mode when fingerprinting protections set to Strict";

constexpr char kBraveDomainBlockName[] = "Enable domain blocking";
constexpr char kBraveDomainBlockDescription[] =
    "Enable support for blocking domains with an interstitial page";

constexpr char kBraveDomainBlock1PESName[] =
    "Enable domain blocking using First Party Ephemeral Storage";
constexpr char kBraveDomainBlock1PESDescription[] =
    "When visiting a blocked domain, Brave will try to enable Ephemeral "
    "Storage for a first party context, meaning neither cookies nor "
    "localStorage data will be persisted after a website is closed. Ephemeral "
    "Storage will be auto-enabled only if no data was previously stored for a "
    "website";

constexpr char kBraveDebounceName[] = "Enable debouncing";
constexpr char kBraveDebounceDescription[] =
    "Enable support for skipping top-level redirect tracking URLs";

constexpr char kBraveDeAMPName[] = "Enable De-AMP";
constexpr char kBraveDeAMPDescription[] = "Enable De-AMPing feature";

constexpr char kBraveGoogleSignInPermissionName[] =
    "Enable Google Sign-In Permission Prompt";
constexpr char kBraveGoogleSignInPermissionDescription[] =
    "Enable permissioning access to legacy Google Sign-In";

constexpr char kBraveExtensionNetworkBlockingName[] =
    "Enable extension network blocking";
constexpr char kBraveExtensionNetworkBlockingDescription[] =
    "Enable blocking for network requests initiated by extensions";

constexpr char kBraveReduceLanguageName[] = "Reduce language identifiability";
constexpr char kBraveReduceLanguageDescription[] =
    "Reduce the identifiability of my language preferences";

constexpr char kCosmeticFilteringSyncLoadName[] =
    "Enable sync loading of cosmetic filter rules";
constexpr char kCosmeticFilteringSyncLoadDescription[] =
    "Enable sync loading of cosmetic filter rules";

constexpr char kBraveIpfsName[] = "Enable IPFS";
constexpr char kBraveIpfsDescription[] = "Enable native support of IPFS.";

constexpr char kBraveNTPBrandedWallpaperDemoName[] =
    "New Tab Page Demo Branded Wallpaper";
constexpr char kBraveNTPBrandedWallpaperDemoDescription[] =
    "Force dummy data for the Branded Wallpaper New Tab Page Experience. "
    "View rate and user opt-in conditionals will still be followed to decide "
    "when to display the Branded Wallpaper.";

constexpr char kBraveBlockScreenFingerprintingName[] =
    "Block screen fingerprinting";
constexpr char kBraveBlockScreenFingerprintingDescription[] =
    "Prevents JavaScript and CSS from learning the user's screen dimensions "
    "or window position.";

constexpr char kBraveTorWindowsHttpsOnlyName[] =
    "Use HTTPS-Only Mode in Private Windows with Tor";
constexpr char kBraveTorWindowsHttpsOnlyDescription[] =
    "Prevents Private Windows with Tor from making any insecure HTTP "
    "connections without warning the user first.";

constexpr char kBraveRoundTimeStampsName[] = "Round time stamps";
constexpr char kBraveRoundTimeStampsDescription[] =
    "Prevents JavaScript from getting access to high-resolution clocks by "
    "rounding all DOMHighResTimeStamps to the nearest millisecond.";

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
#if BUILDFLAG(IS_WIN)
constexpr char kBraveVPNDnsProtectionName[] = "Enable DoH for Brave VPN";
constexpr char kBraveVPNDnsProtectionDescription[] =
    "Override DoH settings with Cloudflare dns if necessary to avoid leaking "
    "requests due to Smart Multi-Home Named Resolution";
#endif
constexpr char kBraveSkusSdkName[] = "Enable experimental SKU SDK";
constexpr char kBraveSkusSdkDescription[] = "Experimental SKU SDK support";

constexpr char kBraveShieldsV1Name[] = "Enable Brave Shields v1";
constexpr char kBraveShieldsV1Description[] =
    "Legacy extension-based panel UX/UI for Brave Shields";
constexpr char kBraveShieldsV2Name[] = "Enable Brave Shields v2";
constexpr char kBraveShieldsV2Description[] =
    "Major UX/UI overhaul of Brave Shields panel";

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

constexpr char kBraveFirstPartyEphemeralStorageName[] =
    "Enable First Party Ephemeral Storage";
constexpr char kBraveFirstPartyEphemeralStorageDescription[] =
    "Enable support for First Party Ephemeral Storage using SESSION_ONLY "
    "cookie setting";

#if BUILDFLAG(ENABLE_GEMINI_WALLET)
constexpr char kBraveRewardsGeminiName[] = "Enable Gemini for Brave Rewards";
constexpr char kBraveRewardsGeminiDescription[] =
    "Enables support for Gemini as an external wallet provider for Brave";
#endif

constexpr char kBraveRewardsVBatNoticeName[] =
    "Enable Brave Rewards VBAT notices";
constexpr char kBraveRewardsVBatNoticeDescription[] =
    "Enables notices in the Brave Rewards UI about VBAT deadlines.";

constexpr char kBraveRewardsVerboseLoggingName[] =
    "Enable Brave Rewards verbose logging";
constexpr char kBraveRewardsVerboseLoggingDescription[] =
    "Enables detailed logging of Brave Rewards system events to a log file "
    "stored on your device. Please note that this log file could include "
    "information such as browsing history and credentials such as passwords "
    "and access tokens depending on your activity. Please do not share it "
    "unless asked to by Brave staff.";

constexpr char kBraveRewardsAllowUnsupportedWalletProvidersName[] =
    "Always show Brave Rewards custodial connection options";
constexpr char kBraveRewardsAllowUnsupportedWalletProvidersDescription[] =
    "Allows all custodial options to be selected in Brave Rewards, "
    "including those not supported for your Rewards country.";

constexpr char kBraveSearchDefaultAPIName[] =
    "Enable Brave Search website default search provider API";
constexpr char kBraveSearchDefaultAPIDescription[] =
    "Enable javascript API only on "
    "Brave Search websites which will allow the user to make the search engine "
    "their default search provider.";

constexpr char kBraveSuperReferralName[] = "Enable Brave Super Referral";
constexpr char kBraveSuperReferralDescription[] =
    "Use custom theme for Brave Super Referral";

constexpr char kNativeBraveWalletName[] = "Enable Brave Wallet";
constexpr char kNativeBraveWalletDescription[] =
    "Native cryptocurrency wallet support without the use of extensions";

constexpr char kBraveWalletFilecoinName[] =
    "Enable Brave Wallet Filecoin support";
constexpr char kBraveWalletFilecoinDescription[] =
    "Filecoin support for native Brave Wallet";

constexpr char kBraveWalletSolanaName[] = "Enable Brave Wallet Solana support";
constexpr char kBraveWalletSolanaDescription[] =
    "Solana support for native Brave Wallet";
constexpr char kBraveWalletSolanaProviderName[] =
    "Enable Brave Wallet Solana provider support";
constexpr char kBraveWalletSolanaProviderDescription[] =
    "Solana provider support for native Brave Wallet";
constexpr char kBraveWalletSnsName[] = "Enable Solana Name Service support";
constexpr char kBraveWalletSnsDescription[] =
    "Enable Solana Name Service(.sol) support for Wallet and omnibox address "
    "resolution";
constexpr char kBraveWalletDappsSupportName[] =
    "Enable Brave Wallet Dapps support";
constexpr char kBraveWalletDappsSupportDescription[] =
    "Brave Wallet Dapps support";

constexpr char kBraveNewsName[] = "Enable Brave News";
constexpr char kBraveNewsDescription[] =
    "Brave News is completely private and includes anonymized ads matched on "
    "your device.";

constexpr char kBraveNewsV2Name[] = "Enable Brave News V2";
constexpr char kBraveNewsV2Description[] =
    "Use the new Brave News UI and sources lists";

constexpr char kBraveNewsCardPeekFeatureName[] =
    "Brave News prompts on New Tab Page";
constexpr char kBraveNewsCardPeekFeatureDescription[] =
    "Prompt Brave News via the top featured article peeking up from the bottom "
    "of the New Tab Page, after a short delay.";

constexpr char kCryptoWalletsForNewInstallsName[] =
    "Enable Crypto Wallets option in settings";
constexpr char kCryptoWalletsForNewInstallsDescription[] =
    "Crypto Wallets extension is deprecated but with this option it can still "
    "be enabled in settings. If it was previously used, this flag is ignored.";

constexpr char kUseDevUpdaterUrlName[] = "Use dev updater url";
constexpr char kUseDevUpdaterUrlDescription[] =
    "Use the dev url for the component updater. "
    "This is for internal testing only.";

constexpr char kBraveTranslateGoName[] =
    "Enable internal translate engine (brave-translate-go)";
constexpr char kBraveTranslateGoDescription[] =
    "For Android also enable `translate` flag. Enable internal translate "
    "engine, which are build on top of client engine "
    "and brave translation backed. Also disables suggestions to install google "
    "translate extension.";

constexpr char kTranslateName[] = "Enable Chromium Translate feature";
constexpr char kTranslateDescription[] =
    "Should be used with brave-translate-go, see the description here.";

constexpr char kBraveFederatedName[] =
    "Enables local data collection for notification ad timing "
    "(brave-federated)";
constexpr char kBraveFederatedDescription[] =
    "Starts local collection for notification ad timing data. This data "
    "is stored locally and automatically erased after one month. No data "
    "leaves the client.";

constexpr char kAllowIncognitoPermissionInheritanceName[] =
    "Allow permission inheritance in incognito profiles";
constexpr char kAllowIncognitoPermissionInheritanceDescription[] =
    "When enabled, most permissions set in a normal profile will be inherited "
    "in incognito profile if they are less permissive, for ex. Geolocation "
    "BLOCK will be automatically set to BLOCK in incognito.";

constexpr char kBraveSyncHistoryDiagnosticsName[] =
    "Enable Brave Sync History Diagnostics";
constexpr char kBraveSyncHistoryDiagnosticsDescription[] =
    "Brave Sync History Diagnostics flag displays additional sync related "
    "information on History page";

// Blink features.
constexpr char kFileSystemAccessAPIName[] = "File System Access API";
constexpr char kFileSystemAccessAPIDescription[] =
    "Enables the File System Access API, giving websites access to the file "
    "system";

constexpr char kNavigatorConnectionAttributeName[] =
    "Enable navigator.connection attribute";
constexpr char kNavigatorConnectionAttributeDescription[] =
    "Enables the navigator.connection API. Enabling this API will allow sites "
    "to learn information about your network and internet connection. Trackers "
    "can use this information to fingerprint your browser, or to infer when "
    "you are traveling or at home.";

constexpr char kRestrictWebSocketsPoolName[] = "Restrict WebSockets pool";
constexpr char kRestrictWebSocketsPoolDescription[] =
    "Limits simultaneous active WebSockets connections per eTLD+1";

constexpr char kPlaylistName[] = "Playlist";
constexpr char kPlaylistDescription[] = "Enables Playlist";

constexpr char kAllowCertainClientHintsName[] =
    "Allow certain request client hints";
constexpr char kAllowCertainClientHintsDescription[] =
    "Allows setting certain request client hints (sec-ch-ua, sec-ch-ua-mobile, "
    "sec-ch-ua-platform)";

#if defined(TOOLKIT_VIEWS)
constexpr char kBraveVerticalTabsName[] = "Vertical tabs";
constexpr char kBraveVerticalTabsDescription[] =
    "Move tab strip to be a vertical panel on the side of the window instead "
    "of horizontal at the top of the window.";
#endif  // defined(TOOLKIT_VIEWS)

#if BUILDFLAG(IS_ANDROID)
constexpr char kBraveBackgroundVideoPlaybackName[] =
    "Background video playback";
constexpr char kBraveBackgroundVideoPlaybackDescription[] =
    "Enables play audio from video in background when tab is not active or "
    "device screen is turned off. Try to switch to desktop mode if this "
    "feature is not working.";
constexpr char kBraveAndroidSafeBrowsingName[] = "Safe Browsing";
constexpr char kBraveAndroidSafeBrowsingDescription[] =
    "Enables Google Safe Browsing for determining whether a URL has been "
    "marked as a known threat.";
#endif

#if BUILDFLAG(IS_LINUX)
constexpr char kBraveChangeActiveTabOnScrollEventName[] =
    "Change active tab on scroll event";
constexpr char kBraveChangeActiveTabOnScrollEventDescription[] =
    "Change the active tab when scroll events occur on tab strip.";
#endif  // BUILDFLAG(IS_LINUX)

constexpr char kRestrictEventSourcePoolName[] = "Restrict Event Source Pool";
constexpr char kRestrictEventSourcePoolDescription[] =
    "Limits simultaneous active WebSockets connections per eTLD+1";

}  // namespace

}  // namespace flag_descriptions

// clang-format seems to have a lot of issues with the macros in this
// file so we turn it off for the macro sections.
// clang-format off

#if BUILDFLAG(ENABLE_BRAVE_VPN)
#define BRAVE_VPN_FEATURE_ENTRIES                         \
    {kBraveVPNFeatureInternalName,                        \
     flag_descriptions::kBraveVPNName,                    \
     flag_descriptions::kBraveVPNDescription,             \
     kOsMac | kOsWin,                                     \
     FEATURE_VALUE_TYPE(brave_vpn::features::kBraveVPN)},
#if BUILDFLAG(IS_WIN)
#define BRAVE_VPN_DNS_FEATURE_ENTRIES                                  \
    {kBraveVPNDnsFeatureInternalName,                                  \
     flag_descriptions::kBraveVPNDnsProtectionName,                    \
     flag_descriptions::kBraveVPNDnsProtectionDescription,             \
     kOsWin,                                                           \
     FEATURE_VALUE_TYPE(brave_vpn::features::kBraveVPNDnsProtection)},
#else
#define BRAVE_VPN_DNS_FEATURE_ENTRIES
#endif
#else
#define BRAVE_VPN_FEATURE_ENTRIES
#define BRAVE_VPN_DNS_FEATURE_ENTRIES
#endif

#define BRAVE_SKU_SDK_FEATURE_ENTRIES                   \
    {"skus-sdk",                                        \
     flag_descriptions::kBraveSkusSdkName,              \
     flag_descriptions::kBraveSkusSdkDescription,       \
     kOsMac | kOsWin | kOsAndroid,                      \
     FEATURE_VALUE_TYPE(skus::features::kSkusFeature)}, \

#if BUILDFLAG(ENABLE_SPEEDREADER)
#define SPEEDREADER_FEATURE_ENTRIES \
    {"brave-speedreader",                                               \
     flag_descriptions::kBraveSpeedreaderName,                          \
     flag_descriptions::kBraveSpeedreaderDescription, kOsDesktop,       \
     FEATURE_VALUE_TYPE(speedreader::kSpeedreaderFeature)},
#else
#define SPEEDREADER_FEATURE_ENTRIES
#endif

#if BUILDFLAG(ENABLE_GEMINI_WALLET)
#define BRAVE_REWARDS_GEMINI_FEATURE_ENTRIES                        \
    {"brave-rewards-gemini",                                        \
     flag_descriptions::kBraveRewardsGeminiName,                    \
     flag_descriptions::kBraveRewardsGeminiDescription, kOsDesktop, \
     FEATURE_VALUE_TYPE(brave_rewards::features::kGeminiFeature)},
#else
#define BRAVE_REWARDS_GEMINI_FEATURE_ENTRIES
#endif

#if BUILDFLAG(ENABLE_IPFS)
#define BRAVE_IPFS_FEATURE_ENTRIES                      \
    {"brave-ipfs",                                      \
     flag_descriptions::kBraveIpfsName,                 \
     flag_descriptions::kBraveIpfsDescription,          \
     kOsDesktop | kOsAndroid,                           \
     FEATURE_VALUE_TYPE(ipfs::features::kIpfsFeature)},
#else
#define BRAVE_IPFS_FEATURE_ENTRIES
#endif

#define BRAVE_NATIVE_WALLET_FEATURE_ENTRIES                                    \
    {"native-brave-wallet",                                                    \
     flag_descriptions::kNativeBraveWalletName,                                \
     flag_descriptions::kNativeBraveWalletDescription,                         \
     kOsDesktop | kOsAndroid,                                                  \
     FEATURE_VALUE_TYPE(brave_wallet::features::kNativeBraveWalletFeature)},   \
    {"brave-wallet-filecoin",                                                  \
     flag_descriptions::kBraveWalletFilecoinName,                              \
     flag_descriptions::kBraveWalletFilecoinDescription,                       \
     kOsDesktop | kOsAndroid,                                                  \
     FEATURE_VALUE_TYPE(brave_wallet::features::kBraveWalletFilecoinFeature)}, \
    {"brave-wallet-solana",                                                    \
     flag_descriptions::kBraveWalletSolanaName,                                \
     flag_descriptions::kBraveWalletSolanaDescription,                         \
     kOsDesktop | kOsAndroid,                                                  \
     FEATURE_VALUE_TYPE(brave_wallet::features::kBraveWalletSolanaFeature)},   \
    {"brave-wallet-solana-provider",                                           \
     flag_descriptions::kBraveWalletSolanaProviderName,                        \
     flag_descriptions::kBraveWalletSolanaProviderDescription,                 \
     kOsDesktop | kOsAndroid,                                                  \
     FEATURE_VALUE_TYPE(                                                       \
      brave_wallet::features::kBraveWalletSolanaProviderFeature)},             \
    {"brave-wallet-sns",                                                       \
     flag_descriptions::kBraveWalletSnsName,                                   \
     flag_descriptions::kBraveWalletSnsDescription,                            \
     kOsDesktop | kOsAndroid,                                                  \
     FEATURE_VALUE_TYPE(brave_wallet::features::kBraveWalletSnsFeature)},      \
    {"brave-wallet-dapps-support",                                             \
     flag_descriptions::kBraveWalletDappsSupportName,                          \
     flag_descriptions::kBraveWalletDappsSupportDescription,                   \
     kOsDesktop | kOsAndroid,                                                  \
     FEATURE_VALUE_TYPE(                                                       \
      brave_wallet::features::kBraveWalletDappsSupportFeature)},

#define BRAVE_NEWS_FEATURE_ENTRIES                                          \
    {"brave-news",                                                          \
     flag_descriptions::kBraveNewsName,                                     \
     flag_descriptions::kBraveNewsDescription,                              \
     kOsDesktop | kOsAndroid,                                               \
     FEATURE_VALUE_TYPE(brave_today::features::kBraveNewsFeature)},         \
    {"brave-news-v2",                                                       \
     flag_descriptions::kBraveNewsV2Name,                                   \
     flag_descriptions::kBraveNewsV2Description,                            \
     kOsDesktop | flags_ui::kOsAndroid,                                     \
     FEATURE_VALUE_TYPE(brave_today::features::kBraveNewsV2Feature)},       \
    {"brave-news-peek",                                                     \
     flag_descriptions::kBraveNewsCardPeekFeatureName,                      \
     flag_descriptions::kBraveNewsCardPeekFeatureDescription,               \
     kOsDesktop,                                                            \
     FEATURE_VALUE_TYPE(brave_today::features::kBraveNewsCardPeekFeature)}, \

#define BRAVE_FEDERATED_FEATURE_ENTRIES                                 \
    {"brave-federated",                                                 \
     flag_descriptions::kBraveFederatedName,                            \
     flag_descriptions::kBraveFederatedDescription,                     \
     kOsDesktop,                                                        \
     FEATURE_VALUE_TYPE(brave_federated::features::kFederatedLearning)},

#if BUILDFLAG(ETHEREUM_REMOTE_CLIENT_ENABLED)
#define CRYPTO_WALLETS_FEATURE_ENTRIES                                       \
    {"ethereum_remote-client_new-installs",                                  \
     flag_descriptions::kCryptoWalletsForNewInstallsName,                    \
     flag_descriptions::kCryptoWalletsForNewInstallsDescription,             \
     kOsDesktop,                                                             \
     FEATURE_VALUE_TYPE(                                                     \
       ethereum_remote_client::features::kCryptoWalletsForNewInstallsFeature)},
#else
#define CRYPTO_WALLETS_FEATURE_ENTRIES
#endif

#if BUILDFLAG(ENABLE_PLAYLIST)
#define PLAYLIST_FEATURE_ENTRIES                                           \
     {kPlaylistFeatureInternalName,                                        \
     flag_descriptions::kPlaylistName,                                     \
     flag_descriptions::kPlaylistDescription,                              \
     kOsMac | kOsWin | kOsLinux | kOsAndroid,                              \
     FEATURE_VALUE_TYPE(playlist::features::kPlaylist)},
#else
#define PLAYLIST_FEATURE_ENTRIES
#endif

#if defined(TOOLKIT_VIEWS)
#define BRAVE_VERTICAL_TABS_FEATURE_ENTRY                   \
    {"brave-vertical-tabs",                                 \
     flag_descriptions::kBraveVerticalTabsName,             \
     flag_descriptions::kBraveVerticalTabsDescription,      \
     kOsWin | kOsMac | kOsLinux,                            \
     FEATURE_VALUE_TYPE(tabs::features::kBraveVerticalTabs)},
#else
#define BRAVE_VERTICAL_TABS_FEATURE_ENTRY
#endif  // defined(TOOLKIT_VIEWS)

#if BUILDFLAG(IS_LINUX)
#define BRAVE_CHANGE_ACTIVE_TAB_ON_SCROLL_EVENT_FEATURE_ENTRIES              \
    {"brave-change-active-tab-on-scroll-event",                              \
     flag_descriptions::kBraveChangeActiveTabOnScrollEventName,              \
     flag_descriptions::kBraveChangeActiveTabOnScrollEventDescription,       \
     kOsLinux,                                                               \
     FEATURE_VALUE_TYPE(tabs::features::kBraveChangeActiveTabOnScrollEvent)},
#else
#define BRAVE_CHANGE_ACTIVE_TAB_ON_SCROLL_EVENT_FEATURE_ENTRIES
#endif

#if BUILDFLAG(IS_ANDROID)
#define BRAVE_BACKGROUND_VIDEO_PLAYBACK_ANDROID                   \
    {"brave-background-video-playback",                           \
     flag_descriptions::kBraveBackgroundVideoPlaybackName,        \
     flag_descriptions::kBraveBackgroundVideoPlaybackDescription, \
     kOsAndroid,                                                  \
     FEATURE_VALUE_TYPE(kBraveBackgroundVideoPlayback)},
#define BRAVE_SAFE_BROWSING_ANDROID                               \
    {"brave-safe-browsing",                                       \
     flag_descriptions::kBraveAndroidSafeBrowsingName,            \
     flag_descriptions::kBraveAndroidSafeBrowsingDescription,     \
     kOsAndroid,                                                  \
     FEATURE_VALUE_TYPE(kBraveAndroidSafeBrowsing)},
#else
#define BRAVE_BACKGROUND_VIDEO_PLAYBACK_ANDROID
#define BRAVE_SAFE_BROWSING_ANDROID
#endif  // BUILDFLAG(IS_ANDROID)

#define BRAVE_ABOUT_FLAGS_FEATURE_ENTRIES                                   \
    {"use-dev-updater-url",                                                 \
     flag_descriptions::kUseDevUpdaterUrlName,                              \
     flag_descriptions::kUseDevUpdaterUrlDescription, kOsAll,               \
     FEATURE_VALUE_TYPE(brave_component_updater::kUseDevUpdaterUrl)},       \
    {"allow-certain-client-hints",                                          \
      flag_descriptions::kAllowCertainClientHintsName,                      \
      flag_descriptions::kAllowCertainClientHintsDescription, kOsAll,       \
      FEATURE_VALUE_TYPE(blink::features::kAllowCertainClientHints)},       \
    {"brave-ntp-branded-wallpaper-demo",                                    \
     flag_descriptions::kBraveNTPBrandedWallpaperDemoName,                  \
     flag_descriptions::kBraveNTPBrandedWallpaperDemoDescription, kOsAll,   \
     FEATURE_VALUE_TYPE(kBraveNTPBrandedWallpaperDemo)},                    \
    {"brave-adblock-cname-uncloaking",                                      \
     flag_descriptions::kBraveAdblockCnameUncloakingName,                   \
     flag_descriptions::kBraveAdblockCnameUncloakingDescription, kOsAll,    \
     FEATURE_VALUE_TYPE(kBraveAdblockCnameUncloaking)},                     \
    {"brave-adblock-collapse-blocked-elements",                             \
     flag_descriptions::kBraveAdblockCollapseBlockedElementsName,           \
     flag_descriptions::kBraveAdblockCollapseBlockedElementsDescription,    \
     kOsAll, FEATURE_VALUE_TYPE(kBraveAdblockCollapseBlockedElements)},     \
    {"brave-adblock-cookie-list-default",                                   \
     flag_descriptions::kBraveAdblockCookieListDefaultName,                 \
     flag_descriptions::kBraveAdblockCookieListDefaultDescription,          \
     kOsAll, FEATURE_VALUE_TYPE(kBraveAdblockCookieListDefault)},           \
    {"brave-adblock-cookie-list-opt-in",                                    \
     flag_descriptions::kBraveAdblockCookieListOptInName,                   \
     flag_descriptions::kBraveAdblockCookieListOptInDescription,            \
     kOsDesktop | kOsAndroid,                                               \
     FEATURE_VALUE_TYPE(kBraveAdblockCookieListOptIn)},                     \
    {"brave-adblock-cosmetic-filtering",                                    \
     flag_descriptions::kBraveAdblockCosmeticFilteringName,                 \
     flag_descriptions::kBraveAdblockCosmeticFilteringDescription, kOsAll,  \
     FEATURE_VALUE_TYPE(kBraveAdblockCosmeticFiltering)},                   \
    {"brave-adblock-csp-rules",                                             \
     flag_descriptions::kBraveAdblockCspRulesName,                          \
     flag_descriptions::kBraveAdblockCspRulesDescription, kOsAll,           \
     FEATURE_VALUE_TYPE(kBraveAdblockCspRules)},                            \
    {"brave-adblock-default-1p-blocking",                                   \
     flag_descriptions::kBraveAdblockDefault1pBlockingName,                 \
     flag_descriptions::kBraveAdblockDefault1pBlockingDescription, kOsAll,  \
     FEATURE_VALUE_TYPE(kBraveAdblockDefault1pBlocking)},                   \
    {"brave-adblock-mobile-notifications-list-default",                         \
     flag_descriptions::kBraveAdblockMobileNotificationsListDefaultName,        \
     flag_descriptions::kBraveAdblockMobileNotificationsListDefaultDescription, \
     kOsAll, FEATURE_VALUE_TYPE(kBraveAdblockMobileNotificationsListDefault)},  \
    {"brave-dark-mode-block",                                               \
     flag_descriptions::kBraveDarkModeBlockName,                            \
     flag_descriptions::kBraveDarkModeBlockDescription, kOsAll,             \
     FEATURE_VALUE_TYPE(kBraveDarkModeBlock)},                              \
    {"brave-domain-block",                                                  \
     flag_descriptions::kBraveDomainBlockName,                              \
     flag_descriptions::kBraveDomainBlockDescription, kOsAll,               \
     FEATURE_VALUE_TYPE(kBraveDomainBlock)},                                \
    {"brave-domain-block-1pes",                                             \
     flag_descriptions::kBraveDomainBlock1PESName,                          \
     flag_descriptions::kBraveDomainBlock1PESDescription, kOsAll,           \
     FEATURE_VALUE_TYPE(kBraveDomainBlock1PES)},                            \
    {"brave-debounce",                                                      \
        flag_descriptions::kBraveDebounceName,                              \
        flag_descriptions::kBraveDebounceDescription, kOsAll,               \
        FEATURE_VALUE_TYPE(kBraveDebounce)},                                \
    {"brave-de-amp",                                                        \
        flag_descriptions::kBraveDeAMPName,                                 \
        flag_descriptions::kBraveDeAMPDescription, kOsAll,                  \
        FEATURE_VALUE_TYPE(kBraveDeAMP)},                                   \
    {"brave-google-sign-in-permission",                                     \
        flag_descriptions::kBraveGoogleSignInPermissionName,                \
        flag_descriptions::kBraveGoogleSignInPermissionDescription, kOsAll, \
        FEATURE_VALUE_TYPE(kBraveGoogleSignInPermission)},                  \
    {"brave-extension-network-blocking",                                    \
     flag_descriptions::kBraveExtensionNetworkBlockingName,                 \
     flag_descriptions::kBraveExtensionNetworkBlockingDescription, kOsAll,  \
     FEATURE_VALUE_TYPE(kBraveExtensionNetworkBlocking)},                   \
    {"brave-reduce-language",                                               \
        flag_descriptions::kBraveReduceLanguageName,                        \
        flag_descriptions::kBraveReduceLanguageDescription, kOsAll,         \
        FEATURE_VALUE_TYPE(kBraveReduceLanguage)},                          \
    {"brave-cosmetic-filtering-sync-load",                                  \
     flag_descriptions::kCosmeticFilteringSyncLoadName,                     \
     flag_descriptions::kCosmeticFilteringSyncLoadDescription, kOsAll,      \
     FEATURE_VALUE_TYPE(kCosmeticFilteringSyncLoad)},                       \
    {"brave-super-referral",                                                \
     flag_descriptions::kBraveSuperReferralName,                            \
     flag_descriptions::kBraveSuperReferralDescription,                     \
     flags_ui::kOsMac | flags_ui::kOsWin | flags_ui::kOsAndroid,            \
     FEATURE_VALUE_TYPE(kBraveNTPSuperReferralWallpaper)},                  \
    {"brave-ephemeral-storage",                                             \
     flag_descriptions::kBraveEphemeralStorageName,                         \
     flag_descriptions::kBraveEphemeralStorageDescription, kOsAll,          \
     FEATURE_VALUE_TYPE(net::features::kBraveEphemeralStorage)},            \
    {"brave-ephemeral-storage-keep-alive",                                  \
     flag_descriptions::kBraveEphemeralStorageKeepAliveName,                \
     flag_descriptions::kBraveEphemeralStorageKeepAliveDescription, kOsAll, \
     FEATURE_VALUE_TYPE(net::features::kBraveEphemeralStorageKeepAlive)},   \
    {"brave-first-party-ephemeral-storage",                                 \
     flag_descriptions::kBraveFirstPartyEphemeralStorageName,               \
     flag_descriptions::kBraveFirstPartyEphemeralStorageDescription,        \
     kOsAll,                                                                \
     FEATURE_VALUE_TYPE(net::features::kBraveFirstPartyEphemeralStorage)},  \
    {"brave-rewards-vbat-notice",                                           \
     flag_descriptions::kBraveRewardsVBatNoticeName,                        \
     flag_descriptions::kBraveRewardsVBatNoticeDescription,                 \
     kOsDesktop | kOsAndroid,                                               \
     FEATURE_VALUE_TYPE(brave_rewards::features::kVBatNoticeFeature)},      \
    {"brave-rewards-verbose-logging",                                       \
     flag_descriptions::kBraveRewardsVerboseLoggingName,                    \
     flag_descriptions::kBraveRewardsVerboseLoggingDescription,             \
     kOsDesktop | kOsAndroid,                                               \
     FEATURE_VALUE_TYPE(brave_rewards::features::kVerboseLoggingFeature)},  \
    {"brave-rewards-allow-unsupported-wallet-providers",                    \
     flag_descriptions::kBraveRewardsAllowUnsupportedWalletProvidersName,   \
     flag_descriptions::kBraveRewardsAllowUnsupportedWalletProvidersDescription,\
     kOsDesktop | kOsAndroid,                                               \
     FEATURE_VALUE_TYPE(                                                    \
       brave_rewards::features::kAllowUnsupportedWalletProvidersFeature)},  \
    {"brave-ads-custom-push-notifications-ads",                             \
     flag_descriptions::kBraveAdsCustomNotificationsName,                   \
     flag_descriptions::kBraveAdsCustomNotificationsDescription,            \
     kOsAll,                                                                \
     FEATURE_VALUE_TYPE(brave_ads::features::kCustomNotificationAds)},      \
    {"brave-ads-allowed-to-fallback-to-custom-push-notification-ads",       \
     flag_descriptions::kBraveAdsCustomNotificationsFallbackName,           \
     flag_descriptions::kBraveAdsCustomNotificationsFallbackDescription,    \
     kOsAll,                                                                \
     FEATURE_VALUE_TYPE(                                                    \
       brave_ads::features::kAllowedToFallbackToCustomNotificationAds)},    \
    {"brave-sync-v2",                                                       \
      flag_descriptions::kBraveSyncName,                                    \
      flag_descriptions::kBraveSyncDescription, kOsDesktop,                 \
      FEATURE_VALUE_TYPE(brave_sync::features::kBraveSync)},                \
    {"file-system-access-api",                                              \
      flag_descriptions::kFileSystemAccessAPIName,                          \
      flag_descriptions::kFileSystemAccessAPIDescription, kOsDesktop,       \
      FEATURE_VALUE_TYPE(blink::features::kFileSystemAccessAPI)},           \
    {"navigator-connection-attribute",                                      \
      flag_descriptions::kNavigatorConnectionAttributeName,                 \
      flag_descriptions::kNavigatorConnectionAttributeDescription, kOsAll,  \
      FEATURE_VALUE_TYPE(blink::features::kNavigatorConnectionAttribute)},  \
    {"restrict-websockets-pool",                                            \
      flag_descriptions::kRestrictWebSocketsPoolName,                       \
      flag_descriptions::kRestrictWebSocketsPoolDescription, kOsAll,        \
      FEATURE_VALUE_TYPE(blink::features::kRestrictWebSocketsPool)},        \
    {"allow-incognito-permission-inheritance",                              \
      flag_descriptions::kAllowIncognitoPermissionInheritanceName,          \
      flag_descriptions::kAllowIncognitoPermissionInheritanceDescription,   \
      kOsAll, FEATURE_VALUE_TYPE(                                           \
          content_settings::kAllowIncognitoPermissionInheritance)},         \
    {"brave-block-screen-fingerprinting",                                   \
      flag_descriptions::kBraveBlockScreenFingerprintingName,               \
      flag_descriptions::kBraveBlockScreenFingerprintingDescription,        \
      kOsAll, FEATURE_VALUE_TYPE(                                           \
          blink::features::kBraveBlockScreenFingerprinting)},               \
    {"brave-tor-windows-https-only",                                        \
      flag_descriptions::kBraveTorWindowsHttpsOnlyName,                     \
      flag_descriptions::kBraveTorWindowsHttpsOnlyDescription,              \
      kOsAll, FEATURE_VALUE_TYPE(                                           \
          blink::features::kBraveTorWindowsHttpsOnly)},                     \
    {"brave-round-time-stamps",                                             \
      flag_descriptions::kBraveRoundTimeStampsName,                         \
      flag_descriptions::kBraveRoundTimeStampsDescription,                  \
      kOsAll, FEATURE_VALUE_TYPE(                                           \
          blink::features::kBraveRoundTimeStamps)},                         \
    {"translate",                                                           \
      flag_descriptions::kTranslateName,                                    \
      flag_descriptions::kTranslateDescription,                             \
      kOsDesktop | kOsAndroid,                                              \
      FEATURE_VALUE_TYPE(translate::kTranslate)},                           \
    {"brave-sync-history-diagnostics",                                      \
      flag_descriptions::kBraveSyncHistoryDiagnosticsName,                  \
      flag_descriptions::kBraveSyncHistoryDiagnosticsDescription,           \
      kOsAll, FEATURE_VALUE_TYPE(                                           \
          brave_sync::features::kBraveSyncHistoryDiagnostics)},             \
    {"restrict-event-source-pool",                                          \
      flag_descriptions::kRestrictEventSourcePoolName,                      \
      flag_descriptions::kRestrictEventSourcePoolDescription,               \
      kOsAll, FEATURE_VALUE_TYPE(                                           \
          blink::features::kRestrictEventSourcePool)},                      \
    BRAVE_IPFS_FEATURE_ENTRIES                                              \
    BRAVE_NATIVE_WALLET_FEATURE_ENTRIES                                     \
    BRAVE_NEWS_FEATURE_ENTRIES                                              \
    CRYPTO_WALLETS_FEATURE_ENTRIES                                          \
    BRAVE_REWARDS_GEMINI_FEATURE_ENTRIES                                    \
    BRAVE_VPN_FEATURE_ENTRIES                                               \
    BRAVE_VPN_DNS_FEATURE_ENTRIES                                           \
    BRAVE_SKU_SDK_FEATURE_ENTRIES                                           \
    SPEEDREADER_FEATURE_ENTRIES                                             \
    BRAVE_FEDERATED_FEATURE_ENTRIES                                         \
    PLAYLIST_FEATURE_ENTRIES                                                \
    BRAVE_VERTICAL_TABS_FEATURE_ENTRY                                       \
    BRAVE_BACKGROUND_VIDEO_PLAYBACK_ANDROID                                 \
    BRAVE_SAFE_BROWSING_ANDROID                                             \
    BRAVE_CHANGE_ACTIVE_TAB_ON_SCROLL_EVENT_FEATURE_ENTRIES
