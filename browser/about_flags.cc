/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/about_flags.h"

#include "base/strings/string_util.h"
#include "brave/browser/brave_features_internal_names.h"
#include "brave/browser/ethereum_remote_client/buildflags/buildflags.h"
#include "brave/browser/ethereum_remote_client/features.h"
#include "brave/common/brave_features.h"
#include "brave/components/brave_ads/common/features.h"
#include "brave/components/brave_component_updater/browser/features.h"
#include "brave/components/brave_rewards/common/buildflags/buildflags.h"
#include "brave/components/brave_rewards/common/features.h"
#include "brave/components/brave_shields/common/features.h"
#include "brave/components/brave_sync/features.h"
#include "brave/components/brave_today/common/features.h"
#include "brave/components/brave_vpn/buildflags/buildflags.h"
#include "brave/components/brave_wallet/common/features.h"
#include "brave/components/debounce/common/features.h"
#include "brave/components/decentralized_dns/buildflags/buildflags.h"
#include "brave/components/ipfs/buildflags/buildflags.h"
#include "brave/components/ntp_background_images/browser/features.h"
#include "brave/components/sidebar/buildflags/buildflags.h"
#include "brave/components/speedreader/buildflags.h"
#include "brave/components/translate/core/common/brave_translate_features.h"
#include "brave/components/translate/core/common/buildflags.h"
#include "net/base/features.h"
#include "third_party/blink/public/common/features.h"

#if BUILDFLAG(ENABLE_BRAVE_VPN) && !defined(OS_ANDROID)
#include "brave/components/brave_vpn/features.h"
#endif

#if BUILDFLAG(ENABLE_SIDEBAR)
#include "brave/components/sidebar/features.h"
#endif

#if BUILDFLAG(ENABLE_SPEEDREADER)
#include "brave/components/speedreader/features.h"
#endif

#if BUILDFLAG(ENABLE_IPFS)
#include "brave/components/ipfs/features.h"
#endif


#if BUILDFLAG(DECENTRALIZED_DNS_ENABLED)
#include "brave/components/decentralized_dns/features.h"
#endif

using brave_shields::features::kBraveAdblockCnameUncloaking;
using brave_shields::features::kBraveAdblockCollapseBlockedElements;
using brave_shields::features::kBraveAdblockCookieListDefault;
using brave_shields::features::kBraveAdblockCosmeticFiltering;
using brave_shields::features::kBraveAdblockCspRules;
using brave_shields::features::kBraveAdblockDefault1pBlocking;
using brave_shields::features::kBraveDarkModeBlock;
using brave_shields::features::kBraveDomainBlock;
using brave_shields::features::kBraveDomainBlock1PES;
using brave_shields::features::kBraveExtensionNetworkBlocking;
using brave_shields::features::kCosmeticFilteringSyncLoad;

using debounce::features::kBraveDebounce;
using ntp_background_images::features::kBraveNTPBrandedWallpaperDemo;
using ntp_background_images::features::kBraveNTPSuperReferralWallpaper;

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
    "Enables the 'Easylist-Cookie List' regional list regardless of its "
    "toggle setting in brave://adblock";

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

constexpr char kAdblockRedirectUrlName[] =
    "Enable support for $redirect-url filter option for adblock rules";
constexpr char kAdblockRedirectUrlDescription[] =
    "Enable support for loading adblock replacement resources over the network "
    "via the $redirect-url filter option";

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

constexpr char kBraveExtensionNetworkBlockingName[] =
    "Enable extension network blocking";
constexpr char kBraveExtensionNetworkBlockingDescription[] =
    "Enable blocking for network requests initiated by extensions";

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

constexpr char kBraveShieldsV2Name[] = "Enable Brave Shields v2";
constexpr char kBraveShieldsV2Description[] =
    "Major UX/UI overhaul of Brave Shields";

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

constexpr char kNativeBraveWalletName[] = "Enable Brave Wallet";
constexpr char kNativeBraveWalletDescription[] =
    "Native cryptocurrency wallet support without the use of extensions";

constexpr char kBraveWalletFilecoinName[] =
    "Enable Brave Wallet Filecoin support";
constexpr char kBraveWalletFilecoinDescription[] =
    "Filecoin support for native Brave Wallet";

