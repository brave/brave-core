/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

// This file is included into //chrome/browser/about_flags.cc.

#include <initializer_list>

#include "brave/browser/brave_browser_features.h"
#include "brave/browser/brave_features_internal_names.h"
#include "brave/browser/ethereum_remote_client/buildflags/buildflags.h"
#include "brave/browser/ethereum_remote_client/features.h"
#include "brave/browser/ui/tabs/features.h"
#include "brave/components/ai_chat/common/buildflags/buildflags.h"
#include "brave/components/brave_ads/browser/units/notification_ad/custom_notification_ad_feature.h"
#include "brave/components/brave_ads/core/public/ads_feature.h"
#include "brave/components/brave_ads/core/public/units/notification_ad/notification_ad_feature.h"
#include "brave/components/brave_component_updater/browser/features.h"
#include "brave/components/brave_federated/features.h"
#include "brave/components/brave_news/common/features.h"
#include "brave/components/brave_rewards/common/buildflags/buildflags.h"
#include "brave/components/brave_rewards/common/features.h"
#include "brave/components/brave_shields/common/features.h"
#include "brave/components/brave_sync/features.h"
#include "brave/components/brave_vpn/common/buildflags/buildflags.h"
#include "brave/components/brave_wallet/common/features.h"
#include "brave/components/de_amp/common/features.h"
#include "brave/components/debounce/common/features.h"
#include "brave/components/google_sign_in_permission/features.h"
#include "brave/components/ipfs/buildflags/buildflags.h"
#include "brave/components/ntp_background_images/browser/features.h"
#include "brave/components/playlist/common/buildflags/buildflags.h"
#include "brave/components/request_otr/common/buildflags/buildflags.h"
#include "brave/components/skus/common/features.h"
#include "brave/components/speedreader/common/buildflags/buildflags.h"
#include "build/build_config.h"
#include "chrome/browser/ui/ui_features.h"
#include "components/content_settings/core/common/features.h"
#include "components/flags_ui/feature_entry.h"
#include "components/flags_ui/feature_entry_macros.h"
#include "components/flags_ui/flags_state.h"
#include "components/omnibox/common/omnibox_features.h"
#include "components/translate/core/browser/translate_prefs.h"
#include "net/base/features.h"
#include "third_party/blink/public/common/features.h"

#if BUILDFLAG(ENABLE_AI_CHAT)
#include "brave/components/ai_chat/common/features.h"
#endif

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

#if BUILDFLAG(ENABLE_REQUEST_OTR)
#include "brave/components/request_otr/common/features.h"
#endif

#if BUILDFLAG(IS_ANDROID)
#include "brave/browser/android/preferences/features.h"
#include "brave/browser/android/safe_browsing/features.h"
#else
#include "brave/components/commands/common/features.h"
#endif

#if BUILDFLAG(IS_WIN)
#include "sandbox/policy/features.h"
#endif

#define EXPAND_FEATURE_ENTRIES(...) __VA_ARGS__,

#if BUILDFLAG(ENABLE_BRAVE_VPN)
#define BRAVE_VPN_FEATURE_ENTRIES                         \
  EXPAND_FEATURE_ENTRIES({                                \
      kBraveVPNFeatureInternalName,                       \
      "Enable experimental Brave VPN",                    \
      "Experimental native VPN support",                  \
      kOsMac | kOsWin,                                    \
      FEATURE_VALUE_TYPE(brave_vpn::features::kBraveVPN), \
  })
#if BUILDFLAG(IS_WIN)

#define BRAVE_VPN_WIREGUARD_FEATURE_ENTRIES                                  \
  EXPAND_FEATURE_ENTRIES({                                                   \
      kBraveVPNWireguardFeatureInternalName,                                 \
      "Enable experimental WireGuard Brave VPN service",                     \
      "Experimental WireGuard VPN support. Deprecated.",                     \
      kOsWin,                                                                \
      FEATURE_VALUE_TYPE(brave_vpn::features::kBraveVPNUseWireguardService), \
  })
#define BRAVE_VPN_DNS_FEATURE_ENTRIES                                    \
  EXPAND_FEATURE_ENTRIES({                                               \
      kBraveVPNDnsFeatureInternalName,                                   \
      "Enable DoH for Brave VPN",                                        \
      "Override DoH settings with Cloudflare dns if necessary to avoid " \
      "leaking requests due to Smart Multi-Home Named Resolution",       \
      kOsWin,                                                            \
      FEATURE_VALUE_TYPE(brave_vpn::features::kBraveVPNDnsProtection),   \
  })
#else
#define BRAVE_VPN_DNS_FEATURE_ENTRIES
#define BRAVE_VPN_WIREGUARD_FEATURE_ENTRIES
#endif
#else
#define BRAVE_VPN_FEATURE_ENTRIES
#define BRAVE_VPN_DNS_FEATURE_ENTRIES
#define BRAVE_VPN_WIREGUARD_FEATURE_ENTRIES
#endif

#define BRAVE_SKU_SDK_FEATURE_ENTRIES                   \
  EXPAND_FEATURE_ENTRIES({                              \
      "skus-sdk",                                       \
      "Enable experimental SKU SDK",                    \
      "Experimental SKU SDK support",                   \
      kOsMac | kOsWin | kOsAndroid,                     \
      FEATURE_VALUE_TYPE(skus::features::kSkusFeature), \
  })

#define SPEEDREADER_FEATURE_ENTRIES                                        \
  IF_BUILDFLAG(                                                            \
      ENABLE_SPEEDREADER,                                                  \
      EXPAND_FEATURE_ENTRIES({                                             \
          "brave-speedreader",                                             \
          "Enable SpeedReader",                                            \
          "Enables faster loading of simplified article-style web pages.", \
          kOsDesktop | kOsAndroid,                                         \
          FEATURE_VALUE_TYPE(speedreader::kSpeedreaderFeature),            \
      }))

