/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_account/features.h"

#include "brave/components/email_aliases/buildflags/buildflags.h"

#if BUILDFLAG(ENABLE_EMAIL_ALIASES)
#include "brave/components/email_aliases/features.h"
#endif

namespace brave_account::features {

namespace {
BASE_FEATURE(kBraveAccount, base::FEATURE_DISABLED_BY_DEFAULT);
}  // namespace

// Process-wide: true if kBraveAccount is on, or a dependent feature (e.g.
// Email Aliases) is built and feature-flagged on. Use when per-profile state
// shouldn't or can't influence the answer.
bool IsBraveAccountEnabled() {
  return base::FeatureList::IsEnabled(kBraveAccount)
#if BUILDFLAG(ENABLE_EMAIL_ALIASES)
         || email_aliases::features::IsEmailAliasesEnabled()
#endif
      ;
}

// Per-profile: "is Brave Account active for this profile?".
// Use for UI gating and other per-profile decisions. The result can differ
// across profiles because dependent features (Email Aliases) consult a
// per-profile pref.
bool IsBraveAccountEnabledForProfile(const PrefService& pref_service) {
  return base::FeatureList::IsEnabled(kBraveAccount)
#if BUILDFLAG(ENABLE_EMAIL_ALIASES)
         ||
         email_aliases::features::IsEmailAliasesEnabledForProfile(pref_service)
#endif
      ;
}

const base::Feature& BraveAccountFeatureForTesting() {
  return kBraveAccount;
}

}  // namespace brave_account::features
