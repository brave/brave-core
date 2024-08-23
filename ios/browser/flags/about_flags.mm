// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// This file is included into //ios/chrome/browser/flags/about_flags.mm

#include "base/strings/string_util.h"
#include "brave/components/ai_chat/core/common/buildflags/buildflags.h"
#include "brave/components/brave_ads/core/public/ads_feature.h"
#include "brave/components/brave_component_updater/browser/features.h"
#include "brave/components/brave_rewards/common/buildflags/buildflags.h"
#include "brave/components/brave_rewards/common/features.h"
#include "brave/components/brave_shields/core/common/features.h"
#include "brave/components/brave_wallet/common/features.h"
#include "brave/components/de_amp/common/features.h"
#include "brave/components/debounce/core/common/features.h"
#include "brave/components/ntp_background_images/browser/features.h"
#include "brave/components/skus/common/features.h"
#include "brave/ios/browser/playlist/features.h"
#include "build/build_config.h"
#include "components/flags_ui/feature_entry_macros.h"
#include "components/flags_ui/flags_state.h"
#include "ios/components/security_interstitials/https_only_mode/feature.h"
#include "net/base/features.h"

#if BUILDFLAG(ENABLE_AI_CHAT)
#include "brave/components/ai_chat/core/common/features.h"
#endif

#define EXPAND_FEATURE_ENTRIES(...) __VA_ARGS__,

#define BRAVE_SKU_SDK_FEATURE_ENTRIES                   \
  EXPAND_FEATURE_ENTRIES({                              \
      "skus-sdk",                                       \
      "Enable experimental SKU SDK",                    \
      "Experimental SKU SDK support",                   \
      flags_ui::kOsIos,                                 \
      FEATURE_VALUE_TYPE(skus::features::kSkusFeature), \
  })

#define BRAVE_NATIVE_WALLET_FEATURE_ENTRIES                                   \
  EXPAND_FEATURE_ENTRIES(                                                     \
      {                                                                       \
          "brave-wallet-zcash",                                               \
          "Enable BraveWallet ZCash support",                                 \
          "Zcash support for native Brave Wallet",                            \
          flags_ui::kOsIos,                                                   \
          FEATURE_VALUE_TYPE(                                                 \
              brave_wallet::features::kBraveWalletZCashFeature),              \
      },                                                                      \
      {                                                                       \
          "brave-wallet-bitcoin",                                             \
          "Enable Brave Wallet Bitcoin support",                              \
          "Bitcoin support for native Brave Wallet",                          \
          flags_ui::kOsIos,                                                   \
          FEATURE_VALUE_TYPE(                                                 \
              brave_wallet::features::kBraveWalletBitcoinFeature),            \
      },                                                                      \
      {                                                                       \
          "brave-wallet-enable-ankr-balances",                                \
          "Enable Ankr balances",                                             \
          "Enable usage of Ankr Advanced API for fetching balances in Brave " \
          "Wallet",                                                           \
          flags_ui::kOsIos,                                                   \
          FEATURE_VALUE_TYPE(                                                 \
              brave_wallet::features::kBraveWalletAnkrBalancesFeature),       \
      },                                                                      \
      {                                                                       \
          "brave-wallet-enable-transaction-simulations",                      \
          "Enable transaction simulations",                                   \
          "Enable usage of Blowfish API for running transaction simulations " \
          "in Brave Wallet",                                                  \
          flags_ui::kOsIos,                                                   \
          FEATURE_VALUE_TYPE(brave_wallet::features::                         \
                                 kBraveWalletTransactionSimulationsFeature),  \
      })

#define BRAVE_SHIELDS_FEATURE_ENTRIES                                        \
  EXPAND_FEATURE_ENTRIES(                                                    \
      {                                                                      \
          "brave-shred",                                                     \
          "Enable Brave 'Shred' Feature",                                    \
          "Enable the Brave ‘Shred’ feature which will allow a user to " \
          "easily delete all site data on demand or automatically when "     \
          "closing a site or terminating the application.",                  \
          flags_ui::kOsIos,                                                  \
          FEATURE_VALUE_TYPE(brave_shields::features::kBraveShredFeature),   \
      },                                                                     \
      {                                                                      \
          "https-by-default",                                                \
          "Use HTTPS by Default",                                            \
          "Attempt to connect to all websites using HTTPS before falling "   \
          "back to HTTP.",                                                   \
          flags_ui::kOsIos,                                                  \
          FEATURE_VALUE_TYPE(net::features::kBraveHttpsByDefault),           \
      },                                                                     \
      {                                                                      \
          "https-only-mode",                                                 \
          "Enable HTTPS By Default Strict Mode",                             \
          "Connect to all websites using HTTPS and display an intersitital " \
          "to fallback to HTTP",                                             \
          flags_ui::kOsIos,                                                  \
          FEATURE_VALUE_TYPE(                                                \
              security_interstitials::features::kHttpsOnlyMode),             \
      })

#if BUILDFLAG(ENABLE_AI_CHAT)
#define BRAVE_AI_CHAT                                          \
  EXPAND_FEATURE_ENTRIES({                                     \
      "brave-ai-chat",                                         \
      "Brave AI Chat",                                         \
      "Summarize articles and engage in conversation with AI", \
      flags_ui::kOsIos,                                        \
      FEATURE_VALUE_TYPE(ai_chat::features::kAIChat),          \
  })
#define BRAVE_AI_CHAT_HISTORY                                \
  EXPAND_FEATURE_ENTRIES({                                   \
      "brave-ai-chat-history",                               \
      "Brave AI Chat History",                               \
      "Enables AI Chat History persistence and management",  \
      flags_ui::kOsIos,                                      \
      FEATURE_VALUE_TYPE(ai_chat::features::kAIChatHistory), \
  })