#define REQUEST_OTR_FEATURE_ENTRIES                                           \
  IF_BUILDFLAG(                                                               \
      ENABLE_REQUEST_OTR,                                                     \
      EXPAND_FEATURE_ENTRIES({                                                \
          "brave-request-otr-tab",                                            \
          "Enable Request-OTR Tab",                                           \
          "Suggest going off-the-record when visiting potentially sensitive " \
          "URLs",                                                             \
          kOsDesktop | kOsAndroid,                                            \
          FEATURE_VALUE_TYPE(request_otr::features::kBraveRequestOTRTab),     \
      }))

#define BRAVE_MODULE_FILENAME_PATCH                                            \
  IF_BUILDFLAG(                                                                \
      IS_WIN,                                                                  \
      EXPAND_FEATURE_ENTRIES({                                                 \
          "brave-module-filename-patch",                                       \
          "Enable Module Filename patch",                                      \
          "Enables patching of executable's name from brave.exe to "           \
          "chrome.exe in sandboxed processes.",                                \
          kOsWin,                                                              \
          FEATURE_VALUE_TYPE(sandbox::policy::features::kModuleFileNamePatch), \
      }))

#define BRAVE_REWARDS_GEMINI_FEATURE_ENTRIES                               \
  IF_BUILDFLAG(                                                            \
      ENABLE_GEMINI_WALLET,                                                \
      EXPAND_FEATURE_ENTRIES({                                             \
          "brave-rewards-gemini",                                          \
          "Enable Gemini for Brave Rewards",                               \
          "Enables support for Gemini as an external wallet provider for " \
          "Brave",                                                         \
          kOsDesktop,                                                      \
          FEATURE_VALUE_TYPE(brave_rewards::features::kGeminiFeature),     \
      }))

#define BRAVE_IPFS_FEATURE_ENTRIES                                   \
  IF_BUILDFLAG(ENABLE_IPFS,                                          \
               EXPAND_FEATURE_ENTRIES({                              \
                   "brave-ipfs",                                     \
                   "Enable IPFS",                                    \
                   "Enable native support of IPFS.",                 \
                   kOsDesktop | kOsAndroid,                          \
                   FEATURE_VALUE_TYPE(ipfs::features::kIpfsFeature), \
               }))

#define BRAVE_NATIVE_WALLET_FEATURE_ENTRIES                                   \
  EXPAND_FEATURE_ENTRIES(                                                     \
      {                                                                       \
          "enable-nft-pinning",                                               \
          "Enable NFT pinning",                                               \
          "Enable NFT pinning for Brave Wallet",                              \
          kOsDesktop,                                                         \
          FEATURE_VALUE_TYPE(                                                 \
              brave_wallet::features::kBraveWalletNftPinningFeature),         \
      },                                                                      \
      {                                                                       \
          "enable-panel-v2",                                                  \
          "Enable Panel v2",                                                  \
          "Enable Panel v2 for Brave Wallet",                                 \
          kOsDesktop,                                                         \
          FEATURE_VALUE_TYPE(                                                 \
              brave_wallet::features::kBraveWalletPanelV2Feature),            \
      },                                                                      \
      {                                                                       \
          "native-brave-wallet",                                              \
          "Enable Brave Wallet",                                              \
          "Native cryptocurrency wallet support without the use of "          \
          "extensions",                                                       \
          kOsDesktop | kOsAndroid,                                            \
          FEATURE_VALUE_TYPE(                                                 \
              brave_wallet::features::kNativeBraveWalletFeature),             \
      },                                                                      \
      {                                                                       \
          "brave-wallet-filecoin",                                            \
          "Enable Brave Wallet Filecoin support",                             \
          "Filecoin support for native Brave Wallet",                         \
          kOsDesktop | kOsAndroid,                                            \
          FEATURE_VALUE_TYPE(                                                 \
              brave_wallet::features::kBraveWalletFilecoinFeature),           \
      },                                                                      \
      {                                                                       \
          "brave-wallet-solana",                                              \
          "Enable Brave Wallet Solana support",                               \
          "Solana support for native Brave Wallet",                           \
          kOsDesktop | kOsAndroid,                                            \
          FEATURE_VALUE_TYPE(                                                 \
              brave_wallet::features::kBraveWalletSolanaFeature),             \
      },                                                                      \
      {                                                                       \
          "brave-wallet-solana-provider",                                     \
          "Enable Brave Wallet Solana provider support",                      \
          "Solana provider support for native Brave Wallet",                  \
          kOsDesktop | kOsAndroid,                                            \
          FEATURE_VALUE_TYPE(                                                 \
              brave_wallet::features::kBraveWalletSolanaProviderFeature),     \
      },                                                                      \
      {                                                                       \
          "brave-wallet-sns",                                                 \
          "Enable Solana Name Service support",                               \
          "Enable Solana Name Service(.sol) support for Wallet and omnibox "  \
          "address resolution",                                               \
          kOsDesktop | kOsAndroid,                                            \
          FEATURE_VALUE_TYPE(brave_wallet::features::kBraveWalletSnsFeature), \
      },                                                                      \
      {                                                                       \
          "brave-wallet-dapps-support",                                       \
          "Enable Brave Wallet Dapps support",                                \
          "Brave Wallet Dapps support",                                       \
          kOsDesktop | kOsAndroid,                                            \
          FEATURE_VALUE_TYPE(                                                 \
              brave_wallet::features::kBraveWalletDappsSupportFeature),       \
      })

