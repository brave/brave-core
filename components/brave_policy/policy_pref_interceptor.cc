/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_policy/policy_pref_interceptor.h"

#include "base/containers/map_util.h"
#include "brave/components/ai_chat/core/common/buildflags/buildflags.h"
#include "brave/components/brave_news/common/pref_names.h"
#include "brave/components/brave_rewards/core/pref_names.h"
#include "brave/components/brave_talk/buildflags/buildflags.h"
#include "brave/components/brave_vpn/common/buildflags/buildflags.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/playlist/core/common/pref_names.h"
#include "brave/components/speedreader/common/buildflags/buildflags.h"
#include "components/prefs/pref_value_map.h"

#if BUILDFLAG(ENABLE_AI_CHAT)
#include "brave/components/ai_chat/core/common/pref_names.h"
#endif

#if BUILDFLAG(ENABLE_BRAVE_VPN)
#include "brave/components/brave_vpn/common/pref_names.h"
#endif

#if BUILDFLAG(ENABLE_SPEEDREADER)
#include "brave/components/speedreader/speedreader_pref_names.h"
#endif

#if BUILDFLAG(ENABLE_BRAVE_TALK)
#include "brave/components/brave_talk/pref_names.h"
#endif

namespace brave_policy {

namespace {

bool g_disable_caching_for_testing = false;

// Preference names that do not support dynamic refresh.
// All policy values will be initialized at browser start
// and cached for the lifetime of the browser process.
constexpr const char* kNonDynamicPrefs[] = {
#if BUILDFLAG(ENABLE_AI_CHAT)
    ai_chat::prefs::kEnabledByPolicy,
#endif
    brave_news::prefs::kBraveNewsDisabledByPolicy,
    brave_rewards::prefs::kDisabledByPolicy,
#if BUILDFLAG(ENABLE_BRAVE_VPN)
    brave_vpn::prefs::kManagedBraveVPNDisabled,
#endif
    brave_wallet::kBraveWalletDisabledByPolicy,
    brave_talk::prefs::kDisabledByPolicy,
    playlist::kPlaylistEnabledPref,
#if BUILDFLAG(ENABLE_SPEEDREADER)
    speedreader::kSpeedreaderEnabled,
#endif
};

}  // namespace

PolicyPrefInterceptor::PolicyPrefInterceptor() = default;

PolicyPrefInterceptor::~PolicyPrefInterceptor() = default;

// static
void PolicyPrefInterceptor::DisableCachingForTesting() {
  g_disable_caching_for_testing = true;
}

void PolicyPrefInterceptor::InterceptPrefValues(PrefValueMap* pref_value_map) {
  if (!pref_value_map || g_disable_caching_for_testing) {
    return;
  }

  for (const auto* pref_name : kNonDynamicPrefs) {
    if (!initial_policies_loaded_) {
      // First time - cache the initial values.
      bool value = false;
      if (pref_value_map->GetBoolean(pref_name, &value)) {
        pref_cache_[pref_name] = value;
      }
    } else {
      // Subsequent calls - use cached values or remove if not cached.
      if (const auto* cached_value = base::FindOrNull(pref_cache_, pref_name)) {
        pref_value_map->SetBoolean(pref_name, *cached_value);
      } else {
        pref_value_map->RemoveValue(pref_name);
      }
    }
  }

  if (!initial_policies_loaded_) {
    initial_policies_loaded_ = true;
  }
}

}  // namespace brave_policy