#define BRAVE_AI_CHAT_PAGE_CONTENT_REFINE                                   \
  EXPAND_FEATURE_ENTRIES({                                                  \
      "brave-ai-chat-page-content-refine",                                  \
      "Brave AI Chat Page Content Refine",                                  \
      "Enable local text embedding for long page content in order to find " \
      "most relevant parts to the prompt within context limit.",            \
      flags_ui::kOsIos,                                                     \
      FEATURE_VALUE_TYPE(ai_chat::features::kPageContentRefine),            \
  })
#else
#define BRAVE_AI_CHAT
#define BRAVE_AI_CHAT_HISTORY
#define BRAVE_AI_CHAT_PAGE_CONTENT_REFINE
#endif

#define BRAVE_PLAYLIST_FEATURE_ENTRIES                        \
  EXPAND_FEATURE_ENTRIES({                                    \
      "brave-new-playlist-ui",                                \
      "Enables new Playlist UI",                              \
      "Enable the revamped Playlist experience",              \
      flags_ui::kOsIos,                                       \
      FEATURE_VALUE_TYPE(playlist::features::kNewPlaylistUI), \
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
          flags_ui::kOsIos,                                                    \
          FEATURE_VALUE_TYPE(brave_component_updater::kUseDevUpdaterUrl),      \
      },                                                                       \
      {                                                                        \
          "brave-ntp-branded-wallpaper-demo",                                  \
          "New Tab Page Demo Branded Wallpaper",                               \
          "Force dummy data for the Branded Wallpaper New Tab Page "           \
          "Experience. View rate and user opt-in conditionals will still be "  \
          "followed to decide when to display the Branded Wallpaper.",         \
          flags_ui::kOsIos,                                                    \
          FEATURE_VALUE_TYPE(                                                  \
              ntp_background_images::features::kBraveNTPBrandedWallpaperDemo), \
      },                                                                       \
      {                                                                        \
          "brave-debounce",                                                    \
          "Enable debouncing",                                                 \
          "Enable support for skipping top-level redirect tracking URLs",      \
          flags_ui::kOsIos,                                                    \
          FEATURE_VALUE_TYPE(debounce::features::kBraveDebounce),              \
      },                                                                       \
      {                                                                        \
          "brave-de-amp",                                                      \
          "Enable De-AMP",                                                     \
          "Enable De-AMPing feature",                                          \
          flags_ui::kOsIos,                                                    \
          FEATURE_VALUE_TYPE(de_amp::features::kBraveDeAMP),                   \
      },                                                                       \
      {                                                                        \
          "brave-super-referral",                                              \
          "Enable Brave Super Referral",                                       \
          "Use custom theme for Brave Super Referral",                         \
          flags_ui::kOsIos,                                                    \
          FEATURE_VALUE_TYPE(ntp_background_images::features::                 \
                                 kBraveNTPSuperReferralWallpaper),             \
      },                                                                       \
      {                                                                        \
          "brave-rewards-verbose-logging",                                     \
          "Enable Brave Rewards verbose logging",                              \
          "Enables detailed logging of Brave Rewards system events to a log "  \
          "file stored on your device. Please note that this log file could "  \
          "include information such as browsing history and credentials such " \
          "as passwords and access tokens depending on your activity. Please " \
          "do not share it unless asked to by Brave staff.",                   \
          flags_ui::kOsIos,                                                    \
          FEATURE_VALUE_TYPE(brave_rewards::features::kVerboseLoggingFeature), \
      },                                                                       \
      {                                                                        \
          "brave-ads-should-always-run-brave-ads-service",                     \
          "Should always run Brave Ads service",                               \
          "Always run Brave Ads service to support triggering ad events when " \
          "Brave Private Ads are disabled.",                                   \
          flags_ui::kOsIos,                                                    \
          FEATURE_VALUE_TYPE(                                                  \
              brave_ads::kShouldAlwaysRunBraveAdsServiceFeature),              \
      },                                                                       \
      {                                                                        \
          "brave-ads-should-always-trigger-new-tab-page-ad-events",            \
          "Should always trigger new tab page ad events",                      \
          "Support triggering new tab page ad events if Brave Private Ads "    \
          "are disabled. Requires "                                            \
          "#brave-ads-should-always-run-brave-ads-service to be enabled.",     \
          flags_ui::kOsIos,                                                    \
          FEATURE_VALUE_TYPE(                                                  \
              brave_ads::kShouldAlwaysTriggerBraveNewTabPageAdEventsFeature),  \
      },                                                                       \
      {                                                                        \
          "brave-ads-should-always-trigger-search-result-ad-events",           \
          "Should always trigger search result ad events",                     \
          "Support triggering search result ad events if Brave Private Ads "   \
          "are disabled. Requires "                                            \
          "#brave-ads-should-always-run-brave-ads-service to be enabled.",     \
          flags_ui::kOsIos,                                                    \
          FEATURE_VALUE_TYPE(                                                  \
              brave_ads::                                                      \
                  kShouldAlwaysTriggerBraveSearchResultAdEventsFeature),       \
      })                                                                       \
  BRAVE_SHIELDS_FEATURE_ENTRIES                                                \
  BRAVE_NATIVE_WALLET_FEATURE_ENTRIES                                          \
  BRAVE_SKU_SDK_FEATURE_ENTRIES                                                \
  BRAVE_AI_CHAT                                                                \
  BRAVE_AI_CHAT_HISTORY                                                        \
  BRAVE_AI_CHAT_PAGE_CONTENT_REFINE                                            \
  BRAVE_PLAYLIST_FEATURE_ENTRIES                                               \
  LAST_BRAVE_FEATURE_ENTRIES_ITEM  // Keep it as the last item.
