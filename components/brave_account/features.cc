/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_account/features.h"

#include "brave/components/email_aliases/features.h"

namespace brave_account::features {

namespace {
BASE_FEATURE(kBraveAccount, base::FEATURE_DISABLED_BY_DEFAULT);
}  // namespace

// Brave Account is enabled when:
// - the explicit kBraveAccount feature flag is enabled (dev/testing), OR
// - a dependent feature (e.g. Email Aliases) requires it
bool IsBraveAccountEnabled() {
  return base::FeatureList::IsEnabled(kBraveAccount) ||
         email_aliases::features::IsEmailAliasesEnabled();
}

const base::Feature& BraveAccountFeatureForTesting() {
  return kBraveAccount;
}

}  // namespace brave_account::features