constexpr char kBraveNewsName[] = "Enable Brave News";
constexpr char kBraveNewsDescription[] =
    "Brave News is completely private and includes anonymized ads matched on "
    "your device.";

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
    "Enable internal translate engine, which are build on top of client engine "
    "and brave translation backed. Also disables suggestions to install google "
    "translate extension.";

constexpr char kTabAudioIconInteractiveName[] =
    "Interactive Tab audio indicator";
constexpr char kTabAudioIconInteractiveDescription[] =
    "Enable the Tab audio indicator to also be a button which can mute and "
    "unmute the Tab.";

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

}  // namespace

}  // namespace flag_descriptions

// clang-format seems to have a lot of issues with the macros in this
// file so we turn it off for the macro sections.
// clang-format off

#if BUILDFLAG(ENABLE_BRAVE_VPN) && !defined(OS_ANDROID)
#define BRAVE_VPN_FEATURE_ENTRIES                         \
    {kBraveVPNFeatureInternalName,                        \
     flag_descriptions::kBraveVPNName,                    \
     flag_descriptions::kBraveVPNDescription,             \
     kOsMac | kOsWin,                                     \
     FEATURE_VALUE_TYPE(brave_vpn::features::kBraveVPN)},
#else
#define BRAVE_VPN_FEATURE_ENTRIES
#endif

#if BUILDFLAG(ENABLE_SIDEBAR)
#define SIDEBAR_FEATURE_ENTRIES                     \
    {kBraveSidebarFeatureInternalName,              \
     flag_descriptions::kBraveSidebarName,          \
     flag_descriptions::kBraveSidebarDescription,   \
     kOsMac | kOsWin | kOsLinux,                    \
     FEATURE_VALUE_TYPE(sidebar::kSidebarFeature)},
#else
#define SIDEBAR_FEATURE_ENTRIES
#endif

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

#define BRAVE_NATIVE_WALLET_FEATURE_ENTRIES                                  \
    {"native-brave-wallet",                                                  \
     flag_descriptions::kNativeBraveWalletName,                              \
     flag_descriptions::kNativeBraveWalletDescription,                       \
     kOsDesktop | flags_ui::kOsAndroid,                                      \
     FEATURE_VALUE_TYPE(brave_wallet::features::kNativeBraveWalletFeature)}, \
    {"brave-wallet-filecoin",                                                \
     flag_descriptions::kBraveWalletFilecoinName,                            \
     flag_descriptions::kBraveWalletFilecoinDescription,                     \
     kOsDesktop | flags_ui::kOsAndroid,                                      \
     FEATURE_VALUE_TYPE(brave_wallet::features::kBraveWalletFilecoinFeature)},

#define BRAVE_NEWS_FEATURE_ENTRIES                                  \
    {"brave-news",                                                  \
     flag_descriptions::kBraveNewsName,                             \
     flag_descriptions::kBraveNewsDescription,                      \
     kOsDesktop | flags_ui::kOsAndroid,                             \
     FEATURE_VALUE_TYPE(brave_today::features::kBraveNewsFeature)},

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

#if BUILDFLAG(DECENTRALIZED_DNS_ENABLED)
#define BRAVE_DECENTRALIZED_DNS_FEATURE_ENTRIES                           \
    {"brave-decentralized-dns",                                           \
     flag_descriptions::kBraveDecentralizedDnsName,                       \
     flag_descriptions::kBraveDecentralizedDnsDescription,                \
     kOsDesktop | kOsAndroid,                                             \
     FEATURE_VALUE_TYPE(decentralized_dns::features::kDecentralizedDns)},
#else
#define BRAVE_DECENTRALIZED_DNS_FEATURE_ENTRIES
#endif

#if BUILDFLAG(ENABLE_BRAVE_TRANSLATE_GO)
#define BRAVE_TRANSLATE_GO_FEATURE_ENTRIES                           \
    {"brave-translate-go",                                           \
     flag_descriptions::kBraveTranslateGoName,                       \
     flag_descriptions::kBraveTranslateGoDescription,                \
     kOsDesktop,                                                     \
     FEATURE_VALUE_TYPE(translate::features::kUseBraveTranslateGo)},
#else
#define BRAVE_TRANSLATE_GO_FEATURE_ENTRIES
#endif  // BUILDFLAG(ENABLE_BRAVE_TRANSLATE_GO)