#define BRAVE_NEWS_FEATURE_ENTRIES                                             \
  EXPAND_FEATURE_ENTRIES(                                                      \
      {                                                                        \
          "brave-news-peek",                                                   \
          "Brave News prompts on New Tab Page",                                \
          "Prompt Brave News via the top featured article peeking up from "    \
          "the bottom of the New Tab Page, after a short delay.",              \
          kOsDesktop,                                                          \
          FEATURE_VALUE_TYPE(brave_news::features::kBraveNewsCardPeekFeature), \
      },                                                                       \
      {                                                                        \
          "brave-news-feed-update",                                            \
          "Brave News Feed Update",                                            \
          "Use the updated Brave News feed",                                   \
          kOsDesktop,                                                          \
          FEATURE_VALUE_TYPE(brave_news::features::kBraveNewsFeedUpdate),      \
      })

#define BRAVE_FEDERATED_FEATURE_ENTRIES                                        \
  EXPAND_FEATURE_ENTRIES({                                                     \
      "brave-federated",                                                       \
      "Enables local data collection for notification ad timing "              \
      "(brave-federated)",                                                     \
      "Starts local collection for notification ad timing data. This data is " \
      "stored locally and automatically erased after one month. No data "      \
      "leaves the client.",                                                    \
      kOsDesktop,                                                              \
      FEATURE_VALUE_TYPE(brave_federated::features::kFederatedLearning),       \
  })

#define CRYPTO_WALLETS_FEATURE_ENTRIES                                      \
  IF_BUILDFLAG(                                                             \
      ETHEREUM_REMOTE_CLIENT_ENABLED,                                       \
      EXPAND_FEATURE_ENTRIES({                                              \
          "ethereum_remote-client_new-installs",                            \
          "Enable Crypto Wallets option in settings",                       \
          "Crypto Wallets extension is deprecated but with this option it " \
          "can "                                                            \
          "still be enabled in settings. If it was previously used, this "  \
          "flag is "                                                        \
          "ignored.",                                                       \
          kOsDesktop,                                                       \
          FEATURE_VALUE_TYPE(ethereum_remote_client::features::             \
                                 kCryptoWalletsForNewInstallsFeature),      \
      }))

#define PLAYLIST_FEATURE_ENTRIES                                       \
  IF_BUILDFLAG(                                                        \
      ENABLE_PLAYLIST,                                                 \
      EXPAND_FEATURE_ENTRIES(                                          \
          {                                                            \
              kPlaylistFeatureInternalName,                            \
              "Playlist",                                              \
              "Enables Playlist",                                      \
              kOsMac | kOsWin | kOsLinux | kOsAndroid,                 \
              FEATURE_VALUE_TYPE(playlist::features::kPlaylist),       \
          },                                                           \
          {                                                            \
              kPlaylistFakeUAFeatureInternalName,                      \
              "PlaylistFakeUA",                                        \
              "Use fake UA for playlist",                              \
              kOsMac | kOsWin | kOsLinux | kOsAndroid,                 \
              FEATURE_VALUE_TYPE(playlist::features::kPlaylistFakeUA), \
          }))

#if !BUILDFLAG(IS_ANDROID)
#define BRAVE_COMMANDS_FEATURE_ENTRIES                                        \
  EXPAND_FEATURE_ENTRIES({                                                    \
      "brave-commands",                                                       \
      "Brave Commands",                                                       \
      "Enable experimental page for viewing and executing commands in Brave", \
      kOsWin | kOsMac | kOsLinux,                                             \
      FEATURE_VALUE_TYPE(commands::features::kBraveCommands),                 \
  })
#else
#define BRAVE_COMMANDS_FEATURE_ENTRIES
#endif

#if BUILDFLAG(IS_LINUX)
#define BRAVE_CHANGE_ACTIVE_TAB_ON_SCROLL_EVENT_FEATURE_ENTRIES               \
  EXPAND_FEATURE_ENTRIES({                                                    \
      "brave-change-active-tab-on-scroll-event",                              \
      "Change active tab on scroll event",                                    \
      "Change the active tab when scroll events occur on tab strip.",         \
      kOsLinux,                                                               \
      FEATURE_VALUE_TYPE(tabs::features::kBraveChangeActiveTabOnScrollEvent), \
  })
#else
#define BRAVE_CHANGE_ACTIVE_TAB_ON_SCROLL_EVENT_FEATURE_ENTRIES
#endif

#if BUILDFLAG(IS_ANDROID)
#define BRAVE_BACKGROUND_VIDEO_PLAYBACK_ANDROID                                \
  EXPAND_FEATURE_ENTRIES({                                                     \
      "brave-background-video-playback",                                       \
      "Background video playback",                                             \
      "Enables play audio from video in background when tab is not active or " \
      "device screen is turned off. Try to switch to desktop mode if this "    \
      "feature is not working.",                                               \
      kOsAndroid,                                                              \
      FEATURE_VALUE_TYPE(                                                      \
          preferences::features::kBraveBackgroundVideoPlayback),               \
  })
#define BRAVE_SAFE_BROWSING_ANDROID                                           \
  EXPAND_FEATURE_ENTRIES({                                                    \
      "brave-safe-browsing",                                                  \
      "Safe Browsing",                                                        \
      "Enables Google Safe Browsing for determining whether a URL has been "  \
      "marked as a known threat.",                                            \
      kOsAndroid,                                                             \
      FEATURE_VALUE_TYPE(safe_browsing::features::kBraveAndroidSafeBrowsing), \
  })
#else
#define BRAVE_BACKGROUND_VIDEO_PLAYBACK_ANDROID
#define BRAVE_SAFE_BROWSING_ANDROID
#endif  // BUILDFLAG(IS_ANDROID)

