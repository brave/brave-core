/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/feature_override.h"
#include "brave/browser/android/background_video/features.h"
#include "brave/browser/android/safe_browsing/features.h"
#include "brave/components/ai_chat/core/common/features.h"
#include "brave/components/brave_news/common/features.h"
#include "brave/components/brave_rewards/common/features.h"
#include "brave/components/brave_search_conversion/features.h"
#include "brave/components/brave_shields/core/common/features.h"
#include "brave/components/brave_vpn/common/features.h"
#include "brave/components/brave_wallet/common/features.h"
#include "brave/components/debounce/core/common/features.h"
#include "brave/components/google_sign_in_permission/features.h"
#include "brave/components/playlist/common/features.h"
#include "brave/components/request_otr/common/features.h"
#include "brave/components/speedreader/common/features.h"
#include "brave/components/webcompat/core/common/features.h"
#include "net/base/features.h"
#include "third_party/blink/public/common/features.h"

#define BRAVE_AI_CHAT_FLAG &ai_chat::features::kAIChat,

// clang-format off
#define kForceWebContentsDarkMode kForceWebContentsDarkMode,            \
    BRAVE_AI_CHAT_FLAG                                                  \
    &brave_rewards::features::kBraveRewards,                            \
    &brave_search_conversion::features::kOmniboxBanner,                 \
    &brave_vpn::features::kBraveVPNLinkSubscriptionAndroidUI,           \
    &brave_wallet::features::kNativeBraveWalletFeature,                 \
    &playlist::features::kPlaylist,                                     \
    &download::features::kParallelDownloading,                          \
    &preferences::features::kBraveBackgroundVideoPlayback,              \
    &request_otr::features::kBraveRequestOTRTab,                        \
    &safe_browsing::features::kBraveAndroidSafeBrowsing,                \
    &speedreader::kSpeedreaderFeature,                                  \
    &debounce::features::kBraveDebounce,                                \
    &webcompat::features::kBraveWebcompatExceptionsService,             \
    &net::features::kBraveHttpsByDefault,                               \
    &net::features::kBraveFallbackDoHProvider,                          \
    &google_sign_in_permission::features::kBraveGoogleSignInPermission, \
    &net::features::kBraveForgetFirstPartyStorage,                      \
    &brave_shields::features::kBraveShowStrictFingerprintingMode,       \
    &brave_shields::features::kBraveLocalhostAccessPermission,          \
    &brave_shields::features::kBlockAllCookiesToggle

// clang-format on

#include "src/chrome/browser/flags/android/chrome_feature_list.cc"
#undef kForceWebContentsDarkMode
#undef BRAVE_AI_CHAT_FLAG

namespace chrome {
namespace android {

OVERRIDE_FEATURE_DEFAULT_STATES({{
    {kIncognitoReauthenticationForAndroid, base::FEATURE_ENABLED_BY_DEFAULT},
    {kMagicStackAndroid, base::FEATURE_DISABLED_BY_DEFAULT},
    {kAdaptiveButtonInTopToolbarCustomizationV2,
     base::FEATURE_DISABLED_BY_DEFAULT},
}});

}  // namespace android
}  // namespace chrome