#if !defined(OS_ANDROID)
#define BRAVE_SHIELDS_V2_FEATURE_ENTRIES                            \
    {"brave-shields-v2",                                            \
     flag_descriptions::kBraveShieldsV2Name,                        \
     flag_descriptions::kBraveShieldsV2Description,                 \
     kOsDesktop,                                                    \
     FEATURE_VALUE_TYPE(brave_shields::features::kBraveShieldsPanelV2)},
#else
#define BRAVE_SHIELDS_V2_FEATURE_ENTRIES
#endif

#define BRAVE_ABOUT_FLAGS_FEATURE_ENTRIES                                   \
    {"use-dev-updater-url",                                                 \
     flag_descriptions::kUseDevUpdaterUrlName,                              \
     flag_descriptions::kUseDevUpdaterUrlDescription, kOsAll,               \
     FEATURE_VALUE_TYPE(brave_component_updater::kUseDevUpdaterUrl)},       \
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
    {"brave-dark-mode-block",                                               \
     flag_descriptions::kBraveDarkModeBlockName,                            \
     flag_descriptions::kBraveDarkModeBlockDescription, kOsAll,             \
     FEATURE_VALUE_TYPE(kBraveDarkModeBlock)},                              \
    {"brave-adblock-redirect-url",                                          \
     flag_descriptions::kAdblockRedirectUrlName,                            \
     flag_descriptions::kAdblockRedirectUrlDescription, kOsAll,             \
     FEATURE_VALUE_TYPE(net::features::kAdblockRedirectUrl)},               \
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
    {"brave-extension-network-blocking",                                    \
     flag_descriptions::kBraveExtensionNetworkBlockingName,                 \
     flag_descriptions::kBraveExtensionNetworkBlockingDescription, kOsAll,  \
     FEATURE_VALUE_TYPE(kBraveExtensionNetworkBlocking)},                   \
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
    {"brave-rewards-verbose-logging",                                       \
     flag_descriptions::kBraveRewardsVerboseLoggingName,                    \
     flag_descriptions::kBraveRewardsVerboseLoggingDescription,             \
     kOsDesktop | kOsAndroid,                                               \
     FEATURE_VALUE_TYPE(brave_rewards::features::kVerboseLoggingFeature)},  \
    {"brave-ads-custom-push-notifications-ads",                             \
     flag_descriptions::kBraveAdsCustomNotificationsName,                   \
     flag_descriptions::kBraveAdsCustomNotificationsDescription,            \
     kOsAll,                                                                \
     FEATURE_VALUE_TYPE(brave_ads::features::kCustomAdNotifications)},      \
    {"brave-ads-allowed-to-fallback-to-custom-push-notification-ads",       \
     flag_descriptions::kBraveAdsCustomNotificationsFallbackName,           \
     flag_descriptions::kBraveAdsCustomNotificationsFallbackDescription,    \
     kOsAll,                                                                \
     FEATURE_VALUE_TYPE(                                                    \
       brave_ads::features::kAllowedToFallbackToCustomAdNotifications)},    \
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
    {"tab-audio-icon-interactive",                                          \
      flag_descriptions::kTabAudioIconInteractiveName,                      \
      flag_descriptions::kTabAudioIconInteractiveDescription,               \
      kOsDesktop,                                                           \
      FEATURE_VALUE_TYPE(features::kTabAudioIconInteractive)},              \
    BRAVE_DECENTRALIZED_DNS_FEATURE_ENTRIES                                 \
    BRAVE_IPFS_FEATURE_ENTRIES                                              \
    BRAVE_NATIVE_WALLET_FEATURE_ENTRIES                                     \
    BRAVE_NEWS_FEATURE_ENTRIES                                              \
    CRYPTO_WALLETS_FEATURE_ENTRIES                                          \
    BRAVE_REWARDS_GEMINI_FEATURE_ENTRIES                                    \
    BRAVE_VPN_FEATURE_ENTRIES                                               \
    SIDEBAR_FEATURE_ENTRIES                                                 \
    SPEEDREADER_FEATURE_ENTRIES                                             \
    BRAVE_SHIELDS_V2_FEATURE_ENTRIES                                        \
    BRAVE_TRANSLATE_GO_FEATURE_ENTRIES