#if !BUILDFLAG(IS_ANDROID)
#define BRAVE_TABS_FEATURE_ENTRIES                                        \
  EXPAND_FEATURE_ENTRIES(                                                 \
      {                                                                   \
          "brave-shared-pinned-tabs",                                     \
          "Shared pinned tab",                                            \
          "Pinned tabs are shared across windows",                        \
          kOsWin | kOsMac | kOsLinux,                                     \
          FEATURE_VALUE_TYPE(tabs::features::kBraveSharedPinnedTabs),     \
      },                                                                  \
      {                                                                   \
          "brave-horizontal-tabs-update",                                 \
          "Updated horizontal tabs design",                               \
          "Updates the look and feel or horizontal tabs",                 \
          kOsWin | kOsMac | kOsLinux,                                     \
          FEATURE_VALUE_TYPE(tabs::features::kBraveHorizontalTabsUpdate), \
      })
#else
#define BRAVE_TABS_FEATURE_ENTRIES
#endif

#if BUILDFLAG(ENABLE_AI_CHAT)
#define BRAVE_AI_CHAT                                          \
  EXPAND_FEATURE_ENTRIES({                                     \
      "brave-ai-chat",                                         \
      "Brave AI Chat",                                         \
      "Summarize articles and engage in conversation with AI", \
      kOsWin | kOsMac | kOsLinux | kOsAndroid,                 \
      FEATURE_VALUE_TYPE(ai_chat::features::kAIChat),          \
  })
#define BRAVE_AI_CHAT_HISTORY                                \
  EXPAND_FEATURE_ENTRIES({                                   \
      "brave-ai-chat-history",                               \
      "Brave AI Chat History",                               \
      "Enables AI Chat History persistence and management",  \
      kOsWin | kOsMac | kOsLinux,                            \
      FEATURE_VALUE_TYPE(ai_chat::features::kAIChatHistory), \
  })
#else
#define BRAVE_AI_CHAT
#define BRAVE_AI_CHAT_HISTORY
#endif

#define BRAVE_OMNIBOX_FEATURES                                              \
  EXPAND_FEATURE_ENTRIES({                                                  \
      "brave-omnibox-tab-switch-by-default",                                \
      "Brave Tab Switch by Default",                                        \
      "Prefer switching to already open tabs, rather than navigating in a " \
      "new tab",                                                            \
      kOsWin | kOsLinux | kOsMac,                                           \
      FEATURE_VALUE_TYPE(omnibox::kOmniboxTabSwitchByDefault),              \
  })

// Keep the last item empty.
#define LAST_BRAVE_FEATURE_ENTRIES_ITEM

