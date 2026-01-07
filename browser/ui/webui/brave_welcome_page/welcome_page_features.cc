// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/brave_welcome_page/welcome_page_features.h"

#include "base/check.h"
#include "brave/components/ai_chat/core/common/buildflags/buildflags.h"
#include "brave/components/brave_rewards/core/buildflags/buildflags.h"
#include "brave/components/brave_vpn/common/buildflags/buildflags.h"
#include "brave/components/brave_wallet/common/buildflags/buildflags.h"
#include "chrome/browser/profiles/profile.h"

#if BUILDFLAG(ENABLE_AI_CHAT)
#include "brave/browser/ai_chat/ai_chat_utils.h"
#endif

#if BUILDFLAG(ENABLE_BRAVE_REWARDS)
#include "brave/browser/brave_rewards/rewards_util.h"
#endif

#if BUILDFLAG(ENABLE_BRAVE_VPN)
#include "brave/browser/brave_vpn/vpn_utils.h"
#endif

#if BUILDFLAG(ENABLE_BRAVE_WALLET)
#include "brave/browser/brave_wallet/brave_wallet_context_utils.h"
#endif

namespace brave_welcome_page {

base::flat_set<mojom::Feature> GetAvailableFeatures(Profile* profile) {
  CHECK(profile);

  base::flat_set<mojom::Feature> features;

#if BUILDFLAG(ENABLE_AI_CHAT)
  if (ai_chat::IsAllowedForContext(profile)) {
    features.insert(mojom::Feature::kAIChat);
  }
#endif

#if BUILDFLAG(ENABLE_BRAVE_WALLET)
  if (brave_wallet::IsAllowedForContext(profile)) {
    features.insert(mojom::Feature::kWallet);
  }
#endif

#if BUILDFLAG(ENABLE_BRAVE_REWARDS)
  if (brave_rewards::IsSupportedForProfile(profile)) {
    features.insert(mojom::Feature::kRewards);
  }
#endif

#if BUILDFLAG(ENABLE_BRAVE_VPN)
  if (brave_vpn::IsBraveVPNEnabled(profile)) {
    features.insert(mojom::Feature::kVPN);
  }
#endif

  return features;
}

}  // namespace brave_welcome_page
