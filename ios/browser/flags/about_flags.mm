// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// This file is included into //ios/chrome/browser/flags/about_flags.mm

#include "brave/components/ai_chat/core/common/features.h"
#include "brave/components/brave_component_updater/browser/features.h"
#include "brave/components/brave_rewards/core/features.h"
#include "brave/components/brave_shields/core/common/features.h"
#include "brave/components/brave_sync/features.h"
#include "brave/components/brave_user_agent/common/features.h"
#include "brave/components/brave_wallet/common/features.h"
#include "brave/components/de_amp/common/features.h"
#include "brave/components/debounce/core/common/features.h"
#include "brave/components/ntp_background_images/browser/features.h"
#include "brave/components/skus/common/features.h"
#include "brave/ios/browser/api/translate/features.h"
#include "brave/ios/browser/ui/tab_tray/features.h"
#include "brave/ios/browser/ui/web_view/features.h"
#include "build/build_config.h"
#include "components/webui/flags/feature_entry_macros.h"
#include "components/webui/flags/flags_state.h"
#include "net/base/features.h"

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

#define BRAVE_SHIELDS_FEATURE_ENTRIES                                          \
  EXPAND_FEATURE_ENTRIES(                                                      \
      {                                                                        \
          "brave-shred",                                                       \
          "Enable Brave 'Shred' Feature",                                      \
          "Enable the Brave 'Shred' feature which will allow a user to "       \
          "easily delete all site data on demand or automatically when "       \
          "closing a site or terminating the application.",                    \
          flags_ui::kOsIos,                                                    \
          FEATURE_VALUE_TYPE(brave_shields::features::kBraveShredFeature),     \
      },                                                                       \
      {                                                                        \
          "brave-shred-cache-data",                                            \
          "Shred Clears All Cache Data",                                       \
          "Shred feature will also remove all cache data, in addition to the " \
          "data associated with the site being shred.",                        \
          flags_ui::kOsIos,                                                    \
          FEATURE_VALUE_TYPE(brave_shields::features::kBraveShredCacheData),   \
      },                                                                       \
      {                                                                        \
          "brave-shields-content-settings",                                    \
          "Brave Shields use Content Settings",                                \
          "Brave Shields will use content settings for persisting Shields "    \
          "preferences",                                                       \
          flags_ui::kOsIos,                                                    \
          FEATURE_VALUE_TYPE(                                                  \
              brave_shields::features::kBraveShieldsContentSettingsIOS),       \
      },                                                                       \
      {                                                                        \
          "https-by-default",                                                  \
          "Use HTTPS by Default",                                              \
          "Attempt to connect to all websites using HTTPS before falling "     \
          "back to HTTP.",                                                     \
          flags_ui::kOsIos,                                                    \
          FEATURE_VALUE_TYPE(net::features::kBraveHttpsByDefault),             \
      },                                                                       \
      {                                                                        \
          "block-all-cookies-toggle",                                          \
          "If the feature flag is on, we show the Block all Cookies toggle",   \
          "If the feature flag is on, we show the Block all Cookies toggle",   \
          flags_ui::kOsIos,                                                    \
          FEATURE_VALUE_TYPE(brave_shields::features::kBlockAllCookiesToggle), \
      },                                                                       \
      {                                                                        \
          "ios-debug-adblock",                                                 \
          "Enable Debug Adblock views",                                        \
          "Enable debug view for adblock features in Shields panel",           \
          flags_ui::kOsIos,                                                    \
          FEATURE_VALUE_TYPE(brave_shields::features::kBraveIOSDebugAdblock),  \
      },                                                                       \
      {                                                                        \
          "ios-farble-plugins",                                                \
          "Enable Farbling Plugins",                                           \
          "Enable Farbling plugins when enabled globally / per-domain",        \
          flags_ui::kOsIos,                                                    \
          FEATURE_VALUE_TYPE(                                                  \
              brave_shields::features::kBraveIOSEnableFarblingPlugins),        \
      })

#define BRAVE_AI_CHAT_FEATURE_ENTRIES                                      \
  EXPAND_FEATURE_ENTRIES(                                                  \
      {                                                                    \
          "brave-ai-chat",                                                 \
          "Brave AI Chat",                                                 \
          "Summarize articles and engage in conversation with AI",         \
          flags_ui::kOsIos,                                                \
          FEATURE_VALUE_TYPE(ai_chat::features::kAIChat),                  \
      },                                                                   \
      {                                                                    \
          "brave-ai-chat-history",                                         \
          "Brave AI Chat History",                                         \
          "Enables AI Chat History persistence and management",            \
          flags_ui::kOsIos,                                                \
          FEATURE_VALUE_TYPE(ai_chat::features::kAIChatHistory),           \
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
          "brave-use-modern-tab-tray",                                         \
          "Use modern tab tray UI",                                            \
          "Replace the tab tray UI with a modern replacement",                 \
          flags_ui::kOsIos,                                                    \
          FEATURE_VALUE_TYPE(brave::features::kModernTabTrayEnabled),          \
      },                                                                       \
      {                                                                        \
          "brave-sync-default-passwords",                                      \
          "Enable password syncing by default",                                \
          "Turn on password syncing when Sync is enabled.",                    \
          flags_ui::kOsIos,                                                    \
          FEATURE_VALUE_TYPE(                                                  \
              brave_sync::features::kBraveSyncDefaultPasswords),               \
      },                                                                       \
      {                                                                        \
          "brave-translate-enabled",                                           \
          "Use Brave Translate",                                               \
          "Enables page translation",                                          \
          flags_ui::kOsIos,                                                    \
          FEATURE_VALUE_TYPE(brave::features::kBraveTranslateEnabled),         \
      },                                                                       \
      {                                                                        \
          "brave-translate-apple-enabled",                                     \
          "Use Apple Offline Translate",                                       \
          "Enables page translation using Apple APIs",                         \
          flags_ui::kOsIos,                                                    \
          FEATURE_VALUE_TYPE(brave::features::kBraveAppleTranslateEnabled),    \
      },                                                                       \
      {                                                                        \
          "use-brave-user-agent",                                              \
          "Use Brave user agent",                                              \
          "Includes Brave version information in the user agent",              \
          flags_ui::kOsIos,                                                    \
          FEATURE_VALUE_TYPE(brave_user_agent::features::kUseBraveUserAgent),  \
      },                                                                       \
      {                                                                        \
          "brave-use-chromium-web-embedder",                                   \
          "Use Chromium Web Embedder",                                         \
          "Replace WKWebView usages with Chromium web views",                  \
          flags_ui::kOsIos,                                                    \
          FEATURE_VALUE_TYPE(brave::features::kUseChromiumWebViews),           \
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
      })                                                                       \
  BRAVE_SHIELDS_FEATURE_ENTRIES                                                \
  BRAVE_NATIVE_WALLET_FEATURE_ENTRIES                                          \
  BRAVE_SKU_SDK_FEATURE_ENTRIES                                                \
  BRAVE_AI_CHAT_FEATURE_ENTRIES                                                \
  LAST_BRAVE_FEATURE_ENTRIES_ITEM  // Keep it as the last item.