#define BRAVE_ABOUT_FLAGS_FEATURE_ENTRIES                                      \
  EXPAND_FEATURE_ENTRIES(                                                      \
      {                                                                        \
          "use-dev-updater-url",                                               \
          "Use dev updater url",                                               \
          "Use the dev url for the component updater. This is for internal "   \
          "testing only.",                                                     \
          kOsAll,                                                              \
          FEATURE_VALUE_TYPE(brave_component_updater::kUseDevUpdaterUrl),      \
      },                                                                       \
      {                                                                        \
          "allow-certain-client-hints",                                        \
          "Allow certain request client hints",                                \
          "Allows setting certain request client hints (sec-ch-ua, "           \
          "sec-ch-ua-mobile, sec-ch-ua-platform)",                             \
          kOsAll,                                                              \
          FEATURE_VALUE_TYPE(blink::features::kAllowCertainClientHints),       \
      },                                                                       \
      {                                                                        \
          "brave-ntp-branded-wallpaper-demo",                                  \
          "New Tab Page Demo Branded Wallpaper",                               \
          "Force dummy data for the Branded Wallpaper New Tab Page "           \
          "Experience. View rate and user opt-in conditionals will still be "  \
          "followed to decide when to display the Branded Wallpaper.",         \
          kOsAll,                                                              \
          FEATURE_VALUE_TYPE(                                                  \
              ntp_background_images::features::kBraveNTPBrandedWallpaperDemo), \
      },                                                                       \
      {                                                                        \
          "brave-adblock-cname-uncloaking",                                    \
          "Enable CNAME uncloaking",                                           \
          "Take DNS CNAME records into account when making network request "   \
          "blocking decisions.",                                               \
          kOsAll,                                                              \
          FEATURE_VALUE_TYPE(                                                  \
              brave_shields::features::kBraveAdblockCnameUncloaking),          \
      },                                                                       \
      {                                                                        \
          "brave-adblock-collapse-blocked-elements",                           \
          "Collapse HTML elements with blocked source attributes",             \
          "Cause iframe and img elements to be collapsed if the URL of their " \
          "src attribute is blocked",                                          \
          kOsAll,                                                              \
          FEATURE_VALUE_TYPE(                                                  \
              brave_shields::features::kBraveAdblockCollapseBlockedElements),  \
      },                                                                       \
      {                                                                        \
          "brave-adblock-cookie-list-default",                                 \
          "Treat 'Easylist-Cookie List' as a default list source",             \
          "Enables the 'Easylist-Cookie List' regional list if its toggle in " \
          "brave://adblock hasn't otherwise been modified",                    \
          kOsAll,                                                              \
          FEATURE_VALUE_TYPE(                                                  \
              brave_shields::features::kBraveAdblockCookieListDefault),        \
      },                                                                       \
      {                                                                        \
          "brave-adblock-cookie-list-opt-in",                                  \
          "Show an opt-in bubble for the 'Easylist-Cookie List' filter",       \
          "When enabled, a bubble will be displayed inviting the user to "     \
          "enable the 'Easylist-Cookie List' filter for blocking cookie "      \
          "consent dialogs",                                                   \
          kOsDesktop | kOsAndroid,                                             \
          FEATURE_VALUE_TYPE(                                                  \
              brave_shields::features::kBraveAdblockCookieListOptIn),          \
      },                                                                       \
      {                                                                        \
          "brave-adblock-cosmetic-filtering",                                  \
          "Enable cosmetic filtering",                                         \
          "Enable support for cosmetic filtering",                             \
          kOsAll,                                                              \
          FEATURE_VALUE_TYPE(                                                  \
              brave_shields::features::kBraveAdblockCosmeticFiltering),        \
      },                                                                       \
      {                                                                        \
          "brave-adblock-csp-rules",                                           \
          "Enable support for CSP rules",                                      \
          "Applies additional CSP rules to pages for which a $csp rule has "   \
          "been loaded from a filter list",                                    \
          kOsAll,                                                              \
          FEATURE_VALUE_TYPE(brave_shields::features::kBraveAdblockCspRules),  \
      },                                                                       \
      {                                                                        \
          "brave-adblock-default-1p-blocking",                                 \
          "Shields first-party network blocking",                              \
          "Allow Brave Shields to block first-party network requests in "      \
          "Standard blocking mode",                                            \
          kOsAll,                                                              \
          FEATURE_VALUE_TYPE(                                                  \
              brave_shields::features::kBraveAdblockDefault1pBlocking),        \
      },                                                                       \
      {                                                                        \
          "brave-adblock-mobile-notifications-list-default",                   \
          "Treat 'Fanboy's Mobile Notifications List' as a default list "      \
          "source",                                                            \
                                                                               \
          "Enables the 'Fanboy's Mobile Notifications List' regional list if " \
          "its toggle in brave://adblock hasn't otherwise been modified",      \
          kOsAll,                                                              \
          FEATURE_VALUE_TYPE(brave_shields::features::                         \
                                 kBraveAdblockMobileNotificationsListDefault), \
      },                                                                       \
      {                                                                        \
          "brave-adblock-scriptlet-debug-logs",                                \
          "Enable debug logging for scriptlet injections",                     \
          "Enable console debugging for scriptlets injected by cosmetic "      \
          "filtering, exposing additional information that can be useful for " \
          "filter authors.",                                                   \
          kOsDesktop,                                                          \
          FEATURE_VALUE_TYPE(                                                  \
              brave_shields::features::kBraveAdblockScriptletDebugLogs),       \
      },                                                                       \
      {                                                                        \
          "brave-dark-mode-block",                                             \
          "Enable dark mode blocking fingerprinting protection",               \
          "Always report light mode when fingerprinting protections set to "   \
          "Strict",                                                            \
          kOsAll,                                                              \
          FEATURE_VALUE_TYPE(brave_shields::features::kBraveDarkModeBlock),    \
      },                                                                       \
      {                                                                        \
          "brave-domain-block",                                                \
          "Enable domain blocking",                                            \
          "Enable support for blocking domains with an interstitial page",     \
          kOsAll,                                                              \
          FEATURE_VALUE_TYPE(brave_shields::features::kBraveDomainBlock),      \
      },                                                                       \
      {                                                                        \
          "brave-domain-block-1pes",                                           \
          "Enable domain blocking using First Party Ephemeral Storage",        \
          "When visiting a blocked domain, Brave will try to enable "          \
          "Ephemeral Storage for a first party context, meaning neither "      \
          "cookies nor localStorage data will be persisted after a website "   \
          "is closed. Ephemeral Storage will be auto-enabled only if no data " \
          "was previously stored for a website",                               \
          kOsAll,                                                              \
          FEATURE_VALUE_TYPE(brave_shields::features::kBraveDomainBlock1PES),  \
      },                                                                       \
      {                                                                        \
          "brave-debounce",                                                    \
          "Enable debouncing",                                                 \
          "Enable support for skipping top-level redirect tracking URLs",      \
          kOsAll,                                                              \
          FEATURE_VALUE_TYPE(debounce::features::kBraveDebounce),              \
      },                                                                       \
      {                                                                        \
          "brave-de-amp",                                                      \
          "Enable De-AMP",                                                     \
          "Enable De-AMPing feature",                                          \
          kOsAll,                                                              \
          FEATURE_VALUE_TYPE(de_amp::features::kBraveDeAMP),                   \
      },                                                                       \
      {                                                                        \
          "brave-google-sign-in-permission",                                   \
          "Enable Google Sign-In Permission Prompt",                           \
          "Enable permissioning access to legacy Google Sign-In",              \
          kOsAll,                                                              \
          FEATURE_VALUE_TYPE(google_sign_in_permission::features::             \
                                 kBraveGoogleSignInPermission),                \
      },                                                                       \
      {                                                                        \
          "brave-localhost-access-permission",                                 \
          "Enable Localhost access permission prompt",                         \
          "Enable permissioning access to localhost connections",              \
          kOsAll,                                                              \
          FEATURE_VALUE_TYPE(                                                  \
              brave_shields::features::kBraveLocalhostAccessPermission),       \
      },                                                                       \
      {                                                                        \
          "brave-extension-network-blocking",                                  \
          "Enable extension network blocking",                                 \
          "Enable blocking for network requests initiated by extensions",      \
          kOsAll,                                                              \
          FEATURE_VALUE_TYPE(                                                  \
              brave_shields::features::kBraveExtensionNetworkBlocking),        \
      },                                                                       \
      {                                                                        \
          "brave-reduce-language",                                             \
          "Reduce language identifiability",                                   \
          "Reduce the identifiability of my language preferences",             \
          kOsAll,                                                              \
          FEATURE_VALUE_TYPE(brave_shields::features::kBraveReduceLanguage),   \
      },                                                                       \
      {                                                                        \
          "brave-cosmetic-filtering-sync-load",                                \
          "Enable sync loading of cosmetic filter rules",                      \
          "Enable sync loading of cosmetic filter rules",                      \
          kOsAll,                                                              \
          FEATURE_VALUE_TYPE(                                                  \
              brave_shields::features::kCosmeticFilteringSyncLoad),            \
      },                                                                       \
      {                                                                        \
          "brave-super-referral",                                              \
          "Enable Brave Super Referral",                                       \
          "Use custom theme for Brave Super Referral",                         \
          flags_ui::kOsMac | flags_ui::kOsWin | flags_ui::kOsAndroid,          \
          FEATURE_VALUE_TYPE(ntp_background_images::features::                 \
                                 kBraveNTPSuperReferralWallpaper),             \
      },                                                                       \
      {                                                                        \
          "brave-ephemeral-storage",                                           \
          "Enable Ephemeral Storage",                                          \
          "Use ephemeral storage for third-party frames",                      \
          kOsAll,                                                              \
          FEATURE_VALUE_TYPE(net::features::kBraveEphemeralStorage),           \
      },                                                                       \
      {                                                                        \
          "brave-ephemeral-storage-keep-alive",                                \
          "Ephemeral Storage Keep Alive",                                      \
          "Keep ephemeral storage partitions alive for a specified time "      \
          "after all tabs for that origin are closed",                         \
          kOsAll,                                                              \
          FEATURE_VALUE_TYPE(net::features::kBraveEphemeralStorageKeepAlive),  \
      },                                                                       \
      {                                                                        \
          "brave-first-party-ephemeral-storage",                               \
          "Enable First Party Ephemeral Storage",                              \
          "Enable support for First Party Ephemeral Storage using "            \
          "SESSION_ONLY cookie setting",                                       \
          kOsAll,                                                              \
          FEATURE_VALUE_TYPE(net::features::kBraveFirstPartyEphemeralStorage), \
      },                                                                       \
      {                                                                        \
          "brave-forget-first-party-storage",                                  \
          "Enable First Party Storage Cleanup support",                        \
          "Add cookie blocking mode which allows Brave to cleanup first "      \
          "party storage (Cookies, DOM Storage) on website close",             \
          kOsAll,                                                              \
          FEATURE_VALUE_TYPE(net::features::kBraveForgetFirstPartyStorage),    \
      },                                                                       \
      {                                                                        \
          "brave-rewards-vbat-notice",                                         \
          "Enable Brave Rewards VBAT notices",                                 \
          "Enables notices in the Brave Rewards UI about VBAT deadlines.",     \
          kOsDesktop | kOsAndroid,                                             \
          FEATURE_VALUE_TYPE(brave_rewards::features::kVBatNoticeFeature),     \
      },                                                                       \
      {                                                                        \
          "brave-rewards-verbose-logging",                                     \
          "Enable Brave Rewards verbose logging",                              \
          "Enables detailed logging of Brave Rewards system events to a log "  \
          "file stored on your device. Please note that this log file could "  \
          "include information such as browsing history and credentials such " \
          "as passwords and access tokens depending on your activity. Please " \
          "do not share it unless asked to by Brave staff.",                   \
          kOsDesktop | kOsAndroid,                                             \
          FEATURE_VALUE_TYPE(brave_rewards::features::kVerboseLoggingFeature), \
      },                                                                       \
      {                                                                        \
          "brave-rewards-allow-unsupported-wallet-providers",                  \
          "Always show Brave Rewards custodial connection options",            \
                                                                               \
          "Allows all custodial options to be selected in Brave Rewards, "     \
          "including those not supported for your Rewards country.",           \
          kOsDesktop | kOsAndroid,                                             \
          FEATURE_VALUE_TYPE(brave_rewards::features::                         \
                                 kAllowUnsupportedWalletProvidersFeature),     \
      },                                                                       \
      {                                                                        \
          "brave-ads-should-launch-brave-ads-as-an-in-process-service",        \
          "Launch Brave Ads as an in-process service",                         \
          "Launch Brave Ads as an in-process service removing the utility "    \
          "process.",                                                          \
          kOsAll,                                                              \
          FEATURE_VALUE_TYPE(                                                  \
              brave_ads::kShouldLaunchBraveAdsAsAnInProcessServiceFeature),    \
      },                                                                       \
      {                                                                        \
          "brave-ads-should-always-run-brave-ads-service",                     \
          "Should always run Brave Ads service",                               \
          "Always run Brave Ads service to support triggering ad events when " \
          "Brave Private Ads are disabled.",                                   \
          kOsAll,                                                              \
          FEATURE_VALUE_TYPE(                                                  \
              brave_ads::kShouldAlwaysRunBraveAdsServiceFeature),              \
      },                                                                       \
      {                                                                        \
          "brave-ads-should-always-trigger-new-tab-page-ad-events",            \
          "Should always trigger new tab page ad events",                      \
          "Support triggering new tab page ad events if Brave Private Ads "    \
          "are disabled. Requires "                                            \
          "#brave-ads-should-always-run-brave-ads-service to be enabled.",     \
          kOsAll,                                                              \
          FEATURE_VALUE_TYPE(                                                  \
              brave_ads::kShouldAlwaysTriggerBraveNewTabPageAdEventsFeature),  \
      },                                                                       \
      {                                                                        \
          "brave-ads-should-support-search-result-ads",                        \
          "Support Search Result Ads feature",                                 \
          "Should be used in combination with "                                \
          "#brave-ads-should-always-trigger-search-result-ad-events and "      \
          "#brave-ads-should-always-run-brave-ads-service",                    \
          kOsAll,                                                              \
          FEATURE_VALUE_TYPE(brave_ads::kShouldSupportSearchResultAdsFeature), \
      },                                                                       \
      {                                                                        \
          "brave-ads-should-always-trigger-search-result-ad-events",           \
          "Should always trigger search result ad events",                     \
          "Support triggering search result ad events if Brave Private Ads "   \
          "are disabled. Requires "                                            \
          "#brave-ads-should-always-run-brave-ads-service to be enabled.",     \
          kOsAll,                                                              \
          FEATURE_VALUE_TYPE(                                                  \
              brave_ads::                                                      \
                  kShouldAlwaysTriggerBraveSearchResultAdEventsFeature),       \
      },                                                                       \
      {                                                                        \
          "brave-ads-custom-push-notifications-ads",                           \
          "Enable Brave Ads custom push notifications",                        \
          "Enable Brave Ads custom push notifications to support rich media",  \
          kOsAll,                                                              \
          FEATURE_VALUE_TYPE(brave_ads::kCustomNotificationAdFeature),         \
      },                                                                       \
      {                                                                        \
          "brave-ads-allowed-to-fallback-to-custom-push-notification-ads",     \
          "Allow Brave Ads to fallback from native to custom push "            \
          "notifications",                                                     \
          "Allow Brave Ads to fallback from native to custom push "            \
          "notifications on operating systems which do not support native "    \
          "notifications",                                                     \
          kOsAll,                                                              \
          FEATURE_VALUE_TYPE(                                                  \
              brave_ads::kAllowedToFallbackToCustomNotificationAdFeature),     \
      },                                                                       \
      {                                                                        \
          "brave-sync-v2",                                                     \
          "Enable Brave Sync v2",                                              \
          "Brave Sync v2 integrates with chromium sync engine with Brave "     \
          "specific authentication flow and enforce client side encryption",   \
          kOsDesktop,                                                          \
          FEATURE_VALUE_TYPE(brave_sync::features::kBraveSync),                \
      },                                                                       \
      {                                                                        \
          "file-system-access-api",                                            \
          "File System Access API",                                            \
          "Enables the File System Access API, giving websites access to the " \
          "file system",                                                       \
          kOsDesktop,                                                          \
          FEATURE_VALUE_TYPE(blink::features::kFileSystemAccessAPI),           \
      },                                                                       \
      {                                                                        \
          "brave-web-bluetooth-api",                                           \
          "Web Bluetooth API",                                                 \
          "Enables the Web Bluetooth API, giving websites access to "          \
          "Bluetooth devices",                                                 \
          kOsAll,                                                              \
          FEATURE_VALUE_TYPE(blink::features::kBraveWebBluetoothAPI),          \
      },                                                                       \
      {                                                                        \
          "brave-web-serial-api",                                              \
          "Web Serial API",                                                    \
          "Enables the Web Serial API, allowing websites to request access "   \
          "to serial ports ",                                                  \
          kOsDesktop,                                                          \
          FEATURE_VALUE_TYPE(blink::features::kBraveWebSerialAPI),             \
      },                                                                       \
      {                                                                        \
          "navigator-connection-attribute",                                    \
          "Enable navigator.connection attribute",                             \
          "Enables the navigator.connection API. Enabling this API will "      \
          "allow sites to learn information about your network and internet "  \
          "connection. Trackers can use this information to fingerprint your " \
          "browser, or to infer when you are traveling or at home.",           \
          kOsAll,                                                              \
          FEATURE_VALUE_TYPE(blink::features::kNavigatorConnectionAttribute),  \
      },                                                                       \
      {                                                                        \
          "restrict-websockets-pool",                                          \
          "Restrict WebSockets pool",                                          \
          "Limits simultaneous active WebSockets connections per eTLD+1",      \
          kOsAll,                                                              \
          FEATURE_VALUE_TYPE(blink::features::kRestrictWebSocketsPool),        \
      },                                                                       \
      {                                                                        \
          "allow-incognito-permission-inheritance",                            \
          "Allow permission inheritance in incognito profiles",                \
          "When enabled, most permissions set in a normal profile will be "    \
          "inherited in incognito profile if they are less permissive, for "   \
          "ex. Geolocation BLOCK will be automatically set to BLOCK in "       \
          "incognito.",                                                        \
          kOsAll,                                                              \
          FEATURE_VALUE_TYPE(                                                  \
              content_settings::kAllowIncognitoPermissionInheritance),         \
      },                                                                       \
      {                                                                        \
          "brave-block-screen-fingerprinting",                                 \
          "Block screen fingerprinting",                                       \
          "Prevents JavaScript and CSS from learning the user's screen "       \
          "dimensions or window position.",                                    \
          kOsAll,                                                              \
          FEATURE_VALUE_TYPE(                                                  \
              blink::features::kBraveBlockScreenFingerprinting),               \
      },                                                                       \
      {                                                                        \
          "brave-tor-windows-https-only",                                      \
          "Use HTTPS-Only Mode in Private Windows with Tor",                   \
          "Prevents Private Windows with Tor from making any insecure HTTP "   \
          "connections without warning the user first.",                       \
          kOsAll,                                                              \
          FEATURE_VALUE_TYPE(net::features::kBraveTorWindowsHttpsOnly),        \
      },                                                                       \
      {                                                                        \
          "brave-round-time-stamps",                                           \
          "Round time stamps",                                                 \
          "Prevents JavaScript from getting access to high-resolution clocks " \
          "by rounding all DOMHighResTimeStamps to the nearest millisecond.",  \
          kOsAll,                                                              \
          FEATURE_VALUE_TYPE(blink::features::kBraveRoundTimeStamps),          \
      },                                                                       \
      {                                                                        \
          "translate",                                                         \
          "Enable Chromium Translate feature",                                 \
          "Should be used with brave-translate-go, see the description here.", \
          kOsDesktop | kOsAndroid,                                             \
          FEATURE_VALUE_TYPE(translate::kTranslate),                           \
      },                                                                       \
      {                                                                        \
          "brave-sync-history-diagnostics",                                    \
          "Enable Brave Sync History Diagnostics",                             \
          "Brave Sync History Diagnostics flag displays additional sync "      \
          "related information on History page",                               \
          kOsAll,                                                              \
          FEATURE_VALUE_TYPE(                                                  \
              brave_sync::features::kBraveSyncHistoryDiagnostics),             \
      },                                                                       \
      {                                                                        \
          "restrict-event-source-pool",                                        \
          "Restrict Event Source Pool",                                        \
          "Limits simultaneous active WebSockets connections per eTLD+1",      \
          kOsAll,                                                              \
          FEATURE_VALUE_TYPE(blink::features::kRestrictEventSourcePool),       \
      },                                                                       \
      {                                                                        \
          "brave-sync-send-all-history",                                       \
          "Send All History to Brave Sync",                                    \
          "With Send All History flag all sync entries are sent to Sync "      \
          "server including transitions of link, bookmark, reload, etc",       \
          kOsAll,                                                              \
          FEATURE_VALUE_TYPE(brave_sync::features::kBraveSyncSendAllHistory),  \
      },                                                                       \
      {                                                                        \
          "brave-copy-clean-link-by-default",                                  \
          "Override default copy hotkey with copy clean link",                 \
          "Sanitize url before copying, replaces default ctrl+c hotkey for "   \
          "url ",                                                              \
          kOsWin | kOsLinux | kOsMac,                                          \
          FEATURE_VALUE_TYPE(features::kBraveCopyCleanLinkByDefault),          \
      },                                                                       \
      {                                                                        \
          "brave-global-privacy-control-enabled",                              \
          "Enable Global Privacy Control",                                     \
          "Enable the Sec-GPC request header and the "                         \
          "navigator.globalPrivacyControl JS API",                             \
          kOsAll,                                                              \
          FEATURE_VALUE_TYPE(blink::features::kBraveGlobalPrivacyControl),     \
      },                                                                       \
      {                                                                        \
          "https-by-default",                                                  \
          "Use HTTPS by Default",                                              \
          "Attempt to connect to all websites using HTTPS before falling "     \
          "back to HTTP.",                                                     \
          kOsAll,                                                              \
          FEATURE_VALUE_TYPE(net::features::kBraveHttpsByDefault),             \
      },                                                                       \
      {                                                                        \
          "brave-override-download-danger-level",                              \
          "Override download danger level",                                    \
          "Disables download warnings for files which are considered "         \
          "dangerous when Safe Browsing is disabled. Use at your own risks. "  \
          "Not recommended.",                                                  \
          kOsWin | kOsLinux | kOsMac,                                          \
          FEATURE_VALUE_TYPE(features::kBraveOverrideDownloadDangerLevel),     \
      })                                                                       \
  BRAVE_IPFS_FEATURE_ENTRIES                                                   \
  BRAVE_NATIVE_WALLET_FEATURE_ENTRIES                                          \
  BRAVE_NEWS_FEATURE_ENTRIES                                                   \
  CRYPTO_WALLETS_FEATURE_ENTRIES                                               \
  BRAVE_REWARDS_GEMINI_FEATURE_ENTRIES                                         \
  BRAVE_VPN_FEATURE_ENTRIES                                                    \
  BRAVE_VPN_DNS_FEATURE_ENTRIES                                                \
  BRAVE_VPN_WIREGUARD_FEATURE_ENTRIES                                          \
  BRAVE_SKU_SDK_FEATURE_ENTRIES                                                \
  SPEEDREADER_FEATURE_ENTRIES                                                  \
  REQUEST_OTR_FEATURE_ENTRIES                                                  \
  BRAVE_MODULE_FILENAME_PATCH                                                  \
  BRAVE_FEDERATED_FEATURE_ENTRIES                                              \
  PLAYLIST_FEATURE_ENTRIES                                                     \
  BRAVE_COMMANDS_FEATURE_ENTRIES                                               \
  BRAVE_BACKGROUND_VIDEO_PLAYBACK_ANDROID                                      \
  BRAVE_SAFE_BROWSING_ANDROID                                                  \
  BRAVE_CHANGE_ACTIVE_TAB_ON_SCROLL_EVENT_FEATURE_ENTRIES                      \
  BRAVE_TABS_FEATURE_ENTRIES                                                   \
  BRAVE_AI_CHAT                                                                \
  BRAVE_AI_CHAT_HISTORY                                                        \
  BRAVE_OMNIBOX_FEATURES                                                       \
  LAST_BRAVE_FEATURE_ENTRIES_ITEM  // Keep it as the last item.
namespace flags_ui {
namespace {

// Unused function to reference Brave feature entries for clang checks.
[[maybe_unused]] void UseBraveAboutFlags() {
  // These vars are declared in anonymous namespace in
  // //chrome/browser/about_flags.cc. We declare them here manually to
  // instantiate BRAVE_ABOUT_FLAGS_FEATURE_ENTRIES without errors.
  constexpr int kOsAll = 0;
  constexpr int kOsDesktop = 0;

  static_assert(
      std::initializer_list<FeatureEntry>{BRAVE_ABOUT_FLAGS_FEATURE_ENTRIES}
          .size());
}

}  // namespace
}  // namespace flags_ui
