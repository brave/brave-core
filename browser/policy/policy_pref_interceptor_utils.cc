/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/policy/policy_pref_interceptor_utils.h"

#include <string_view>

#include "brave/components/ai_chat/core/common/buildflags/buildflags.h"
#include "brave/components/brave_news/common/buildflags/buildflags.h"
#include "brave/components/brave_policy/policy_pref_interceptor_list.h"
#include "brave/components/brave_rewards/core/pref_names.h"
#include "brave/components/brave_talk/buildflags/buildflags.h"
#include "brave/components/brave_vpn/common/buildflags/buildflags.h"
#include "brave/components/brave_wallet/common/buildflags/buildflags.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/playlist/core/common/buildflags/buildflags.h"
#include "brave/components/speedreader/common/buildflags/buildflags.h"

#if BUILDFLAG(ENABLE_AI_CHAT)
#include "brave/components/ai_chat/core/common/pref_names.h"
#endif

#if BUILDFLAG(ENABLE_BRAVE_NEWS)
#include "brave/components/brave_news/common/pref_names.h"
#endif

#if BUILDFLAG(ENABLE_BRAVE_VPN)
#include "brave/components/brave_vpn/common/pref_names.h"
#endif

#if BUILDFLAG(ENABLE_BRAVE_WALLET)
#include "brave/components/brave_wallet/browser/pref_names.h"
#endif

#if BUILDFLAG(ENABLE_BRAVE_TALK)
#include "brave/components/brave_talk/pref_names.h"
#endif

#if BUILDFLAG(ENABLE_PLAYLIST)
#include "brave/components/playlist/core/common/pref_names.h"
#endif

#if BUILDFLAG(ENABLE_SPEEDREADER)
#include "brave/components/speedreader/speedreader_pref_names.h"
#endif

namespace brave_policy {

namespace {

// Pref names that do not support dynamic policy refresh.
constexpr std::string_view kNonDynamicPrefs[] = {
#if BUILDFLAG(ENABLE_AI_CHAT)
    ai_chat::prefs::kEnabledByPolicy,
#endif
#if BUILDFLAG(ENABLE_BRAVE_NEWS)
    brave_news::prefs::kBraveNewsDisabledByPolicy,
#endif
    brave_rewards::prefs::kDisabledByPolicy,
#if BUILDFLAG(ENABLE_BRAVE_VPN)
    brave_vpn::prefs::kManagedBraveVPNDisabled,
#endif
#if BUILDFLAG(ENABLE_BRAVE_WALLET)
    brave_wallet::kBraveWalletDisabledByPolicy,
#endif
#if BUILDFLAG(ENABLE_BRAVE_TALK)
    brave_talk::prefs::kDisabledByPolicy,
#endif
#if BUILDFLAG(ENABLE_PLAYLIST)
    playlist::kPlaylistEnabledPref,
#endif
#if BUILDFLAG(ENABLE_SPEEDREADER)
    speedreader::kSpeedreaderEnabled,
#endif
};

}  // namespace

void InitializePolicyPrefInterceptorList() {
  PolicyPrefInterceptorList::GetInstance()->SetPrefs(kNonDynamicPrefs);
}

}  // namespace brave_policy
